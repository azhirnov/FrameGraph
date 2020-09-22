// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#if defined(FG_ENABLE_OPENVR) && defined(FG_ENABLE_VULKAN)

# include "extensions/vulkan_loader/VulkanLoader.h"
# include "extensions/vulkan_loader/VulkanCheckError.h"
# include "framework/VR/IVRDevice.h"
# include "stl/Containers/Ptr.h"
# include "stl/Containers/FixedMap.h"
# include "openvr_capi.h"
# include <chrono>

namespace FGC
{

	//
	// OpenVR Device
	//

	class OpenVRDevice final : public IVRDevice
	{
	// types
	private:
		using Listeners_t	 = HashSet< IVRDeviceEventListener *>;
		using TimePoint_t	= std::chrono::high_resolution_clock::time_point;
		using SecondsF		= std::chrono::duration< float >;

		struct ControllerAxis
		{
			float2		value;
			bool		pressed	= false;
		};
		using AxisStates_t	= StaticArray< ControllerAxis, k_unControllerStateAxisCount >;
		using Keys_t		= StaticArray< bool, EVRButtonId_k_EButton_Max >;

		struct Controller
		{
			ControllerID	id;
			uint			lastPacket	= ~0u;
			Keys_t			keys;
			AxisStates_t	axis;
			TimePoint_t		lastUpdate;
		};
		using Controllers_t	= FixedMap< /*tracked device index*/uint, Controller, 8 >;
		
		using IVRSystemPtr			= intptr_t;
		using IVRSystemTable		= VR_IVRSystem_FnTable *;
		using IVRCompositorTable	= VR_IVRCompositor_FnTable *;

		struct OpenVRLoader
		{
		// types
			using VR_InitInternal_t							= intptr_t (*) (EVRInitError *peError, EVRApplicationType eType);
			using VR_ShutdownInternal_t						= void (*) ();
			using VR_IsHmdPresent_t							= int (*) ();
			using VR_GetGenericInterface_t					= intptr_t (*) (const char *pchInterfaceVersion, EVRInitError *peError);
			using VR_IsRuntimeInstalled_t					= int (*) ();
			using VR_GetVRInitErrorAsSymbol_t				= const char* (*) (EVRInitError error);
			using VR_GetVRInitErrorAsEnglishDescription_t	= const char* (*) (EVRInitError error);

		// variables
			VR_InitInternal_t							InitInternal;
			VR_ShutdownInternal_t						ShutdownInternal;
			VR_IsHmdPresent_t							IsHmdPresent;
			VR_GetGenericInterface_t					GetGenericInterface;
			VR_IsRuntimeInstalled_t						IsRuntimeInstalled;
			VR_GetVRInitErrorAsSymbol_t					GetVRInitErrorAsSymbol;
			VR_GetVRInitErrorAsEnglishDescription_t		GetVRInitErrorAsEnglishDescription;
		};


	// variables
	private:
		IVRSystemPtr			_hmd			= Zero;
		IVRSystemTable			_vrSystem		= null;
		IVRCompositorTable		_vrCompositor	= null;

		Listeners_t				_listeners;
		VRCamera				_camera;

		VkInstance				_vkInstance;
		VkPhysicalDevice		_vkPhysicalDevice;
		VkDevice				_vkLogicalDevice;

		BitSet<2>				_submitted;
		EHmdStatus				_hmdStatus		= EHmdStatus::PowerOff;

		Controllers_t			_controllers;
		VRControllers_t			_vrControllers;
		TrackedDevicePose_t		_trackedDevicePose [k_unMaxTrackedDeviceCount];
		
		OpenVRLoader			_ovr;
		void*					_openVRLib		= null;


	// methods
	public:
		OpenVRDevice ();
		~OpenVRDevice () override;
		
		bool  Create () override;
		bool  SetVKDevice (InstanceVk_t instance, PhysicalDeviceVk_t physicalDevice, DeviceVk_t logicalDevice) override;
		void  Destroy () override;
		void  AddListener (IVRDeviceEventListener *listener) override;
		void  RemoveListener (IVRDeviceEventListener *listener) override;
		bool  Update () override;
		void  SetupCamera (const float2 &clipPlanes) override;
		bool  Submit (const VRImage &, Eye) override;
		
		VRCamera const&			GetCamera () const override				{ return _camera; }
		VRControllers_t const&	GetControllers () const override		{ return _vrControllers; }
		
		EHmdStatus		GetHmdStatus () const override;
		Array<String>	GetRequiredInstanceExtensions () const override;
		Array<String>	GetRequiredDeviceExtensions (InstanceVk_t instance) const override;
		uint2			GetRenderTargetDimension () const override;

	private:
		bool _LoadLib ();
		void _UnloadLib ();

		void  _ProcessHmdEvents (const VREvent_t &);
		void  _ProcessControllerEvents (INOUT Controller&, const VREvent_t &);
		void  _UpdateHMDMatrixPose ();
		void  _InitControllers ();

		ND_ ControllerID  _GetControllerID (uint tdi) const;

		ND_ String  _GetTrackedDeviceString (TrackedDeviceIndex_t unDevice, TrackedDeviceProperty prop, TrackedPropertyError *peError = null) const;
	};


}	// FGC

#endif	// FG_ENABLE_OPENVR and FG_ENABLE_VULKAN

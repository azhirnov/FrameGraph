// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_OPENVR

# include "framework/VR/IVRDevice.h"
# include "stl/Containers/Ptr.h"
# include "stl/Containers/FixedMap.h"
# include "openvr.h"

namespace FGC
{

	//
	// OpenVR Device
	//

	class OpenVRDevice final : public IVRDevice
	{
	// types
	private:
		using Listeners_t		 = HashSet< IVRDeviceEventListener *>;

		struct Controller
		{
			using AxisStates_t	= StaticArray< float2, vr::k_unControllerStateAxisCount >;
			using Keys_t		= StaticArray< bool, vr::k_EButton_Max >;

			ControllerID	id;
			uint			lastPacket = ~0u;
			Keys_t			keys;
			AxisStates_t	axis;

			explicit Controller (ControllerID id);
		};
		using Controllers_t	= FixedMap< uint, Controller, 8 >;


	// variables
	private:
		Ptr<vr::IVRSystem>			_hmd;
		Listeners_t					_listeners;
		VRCamera					_camera;

		VkInstance					_vkInstance;
		VkPhysicalDevice			_vkPhysicalDevice;
		VkDevice					_vkLogicalDevice;

		BitSet<2>					_submitted;
		EHmdStatus					_hmdStatus		= EHmdStatus::PowerOff;

		Controllers_t				_controllers;
		vr::TrackedDevicePose_t		_trackedDevicePose [vr::k_unMaxTrackedDeviceCount];
		Ptr<vr::IVRRenderModels>	_renderModels;


	// methods
	public:
		OpenVRDevice ();
		~OpenVRDevice () override;
		
		bool Create () override;
		bool SetVKDevice (VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice logicalDevice) override;
		void Destroy () override;
		void AddListener (IVRDeviceEventListener *listener) override;
		void RemoveListener (IVRDeviceEventListener *listener) override;
		bool Update () override;
		void SetupCamera (const float2 &clipPlanes) override;
		bool Submit (const VRImage &, Eye) override;
		
		VRCamera const&	GetCamera () const override						{ return _camera; }
		EHmdStatus		GetHmdStatus () const override;
		Array<String>	GetRequiredInstanceExtensions () const override;
		Array<String>	GetRequiredDeviceExtensions (VkInstance instance) const override;
		uint2			GetRenderTargetDimension () const override;

	private:
		void _ProcessHmdEvents (const vr::VREvent_t &);
		void _ProcessControllerEvents (INOUT Controller&, const vr::VREvent_t &);
		void _UpdateHMDMatrixPose ();
	};


}	// FGC

#endif	// FG_ENABLE_OPENVR

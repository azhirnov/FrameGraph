// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/VR/IVRDevice.h"
#include "stl/Containers/Ptr.h"

#ifdef FG_ENABLE_OPENVR
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
		using Listeners_t	= HashSet< IVRDeviceEventListener *>;


	// variables
	private:
		Ptr<vr::IVRSystem>			_hmd;
		Listeners_t					_listeners;
		VRCamera					_camera;

		VkInstance					_vkInstance;
		VkPhysicalDevice			_vkPhysicalDevice;
		VkDevice					_vkLogicalDevice;

		BitSet<2>					_submitted;
		bool						_enabled		= false;

		Mat4_t						_devicePose [vr::k_unMaxTrackedDeviceCount];
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
		void GetCamera (OUT VRCamera &) const override;
		void SetupCamera (const float2 &clipPlanes) override;
		bool Submit (const VRImage &, Eye) override;
		bool IsEnabled () const override;
		Array<String> GetRequiredInstanceExtensions () const override;
		Array<String> GetRequiredDeviceExtensions (VkInstance instance) const override;
		uint2 GetRenderTargetDimension () const override;

	private:
		void _UpdateHMDMatrixPose ();
	};


}	// FGC

#endif	// FG_ENABLE_OPENVR

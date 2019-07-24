// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"
#include "stl/Math/Rectangle.h"
#include "stl/Math/Matrix.h"
#include "vulkan_loader/VulkanLoader.h"
#include "vulkan_loader/VulkanCheckError.h"

namespace FGC
{

	//
	// VR Device Event Listener interface
	//

	class IVRDeviceEventListener
	{
	// types
	public:
		enum class EHmdStatus {
			PowerOff,
			Standby,
			Active,
			Mounted,
		};

		enum class ControllerID {
			Unknown,
			LeftHand,
			RightHand,
		};
		
		enum class EKeyAction {
			Up,			// signle event when key up
			Down,		// single event when key down
			Pressed,	// continiously event until key is pressed
		};

		
	// interface
	public:
		virtual void HmdStatusChanged (EHmdStatus) = 0;

		virtual void OnAxisStateChanged (ControllerID id, const float2 &value, const float2 &delta) = 0;
		virtual void OnKey (ControllerID id, StringView key, EKeyAction action) = 0;
	};



	//
	// VR Device interface
	//

	class IVRDevice
	{
	// types
	public:
		using Mat4_t		= Matrix< float, 4, 4, EMatrixOrder::ColumnMajor >;
		using Mat3_t		= Matrix< float, 3, 3, EMatrixOrder::ColumnMajor >;
		using EHmdStatus	= IVRDeviceEventListener::EHmdStatus;
		using ControllerID	= IVRDeviceEventListener::ControllerID;
		using EKeyAction	= IVRDeviceEventListener::EKeyAction;

		struct VRImage
		{
			VkImage		handle				= VK_NULL_HANDLE;
			VkQueue		currQueue			= VK_NULL_HANDLE;
			uint		queueFamilyIndex	= UMax;
			uint2		dimension;
			RectF		bounds;
			VkFormat	format				= VK_FORMAT_UNDEFINED;
			uint		sampleCount			= 1;
		};

		struct VRCamera
		{
			struct PerEye {
				Mat4_t		proj;
				Mat4_t		view;
			};
			PerEye		left;
			PerEye		right;
			Mat3_t		pose;			// hmd rotation
			float3		position;		// hmd position
			float2		clipPlanes;
			float3		velocity;
			float3		angularVelocity;
		};

		enum class Eye
		{
			Left,
			Right,
		};


	// interface
	public:
		virtual ~IVRDevice () {}
		virtual bool Create () = 0;
		virtual bool SetVKDevice (VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice logicalDevice) = 0;
		virtual void Destroy () = 0;
		virtual void AddListener (IVRDeviceEventListener *listener) = 0;
		virtual void RemoveListener (IVRDeviceEventListener *listener) = 0;
		virtual bool Update () = 0;
		virtual void SetupCamera (const float2 &clipPlanes) = 0;
		virtual bool Submit (const VRImage &, Eye) = 0;

		ND_ virtual EHmdStatus		GetHmdStatus () const = 0;
		ND_ virtual VRCamera const&	GetCamera () const = 0;
		ND_ virtual Array<String>	GetRequiredInstanceExtensions () const = 0;
		ND_ virtual Array<String>	GetRequiredDeviceExtensions (VkInstance instance = VK_NULL_HANDLE) const = 0;
		ND_ virtual uint2			GetRenderTargetDimension () const = 0;
	};


	using VRDevicePtr = UniquePtr< IVRDevice >;


	ND_ inline bool  operator < (IVRDevice::EHmdStatus lhs, IVRDevice::EHmdStatus rhs) {
		return uint(lhs) < uint(rhs);
	}

}	// FGC

// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"
#include "stl/Math/Rectangle.h"
#include "stl/Math/Matrix.h"
#include "stl/Containers/FixedMap.h"

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
		
		enum class EButtonAction {
			Up,			// signle event when controller button up
			Down,		// single event when controller button down
			Pressed,	// continiously event until controller button is pressed
		};
		
		using Mat3_t = Matrix< float, 3, 3, EMatrixOrder::ColumnMajor >;

		
	// interface
	public:
		virtual void  HmdStatusChanged (EHmdStatus) = 0;

		virtual void  OnAxisStateChanged (ControllerID id, StringView name, const float2 &value, const float2 &delta, float dt) = 0;
		virtual void  OnButton (ControllerID id, StringView btn, EButtonAction action) = 0;
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
		using EButtonAction	= IVRDeviceEventListener::EButtonAction;
		
		using InstanceVk_t			= struct __VrVkInstanceType *;
		using PhysicalDeviceVk_t	= struct __VrVkPhysicalDeviceType *;
		using DeviceVk_t			= struct __VrVkDeviceType *;
		enum  ImageVk_t				: uint64_t {};
		using QueueVk_t				= struct __VrVkQueueType *;
		enum  FormatVk_t			: uint {};

		struct VRImage
		{
			ImageVk_t		handle				= Zero;
			QueueVk_t		currQueue			= Zero;
			uint			queueFamilyIndex	= UMax;
			uint2			dimension;
			RectF			bounds;
			FormatVk_t		format				= Zero;
			uint			sampleCount			= 1;
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

		struct VRController
		{
			Mat3_t		pose;			// hmd rotation
			float3		position;		// hmd position
			float3		velocity;
			float3		angularVelocity;
			bool		isValid			= false;
		};
		using VRControllers_t = FixedMap< ControllerID, VRController, 8 >;

		enum class Eye
		{
			Left,
			Right,
		};


	// interface
	public:
		virtual ~IVRDevice () {}
		virtual bool  Create () = 0;
		virtual bool  SetVKDevice (InstanceVk_t instance, PhysicalDeviceVk_t physicalDevice, DeviceVk_t logicalDevice) = 0;
		virtual void  Destroy () = 0;
		virtual void  AddListener (IVRDeviceEventListener *listener) = 0;
		virtual void  RemoveListener (IVRDeviceEventListener *listener) = 0;
		virtual bool  Update () = 0;
		virtual void  SetupCamera (const float2 &clipPlanes) = 0;
		virtual bool  Submit (const VRImage &, Eye) = 0;

		ND_ virtual EHmdStatus				GetHmdStatus () const = 0;
		ND_ virtual VRCamera const&			GetCamera () const = 0;
		ND_ virtual VRControllers_t const&	GetControllers () const = 0;
		ND_ virtual Array<String>			GetRequiredInstanceExtensions () const = 0;
		ND_ virtual Array<String>			GetRequiredDeviceExtensions (InstanceVk_t instance = Zero) const = 0;
		ND_ virtual uint2					GetRenderTargetDimension () const = 0;
	};


	using VRDevicePtr = UniquePtr< IVRDevice >;


	ND_ inline bool  operator < (IVRDevice::EHmdStatus lhs, IVRDevice::EHmdStatus rhs) {
		return uint(lhs) < uint(rhs);
	}

}	// FGC

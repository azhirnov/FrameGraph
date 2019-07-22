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
		
	// interface
	public:
		virtual void DeviceActivated () = 0;
		virtual void DeviceDeactivated () = 0;
	};



	//
	// VR Device interface
	//

	class IVRDevice
	{
	// types
	public:
		using Mat4_t	= Matrix< float, 4, 4, EMatrixOrder::ColumnMajor >;
		using Mat3_t	= Matrix< float, 3, 3, EMatrixOrder::ColumnMajor >;

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
		virtual void GetCamera (OUT VRCamera &) const = 0;
		virtual void SetupCamera (const float2 &clipPlanes) = 0;
		virtual bool Submit (const VRImage &, Eye) = 0;
		ND_ virtual bool IsEnabled () const = 0;
		ND_ virtual Array<String> GetRequiredInstanceExtensions () const = 0;
		ND_ virtual Array<String> GetRequiredDeviceExtensions (VkInstance instance = VK_NULL_HANDLE) const = 0;
		ND_ virtual uint2 GetRenderTargetDimension () const = 0;
	};


	using VRDevicePtr = UniquePtr< IVRDevice >;

}	// FGC

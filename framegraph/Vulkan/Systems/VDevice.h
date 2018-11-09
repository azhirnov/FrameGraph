// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"
#include "framegraph/Public/ShaderEnums.h"

namespace FG
{

	//
	// Device Queue
	//

	struct VDeviceQueueInfo
	{
	// variables
		mutable std::mutex	lock;			// use when call vkQueueSubmit, vkQueueWaitIdle, vkQueueBindSparse, vkQueuePresentKHR
		VkQueue				id				= null;
		uint				familyIndex		= ~0u;
		VkQueueFlags		familyFlags		= {};
		float				priority		= 0.0f;
		DebugName_t			debugName;
		
	// methods
		VDeviceQueueInfo () {}

		VDeviceQueueInfo (VDeviceQueueInfo &&other) :
			id{other.id}, familyIndex{other.familyIndex}, familyFlags{other.familyFlags},
			priority{other.priority}, debugName{std::move(other.debugName)}
		{}
	};



	//
	// Vulkan Device
	//

	class VDevice final : public VulkanDeviceFn
	{
		friend class VFrameGraph;

	// types
	private:
		using Queues_t				= FixedArray< VDeviceQueueInfo, 16 >;
		using InstanceLayers_t		= Array< VkLayerProperties >;
		using ExtensionName_t		= StaticString<VK_MAX_EXTENSION_NAME_SIZE>;
		using ExtensionSet_t		= HashSet< ExtensionName_t >;


	// variables
	private:
		VkInstance							_vkInstance;
		VkPhysicalDevice					_vkPhysicalDevice;
		VkDevice							_vkDevice;
		EShaderLangFormat					_vkVersion;
		Queues_t							_vkQueues;

		VkPhysicalDeviceProperties			_deviceProperties;
		VkPhysicalDeviceFeatures			_deviceFeatures;
		VkPhysicalDeviceMemoryProperties	_deviceMemoryProperties;
		mutable InstanceLayers_t			_instanceLayers;
		mutable ExtensionSet_t				_instanceExtensions;
		mutable ExtensionSet_t				_deviceExtensions;
		
		VulkanDeviceFnTable					_deviceFnTable;

		bool								_enableDebugMarkers;


	// methods
	public:
		explicit VDevice (const VulkanDeviceInfo &vdi);
		~VDevice ();

		ND_ VkDevice								GetVkDevice ()				const	{ return _vkDevice; }
		ND_ VkPhysicalDevice						GetVkPhysicalDevice ()		const	{ return _vkPhysicalDevice; }
		ND_ VkInstance								GetVkInstance ()			const	{ return _vkInstance; }
		ND_ EShaderLangFormat						GetVkVersion ()				const	{ return _vkVersion; }
		ND_ ArrayView< VDeviceQueueInfo >			GetVkQueues ()				const	{ return _vkQueues; }
		
		ND_ VkPhysicalDeviceProperties const&		GetDeviceProperties ()		 const	{ return _deviceProperties; }
		ND_ VkPhysicalDeviceFeatures const&			GetDeviceFeatures ()		 const	{ return _deviceFeatures; }
		ND_ VkPhysicalDeviceMemoryProperties const&	GetDeviceMemoryProperties () const	{ return _deviceMemoryProperties; }
		ND_ VkPhysicalDeviceLimits const&			GetDeviceLimits ()			 const	{ return _deviceProperties.limits; }
		ND_ VkPhysicalDeviceSparseProperties const&	GetDeviceSparseProperties () const	{ return _deviceProperties.sparseProperties; }


		// check extensions
		ND_ bool HasLayer (StringView name) const;
		ND_ bool HasExtension (StringView name) const;
		ND_ bool HasDeviceExtension (StringView name) const;
		
		bool SetObjectName (uint64_t id, StringView name, VkDebugReportObjectTypeEXT type) const;


	private:
		bool _LoadInstanceLayers () const;
		bool _LoadInstanceExtensions () const;
		bool _LoadDeviceExtensions () const;
	};


}	// FG

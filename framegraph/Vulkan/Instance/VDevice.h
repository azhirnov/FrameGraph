// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
		VkQueue				handle			= VK_NULL_HANDLE;
		EQueueFamily		familyIndex		= Default;
		VkQueueFlags		familyFlags		= {};
		float				priority		= 0.0f;
		uint3				minImageTransferGranularity;
		DebugName_t			debugName;
		
	// methods
		VDeviceQueueInfo () {}

		VDeviceQueueInfo (VDeviceQueueInfo &&other) :
			handle{other.handle}, familyIndex{other.familyIndex}, familyFlags{other.familyFlags},
			priority{other.priority}, minImageTransferGranularity{other.minImageTransferGranularity},
			debugName{std::move(other.debugName)}
		{}
	};



	//
	// Vulkan Device
	//

	class VDevice final : public VulkanDeviceFn
	{
		friend class VFrameGraphInstance;

	// types
	private:
		using Queues_t				= FixedArray< VDeviceQueueInfo, 16 >;
		using InstanceLayers_t		= Array< VkLayerProperties >;
		using ExtensionName_t		= StaticString<VK_MAX_EXTENSION_NAME_SIZE>;
		using ExtensionSet_t		= HashSet< ExtensionName_t >;


	// variables
	private:
		VkInstance								_vkInstance;
		VkPhysicalDevice						_vkPhysicalDevice;
		VkDevice								_vkDevice;
		EShaderLangFormat						_vkVersion;
		Queues_t								_vkQueues;

		VkPhysicalDeviceProperties				_deviceProperties;
		VkPhysicalDeviceFeatures				_deviceFeatures;
		VkPhysicalDeviceMemoryProperties		_deviceMemoryProperties;

		VkPhysicalDeviceMeshShaderPropertiesNV			_deviceMeshShaderProperties;
		VkPhysicalDeviceRayTracingPropertiesNV			_deviceRayTracingProperties;
		VkPhysicalDeviceShadingRateImagePropertiesNV	_deviceShadingRateImageProperties;

		mutable ExtensionSet_t					_instanceExtensions;
		mutable ExtensionSet_t					_deviceExtensions;
		
		VulkanDeviceFnTable						_deviceFnTable;

		bool									_enableDebugUtils	: 1;
		bool									_enableMeshShaderNV	: 1;
		bool									_enableRayTracingNV	: 1;


	// methods
	public:
		explicit VDevice (const VulkanDeviceInfo &vdi);
		~VDevice ();

		ND_ bool							IsDebugUtilsEnabled ()		const	{ return _enableDebugUtils; }
		ND_ bool							IsMeshShaderEnabled ()		const	{ return _enableMeshShaderNV; }
		ND_ bool							IsRayTracingEnabled ()		const	{ return _enableRayTracingNV; }

		ND_ VkDevice						GetVkDevice ()				const	{ return _vkDevice; }
		ND_ VkPhysicalDevice				GetVkPhysicalDevice ()		const	{ return _vkPhysicalDevice; }
		ND_ VkInstance						GetVkInstance ()			const	{ return _vkInstance; }
		ND_ EShaderLangFormat				GetVkVersion ()				const	{ return _vkVersion; }
		ND_ ArrayView< VDeviceQueueInfo >	GetVkQueues ()				const	{ return _vkQueues; }
		

		ND_ VkPhysicalDeviceProperties const&					GetDeviceProperties ()					const	{ return _deviceProperties; }
		ND_ VkPhysicalDeviceFeatures const&						GetDeviceFeatures ()					const	{ return _deviceFeatures; }
		ND_ VkPhysicalDeviceMemoryProperties const&				GetDeviceMemoryProperties ()			const	{ return _deviceMemoryProperties; }
		ND_ VkPhysicalDeviceLimits const&						GetDeviceLimits ()						const	{ return _deviceProperties.limits; }
		ND_ VkPhysicalDeviceSparseProperties const&				GetDeviceSparseProperties ()			const	{ return _deviceProperties.sparseProperties; }
		ND_ VkPhysicalDeviceMeshShaderPropertiesNV const&		GetDeviceMeshShaderProperties ()		const	{ return _deviceMeshShaderProperties; }
		ND_ VkPhysicalDeviceRayTracingPropertiesNV const&		GetDeviceRayTracingProperties ()		const	{ return _deviceRayTracingProperties; }
		ND_ VkPhysicalDeviceShadingRateImagePropertiesNV const&	GetDeviceShadingRateImageProperties ()	const	{ return _deviceShadingRateImageProperties; }


		// check extensions
		ND_ bool HasExtension (StringView name) const;
		ND_ bool HasDeviceExtension (StringView name) const;
		
		bool SetObjectName (uint64_t id, StringView name, VkObjectType type) const;


	private:
		bool _LoadInstanceExtensions () const;
		bool _LoadDeviceExtensions () const;
	};


}	// FG

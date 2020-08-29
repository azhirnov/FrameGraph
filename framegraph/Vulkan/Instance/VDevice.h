// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"
#include "framegraph/Public/ShaderEnums.h"
#include "framegraph/Public/EResourceState.h"

namespace FG
{

	//
	// Device Queue
	//

	struct VDeviceQueueInfo
	{
	// variables
		mutable Mutex		guard;			// use when call vkQueueSubmit, vkQueueWaitIdle, vkQueueBindSparse, vkQueuePresentKHR
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
			debugName{other.debugName}
		{}
	};



	//
	// Vulkan Device
	//

	class VDevice final : public VulkanDeviceFn
	{
	// types
	public:
		struct EnabledFeatures
		{
			// vulkan 1.1 core
			bool	bindMemory2				: 1;
			bool	dedicatedAllocation		: 1;
			bool	descriptorUpdateTemplate: 1;
			bool	imageViewUsage			: 1;
			bool	create2DArrayCompatible	: 1;
			bool	commandPoolTrim			: 1;
			bool	dispatchBase			: 1;
			// vulkan 1.2 core
			bool	samplerMirrorClamp		: 1;
			bool	shaderAtomicInt64		: 1;	// for uniform/storage buffer, for shared variables check features
			bool	float16Arithmetic		: 1;
			bool	bufferAddress			: 1;
			bool	descriptorIndexing		: 1;
			bool	renderPass2				: 1;
			bool	depthStencilResolve		: 1;
			bool	drawIndirectCount		: 1;
			bool	spirv14					: 1;
			bool	memoryModel				: 1;
			// window extensions
			bool	surface					: 1;
			bool	surfaceCaps2			: 1;
			bool	swapchain				: 1;
			// extensions
			bool	debugUtils				: 1;
			bool	meshShaderNV			: 1;
			bool	rayTracingNV			: 1;
			bool	shadingRateImageNV		: 1;
		//	bool	pushDescriptor			: 1;
		//	bool	inlineUniformBlock		: 1;
			bool	shaderClock				: 1;
			bool	timelineSemaphore		: 1;
			bool	pushDescriptor			: 1;
		};

		struct DeviceProperties
		{
			VkPhysicalDeviceProperties							properties;
			VkPhysicalDeviceFeatures							features;
			VkPhysicalDeviceMemoryProperties					memoryProperties;
			#ifdef VK_VERSION_1_1
			VkPhysicalDeviceSubgroupProperties					subgroup;
			#endif
			#ifdef VK_KHR_vulkan_memory_model
			VkPhysicalDeviceVulkanMemoryModelFeaturesKHR		memoryModel;
			#endif
			#ifdef VK_NV_mesh_shader
			VkPhysicalDeviceMeshShaderFeaturesNV				meshShaderFeatures;
			VkPhysicalDeviceMeshShaderPropertiesNV				meshShaderProperties;
			#endif
			#ifdef VK_NV_shading_rate_image
			VkPhysicalDeviceShadingRateImageFeaturesNV			shadingRateImageFeatures;
			VkPhysicalDeviceShadingRateImagePropertiesNV		shadingRateImageProperties;
			#endif
			#ifdef VK_NV_ray_tracing
			VkPhysicalDeviceRayTracingPropertiesNV				rayTracingProperties;
			#endif
			#ifdef VK_KHR_shader_clock
			VkPhysicalDeviceShaderClockFeaturesKHR				shaderClock;
			#endif
			#ifdef VK_KHR_timeline_semaphore
			VkPhysicalDeviceTimelineSemaphorePropertiesKHR		timelineSemaphoreProps;
			#endif
			#ifdef VK_KHR_buffer_device_address
			VkPhysicalDeviceBufferDeviceAddressFeaturesKHR		bufferDeviceAddress;
			#endif
			#ifdef VK_KHR_depth_stencil_resolve
			VkPhysicalDeviceDepthStencilResolvePropertiesKHR	depthStencilResolve;
			#endif
			#ifdef VK_KHR_shader_atomic_int64
			VkPhysicalDeviceShaderAtomicInt64FeaturesKHR		shaderAtomicInt64;
			#endif
			#ifdef VK_EXT_descriptor_indexing
			VkPhysicalDeviceDescriptorIndexingFeaturesEXT		descriptorIndexingFeatures;
			VkPhysicalDeviceDescriptorIndexingPropertiesEXT		descriptorIndexingProperties;
			#endif
			#ifdef VK_VERSION_1_2
			VkPhysicalDeviceVulkan11Properties					properties110;
			VkPhysicalDeviceVulkan12Properties					properties120;
			#endif
		};
		
	private:
		using Queues_t				= FixedArray< VDeviceQueueInfo, 16 >;
		using InstanceLayers_t		= Array< VkLayerProperties >;
		using ExtensionName_t		= StaticString<VK_MAX_EXTENSION_NAME_SIZE>;
		using ExtensionSet_t		= HashSet< ExtensionName_t >;
		

	// variables
	private:
		VkInstance				_vkInstance;
		VkPhysicalDevice		_vkPhysicalDevice;
		VkDevice				_vkDevice;
		EShaderLangFormat		_vkVersion;
		Queues_t				_vkQueues;

		EQueueFamilyMask		_availableQueues;
		EResourceState			_graphicsShaderStages;
		VkPipelineStageFlagBits	_allWritableStages;
		VkPipelineStageFlagBits	_allReadableStages;
		
		EnabledFeatures			_features;
		
		VulkanDeviceFnTable		_deviceFnTable;
		
		DeviceProperties		_properties;
		ExtensionSet_t			_instanceExtensions;
		ExtensionSet_t			_deviceExtensions;


	// methods
	public:
		explicit VDevice (const VulkanDeviceInfo &vdi);
		~VDevice ();

		ND_ EResourceState					GetGraphicsShaderStages ()		const	{ return _graphicsShaderStages; }
		ND_ VkPipelineStageFlagBits			GetAllWritableStages ()			const	{ return _allWritableStages; }
		ND_ VkPipelineStageFlagBits			GetAllReadableStages ()			const	{ return _allReadableStages; }

		ND_ VkDevice						GetVkDevice ()					const	{ return _vkDevice; }
		ND_ VkPhysicalDevice				GetVkPhysicalDevice ()			const	{ return _vkPhysicalDevice; }
		ND_ VkInstance						GetVkInstance ()				const	{ return _vkInstance; }
		ND_ EShaderLangFormat				GetVkVersion ()					const	{ return _vkVersion; }
		ND_ ArrayView< VDeviceQueueInfo >	GetVkQueues ()					const	{ return _vkQueues; }
		ND_ EQueueFamilyMask				GetQueueFamilyMask ()			const	{ return _availableQueues; }
		
		ND_ EnabledFeatures const&			GetFeatures ()					const	{ return _features; }
		ND_ DeviceProperties const&			GetProperties ()				const	{ return _properties; }
		ND_ VkPhysicalDeviceLimits const&	GetDeviceLimits ()				const	{ return _properties.properties.limits; }

		// check extensions
		ND_ bool  HasInstanceExtension (StringView name) const;
		ND_ bool  HasDeviceExtension (StringView name) const;
		
		bool  SetObjectName (uint64_t id, NtStringView name, VkObjectType type) const;


	private:
		void _InitDeviceFeatures ();

		bool _LoadInstanceExtensions ();
		bool _LoadDeviceExtensions ();
	};


}	// FG

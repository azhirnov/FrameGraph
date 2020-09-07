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
			bool	commandPoolTrim			: 1;
			bool	dispatchBase			: 1;
			bool	array2DCompatible		: 1;
			bool	blockTexelView			: 1;
			// vulkan 1.2 core
			bool	samplerMirrorClamp		: 1;
			bool	descriptorIndexing		: 1;
			bool	renderPass2				: 1;
			bool	depthStencilResolve		: 1;
			bool	drawIndirectCount		: 1;
			// window extensions
			bool	surface					: 1;
			bool	surfaceCaps2			: 1;
			bool	swapchain				: 1;
			// extensions
			bool	debugUtils				: 1;
			bool	meshShaderNV			: 1;
			bool	rayTracingNV			: 1;
			bool	shadingRateImageNV		: 1;
			bool	robustness2				: 1;
		};

		struct DeviceProperties
		{
			VkPhysicalDeviceProperties							properties;
			VkPhysicalDeviceFeatures							features;
			VkPhysicalDeviceMemoryProperties					memoryProperties;
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
			#ifdef VK_KHR_depth_stencil_resolve
			VkPhysicalDeviceDepthStencilResolvePropertiesKHR	depthStencilResolve;
			#endif
			#ifdef VK_EXT_descriptor_indexing
			VkPhysicalDeviceDescriptorIndexingFeaturesEXT		descriptorIndexingFeatures;
			VkPhysicalDeviceDescriptorIndexingPropertiesEXT		descriptorIndexingProperties;
			#endif
			#ifdef VK_VERSION_1_2
			VkPhysicalDeviceVulkan11Properties					properties110;
			VkPhysicalDeviceVulkan12Properties					properties120;
			#endif
			#ifdef VK_EXT_robustness2
			VkPhysicalDeviceRobustness2FeaturesEXT				robustness2Features;
			VkPhysicalDeviceRobustness2PropertiesEXT			robustness2Properties;
			#endif
		};

		struct DeviceFlags
		{
			EQueueFamilyMask			availableQueues			= Zero;
			EResourceState				graphicsShaderStages	= Zero;
			VkPipelineStageFlagBits		allWritableStages		= Zero;
			VkPipelineStageFlagBits		allReadableStages		= Zero;
			VkImageCreateFlagBits		imageCreateFlags		= Zero;
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

		
		EnabledFeatures			_features;
		DeviceFlags				_flags;
		
		VulkanDeviceFnTable		_deviceFnTable;
		
		DeviceProperties		_properties;

		ExtensionSet_t			_instanceExtensions;
		ExtensionSet_t			_deviceExtensions;


	// methods
	public:
		explicit VDevice (const VulkanDeviceInfo &vdi);
		~VDevice ();

		ND_ EResourceState					GetGraphicsShaderStages ()		const	{ return _flags.graphicsShaderStages; }
		ND_ VkPipelineStageFlagBits			GetAllWritableStages ()			const	{ return _flags.allWritableStages; }
		ND_ VkPipelineStageFlagBits			GetAllReadableStages ()			const	{ return _flags.allReadableStages; }

		ND_ VkDevice						GetVkDevice ()					const	{ return _vkDevice; }
		ND_ VkPhysicalDevice				GetVkPhysicalDevice ()			const	{ return _vkPhysicalDevice; }
		ND_ VkInstance						GetVkInstance ()				const	{ return _vkInstance; }
		ND_ EShaderLangFormat				GetVkVersion ()					const	{ return _vkVersion; }
		ND_ ArrayView< VDeviceQueueInfo >	GetVkQueues ()					const	{ return _vkQueues; }
		ND_ EQueueFamilyMask				GetQueueFamilyMask ()			const	{ return _flags.availableQueues; }
		
		ND_ EnabledFeatures const&			GetFeatures ()					const	{ return _features; }
		ND_ DeviceProperties const&			GetProperties ()				const	{ return _properties; }
		ND_ VkPhysicalDeviceLimits const&	GetDeviceLimits ()				const	{ return _properties.properties.limits; }
		ND_ DeviceFlags const&				GetFlags ()						const	{ return _flags; }

		// check extensions
		ND_ bool  HasInstanceExtension (StringView name) const;
		ND_ bool  HasDeviceExtension (StringView name) const;
		
		bool  SetObjectName (uint64_t id, NtStringView name, VkObjectType type) const;


	private:
		void  _InitDeviceFeatures ();
		void  _InitDeviceFlags ();

		bool  _LoadInstanceExtensions ();
		bool  _LoadDeviceExtensions ();
	};


}	// FG

// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VDevice.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VDevice::VDevice (const VulkanDeviceInfo &vdi) :
		_vkInstance{ BitCast<VkInstance>( vdi.instance )},
		_vkPhysicalDevice{ BitCast<VkPhysicalDevice>( vdi.physicalDevice )},
		_vkDevice{ BitCast<VkDevice>( vdi.device )},
		_vkVersion{ EShaderLangFormat::Unknown },
		_availableQueues{ Default },
		_graphicsShaderStages{ EResourceState::_VertexShader | EResourceState::_FragmentShader },
		_allWritableStages{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT },
		_allReadableStages{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT }
	{
		VulkanDeviceFn_Init( &_deviceFnTable );

		std::memset( &_properties, 0, sizeof(_properties) );

		StaticArray<VkQueueFamilyProperties, 16>	queue_properties;
		uint										count = uint(queue_properties.size());
		vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, INOUT &count, null );	// to shutup validation warnings
		vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, INOUT &count, OUT queue_properties.data() );

		for (auto& src : vdi.queues)
		{
			auto&	props = queue_properties[src.familyIndex];

			VDeviceQueueInfo	dst = {};
			dst.handle		= BitCast<VkQueue>( src.handle );
			dst.familyFlags	= BitCast<VkQueueFlags>( src.familyFlags );
			dst.familyIndex	= EQueueFamily(src.familyIndex);
			dst.priority	= src.priority;
			dst.debugName	= src.debugName;
			dst.minImageTransferGranularity	= { props.minImageTransferGranularity.width,
												props.minImageTransferGranularity.height,
												props.minImageTransferGranularity.depth };
			_availableQueues |= dst.familyIndex;

			CHECK( dst.familyIndex < EQueueFamily::_Count );

			_vkQueues.push_back( std::move(dst) );
		}

		// TODO: return error ?
		CHECK( VulkanLoader::LoadInstance( _vkInstance ));
		CHECK( VulkanLoader::LoadDevice( _vkDevice, OUT _deviceFnTable ));

		vkGetPhysicalDeviceFeatures( _vkPhysicalDevice, OUT &_properties.features );
		vkGetPhysicalDeviceProperties( _vkPhysicalDevice, OUT &_properties.properties );
		vkGetPhysicalDeviceMemoryProperties( _vkPhysicalDevice, OUT &_properties.memoryProperties );

		CHECK( _LoadInstanceExtensions() );
		CHECK( _LoadDeviceExtensions() );

		_InitDeviceFeatures();

		// add shader stages
		if ( _properties.features.tessellationShader )
			_graphicsShaderStages |= (EResourceState::_TessControlShader | EResourceState::_TessEvaluationShader);

		if ( _properties.features.geometryShader )
			_graphicsShaderStages |= (EResourceState::_GeometryShader);

		if ( _features.meshShaderNV )
			_graphicsShaderStages |= (EResourceState::_MeshTaskShader | EResourceState::_MeshShader);
		
		#ifdef VK_NV_ray_tracing
		//if ( _features.rayTracingNV )
		//	_allReadableStages |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV;
		#endif

		auto&	limits = _properties.properties.limits;

		// validate constants
		CHECK( limits.maxPushConstantsSize >= FG_MaxPushConstantsSize );
		CHECK( limits.maxVertexInputAttributes >= FG_MaxVertexAttribs );
		CHECK( limits.maxVertexInputBindings >= FG_MaxVertexBuffers );
		CHECK( limits.maxViewports >= FG_MaxViewports );
		CHECK( limits.maxColorAttachments >= FG_MaxColorBuffers );
		CHECK( limits.maxBoundDescriptorSets >= FG_MaxDescriptorSets );
		CHECK( (limits.maxDescriptorSetUniformBuffersDynamic + limits.maxDescriptorSetStorageBuffersDynamic) >= FG_MaxBufferDynamicOffsets );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VDevice::~VDevice ()
	{
	}

/*
=================================================
	HasInstanceExtension
=================================================
*/
	bool  VDevice::HasInstanceExtension (StringView name) const
	{
		return !!_instanceExtensions.count( name );
	}

/*
=================================================
	HasDeviceExtension
=================================================
*/
	bool  VDevice::HasDeviceExtension (StringView name) const
	{
		return !!_deviceExtensions.count( name );
	}
	
/*
=================================================
	_InitDeviceFeatures
=================================================
*/
	void  VDevice::_InitDeviceFeatures ()
	{
		vkGetPhysicalDeviceFeatures( _vkPhysicalDevice, OUT &_properties.features );
		vkGetPhysicalDeviceProperties( _vkPhysicalDevice, OUT &_properties.properties );
		vkGetPhysicalDeviceMemoryProperties( _vkPhysicalDevice, OUT &_properties.memoryProperties );

		// get api version
		{
			uint	major = VK_VERSION_MAJOR( _properties.properties.apiVersion );
			uint	minor = VK_VERSION_MINOR( _properties.properties.apiVersion );

			if ( major == 1 )
			{
				if ( minor == 0 )	_vkVersion = EShaderLangFormat::Vulkan_100;	else
				if ( minor == 1 )	_vkVersion = EShaderLangFormat::Vulkan_110;	else
				if ( minor == 2 )	_vkVersion = EShaderLangFormat::Vulkan_120;
			}
			CHECK( _vkVersion != EShaderLangFormat::Unknown );
		}
		
		VulkanLoader::SetupInstanceBackwardCompatibility( _properties.properties.apiVersion );

		#ifdef VK_KHR_surface
		_features.surface					= HasInstanceExtension( VK_KHR_SURFACE_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_get_surface_capabilities2
		_features.surfaceCaps2				= HasInstanceExtension( VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_swapchain
		_features.swapchain					= HasDeviceExtension( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
		#endif
		#ifdef VK_EXT_debug_utils
		_features.debugUtils				= HasInstanceExtension( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
		#endif
		#if defined(VK_KHR_get_memory_requirements2) and defined(VK_KHR_bind_memory2)
		_features.bindMemory2				= _vkVersion >= EShaderLangFormat::Vulkan_110 or (HasDeviceExtension( VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME ) and HasDeviceExtension( VK_KHR_BIND_MEMORY_2_EXTENSION_NAME ));
		#endif
		#ifdef VK_KHR_dedicated_allocation
		_features.dedicatedAllocation		= _vkVersion >= EShaderLangFormat::Vulkan_110 or HasDeviceExtension( VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_descriptor_update_template
		_features.descriptorUpdateTemplate	= _vkVersion >= EShaderLangFormat::Vulkan_110 or HasDeviceExtension( VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_maintenance2
		_features.imageViewUsage			= _vkVersion >= EShaderLangFormat::Vulkan_110 or HasDeviceExtension( VK_KHR_MAINTENANCE2_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_maintenance1
		_features.create2DArrayCompatible	= _vkVersion >= EShaderLangFormat::Vulkan_110 or HasDeviceExtension( VK_KHR_MAINTENANCE1_EXTENSION_NAME );
		_features.commandPoolTrim			= _vkVersion >= EShaderLangFormat::Vulkan_110 or HasDeviceExtension( VK_KHR_MAINTENANCE1_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_device_group
		_features.dispatchBase				= _vkVersion >= EShaderLangFormat::Vulkan_110 or HasDeviceExtension( VK_KHR_DEVICE_GROUP_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_create_renderpass2
		_features.renderPass2				= _vkVersion >= EShaderLangFormat::Vulkan_120 or HasDeviceExtension( VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_sampler_mirror_clamp_to_edge
		_features.samplerMirrorClamp		= _vkVersion >= EShaderLangFormat::Vulkan_120 or HasDeviceExtension( VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_draw_indirect_count
		_features.drawIndirectCount			= _vkVersion >= EShaderLangFormat::Vulkan_120 or HasDeviceExtension( VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME );
		#endif
		#ifdef VK_EXT_descriptor_indexing
		_features.descriptorIndexing		= _vkVersion >= EShaderLangFormat::Vulkan_120 or HasDeviceExtension( VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME );
		#endif
		#ifdef VK_NV_mesh_shader
		_features.meshShaderNV				= HasDeviceExtension( VK_NV_MESH_SHADER_EXTENSION_NAME );
		#endif
		#ifdef VK_NV_ray_tracing
		_features.rayTracingNV				= HasDeviceExtension( VK_NV_RAY_TRACING_EXTENSION_NAME );
		#endif
		#ifdef VK_NV_shading_rate_image
		_features.shadingRateImageNV		= HasDeviceExtension( VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_shader_clock
		_features.shaderClock				= HasDeviceExtension( VK_KHR_SHADER_CLOCK_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_shader_float16_int8
		_features.float16Arithmetic			= _vkVersion >= EShaderLangFormat::Vulkan_120 or HasDeviceExtension( VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_timeline_semaphore
		_features.timelineSemaphore			= _vkVersion >= EShaderLangFormat::Vulkan_120 or HasDeviceExtension( VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME ),
		#endif
		#ifdef VK_KHR_buffer_device_address
		_features.bufferAddress				= _vkVersion >= EShaderLangFormat::Vulkan_120 or HasDeviceExtension( VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME ),
		#endif
		#ifdef VK_KHR_depth_stencil_resolve
		_features.depthStencilResolve		= _vkVersion >= EShaderLangFormat::Vulkan_120 or HasDeviceExtension( VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_shader_atomic_int64
		_features.shaderAtomicInt64			= _vkVersion >= EShaderLangFormat::Vulkan_120 or HasDeviceExtension( VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_spirv_1_4
		_features.spirv14					= _vkVersion >= EShaderLangFormat::Vulkan_120 or HasDeviceExtension( VK_KHR_SPIRV_1_4_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_push_descriptor
		_features.pushDescriptor			= HasDeviceExtension( VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_vulkan_memory_model
		_features.memoryModel				= _vkVersion >= EShaderLangFormat::Vulkan_120 or HasDeviceExtension( VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME );
		#endif

		// load extensions
		if ( _vkVersion >= EShaderLangFormat::Vulkan_110 or HasInstanceExtension( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ))
		{
			VkPhysicalDeviceFeatures2	feat2		= {};
			void **						next_feat	= &feat2.pNext;
			feat2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			
			#ifdef VK_NV_mesh_shader
			if ( _features.meshShaderNV )
			{
				*next_feat	= &_properties.meshShaderFeatures;
				next_feat	= &_properties.meshShaderFeatures.pNext;
				_properties.meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
			}
			#endif
			#ifdef VK_NV_shading_rate_image
			if ( _features.shadingRateImageNV )
			{
				*next_feat	= &_properties.shadingRateImageFeatures;
				next_feat	= &_properties.shadingRateImageFeatures.pNext;
				_properties.shadingRateImageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV;
			}
			#endif
			#ifdef VK_KHR_shader_clock
			if ( _features.shaderClock )
			{
				*next_feat	= &_properties.shaderClock;
				next_feat	= &_properties.shaderClock.pNext;
				_properties.shaderClock.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR;
			}
			#endif
			#ifdef VK_KHR_shader_float16_int8
			VkPhysicalDeviceShaderFloat16Int8FeaturesKHR	shader_float16_int8_feat = {};
			if ( _features.float16Arithmetic )
			{
				*next_feat	= &shader_float16_int8_feat;
				next_feat	= &shader_float16_int8_feat.pNext;
				shader_float16_int8_feat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR;
			}
			#endif
			#ifdef VK_KHR_timeline_semaphore
			VkPhysicalDeviceTimelineSemaphoreFeaturesKHR	timeline_sem_feat = {};
			if ( _features.timelineSemaphore )
			{
				*next_feat	= &timeline_sem_feat;
				next_feat	= &timeline_sem_feat.pNext;
				timeline_sem_feat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;
			}
			#endif
			#ifdef VK_KHR_buffer_device_address
			if ( _features.bufferAddress )
			{
				*next_feat	= &_properties.bufferDeviceAddress;
				next_feat	= &_properties.bufferDeviceAddress.pNext;
				_properties.bufferDeviceAddress.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
			}
			#endif
			#ifdef VK_KHR_shader_atomic_int64
			if ( _features.shaderAtomicInt64 )
			{
				*next_feat	= &_properties.shaderAtomicInt64;
				next_feat	= &_properties.shaderAtomicInt64.pNext;
				_properties.shaderAtomicInt64.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES_KHR;
			}
			#endif
			#ifdef VK_KHR_vulkan_memory_model
			if ( _features.memoryModel )
			{
				*next_feat	= &_properties.memoryModel;
				next_feat	= &_properties.memoryModel.pNext;
				_properties.memoryModel.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES_KHR;
			}
			#endif
			#ifdef VK_EXT_descriptor_indexing
			if ( _features.descriptorIndexing )
			{
				*next_feat	= &_properties.descriptorIndexingFeatures;
				next_feat	= &_properties.descriptorIndexingFeatures.pNext;
				_properties.descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
			}
			#endif
			Unused( next_feat );

			vkGetPhysicalDeviceFeatures2KHR( GetVkPhysicalDevice(), OUT &feat2 );
			
			#ifdef VK_NV_mesh_shader
			_features.meshShaderNV			&= (_properties.meshShaderFeatures.meshShader or _properties.meshShaderFeatures.taskShader);
			#endif
			#ifdef VK_NV_shading_rate_image
			_features.shadingRateImageNV	&= (_properties.shadingRateImageFeatures.shadingRateImage == VK_TRUE);
			#endif
			#ifdef VK_KHR_shader_clock
			_features.shaderClock			&= !!(_properties.shaderClock.shaderDeviceClock | _properties.shaderClock.shaderSubgroupClock);
			#endif
			#ifdef VK_KHR_shader_float16_int8
			_features.float16Arithmetic		&= (shader_float16_int8_feat.shaderFloat16 == VK_TRUE);
			#endif
			#ifdef VK_KHR_timeline_semaphore
			_features.timelineSemaphore		&= (timeline_sem_feat.timelineSemaphore == VK_TRUE);
			#endif
			#ifdef VK_KHR_buffer_device_address
			_features.bufferAddress			&= (_properties.bufferDeviceAddress.bufferDeviceAddress == VK_TRUE);
			#endif
			#ifdef VK_KHR_shader_atomic_int64
			_features.shaderAtomicInt64		&= (_properties.shaderAtomicInt64.shaderBufferInt64Atomics == VK_TRUE);
			#endif
			#ifdef VK_KHR_vulkan_memory_model
			_features.memoryModel			&= (_properties.memoryModel.vulkanMemoryModel == VK_TRUE);
			#endif

			VkPhysicalDeviceProperties2	props2		= {};
			void **						next_props	= &props2.pNext;
			props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			
			#ifdef VK_NV_mesh_shader
			if ( _features.meshShaderNV )
			{
				*next_props	= &_properties.meshShaderProperties;
				next_props	= &_properties.meshShaderProperties.pNext;
				_properties.meshShaderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
			}
			#endif
			#ifdef VK_NV_shading_rate_image
			if ( _features.shadingRateImageNV )
			{
				*next_props	= &_properties.shadingRateImageProperties;
				next_props	= &_properties.shadingRateImageProperties.pNext;
				_properties.shadingRateImageProperties.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV;
			}
			#endif
			#ifdef VK_NV_ray_tracing
			if ( _features.rayTracingNV )
			{
				*next_props	= &_properties.rayTracingProperties;
				next_props	= &_properties.rayTracingProperties.pNext;
				_properties.rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
			}
			#endif
			#ifdef VK_KHR_timeline_semaphore
			if ( _features.timelineSemaphore )
			{
				*next_props	= &_properties.timelineSemaphoreProps;
				next_props	= &_properties.timelineSemaphoreProps.pNext;
				_properties.timelineSemaphoreProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES_KHR;
			}
			#endif
			#ifdef VK_KHR_depth_stencil_resolve
			if ( _features.depthStencilResolve )
			{
				*next_props	= &_properties.depthStencilResolve;
				next_props	= &_properties.depthStencilResolve.pNext;
				_properties.depthStencilResolve.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES_KHR;
			}
			#endif
			#ifdef VK_VERSION_1_2
			if ( _vkVersion >= EShaderLangFormat::Vulkan_120 )
			{
				*next_props	= &_properties.properties120;
				next_props	= &_properties.properties120.pNext;
				_properties.properties120.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;

				*next_props	= &_properties.properties110;
				next_props	= &_properties.properties110.pNext;
				_properties.properties110.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
			}
			#endif
			#ifdef VK_VERSION_1_1
			if ( _vkVersion >= EShaderLangFormat::Vulkan_110 )
			{
				*next_props	= &_properties.subgroup;
				next_props	= &_properties.subgroup.pNext;
				_properties.subgroup.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
			}
			#endif
			#ifdef VK_EXT_descriptor_indexing
			if ( _features.descriptorIndexing )
			{
				*next_props	= &_properties.descriptorIndexingProperties;
				next_props	= &_properties.descriptorIndexingProperties.pNext;
				_properties.descriptorIndexingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT;
			}
			#endif
			Unused( next_props );

			vkGetPhysicalDeviceProperties2KHR( GetVkPhysicalDevice(), OUT &props2 );
		}

		VulkanLoader::SetupDeviceBackwardCompatibility( _properties.properties.apiVersion, INOUT _deviceFnTable );
	}

/*
=================================================
	_LoadInstanceExtensions
=================================================
*/
	bool  VDevice::_LoadInstanceExtensions ()
	{
		if ( not _instanceExtensions.empty() )
			return true;
		
		uint	count = 0;
		VK_CALL( vkEnumerateInstanceExtensionProperties( null, OUT &count, null ));

		if ( count == 0 )
			return true;

		Array< VkExtensionProperties >	inst_ext;
		inst_ext.resize( count );

		VK_CALL( vkEnumerateInstanceExtensionProperties( null, OUT &count, OUT inst_ext.data() ));

		for (auto& ext : inst_ext) {
			_instanceExtensions.insert( ExtensionName_t(ext.extensionName) );
		}
		return true;
	}
	
/*
=================================================
	_LoadDeviceExtensions
=================================================
*/
	bool  VDevice::_LoadDeviceExtensions ()
	{
		CHECK_ERR( _vkPhysicalDevice );

		if ( not _deviceExtensions.empty() )
			return true;

		uint	count = 0;
		VK_CALL( vkEnumerateDeviceExtensionProperties( _vkPhysicalDevice, null, OUT &count, null ));

		if ( count == 0 )
			return true;

		Array< VkExtensionProperties >	dev_ext;
		dev_ext.resize( count );

		VK_CHECK( vkEnumerateDeviceExtensionProperties( _vkPhysicalDevice, null, OUT &count, OUT dev_ext.data() ));

		for (auto& ext : dev_ext) {
			_deviceExtensions.insert( ExtensionName_t(ext.extensionName) );
		}

		return true;
	}

/*
=================================================
	SetObjectName
=================================================
*/
	bool  VDevice::SetObjectName (uint64_t id, NtStringView name, VkObjectType type) const
	{
		if ( name.empty() or id == VK_NULL_HANDLE )
			return false;

		if ( _features.debugUtils )
		{
			VkDebugUtilsObjectNameInfoEXT	info = {};
			info.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			info.objectType		= type;
			info.objectHandle	= id;
			info.pObjectName	= name.c_str();

			VK_CALL( vkSetDebugUtilsObjectNameEXT( _vkDevice, &info ));
			return true;
		}

		return false;
	}

}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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

		memset( &_deviceInfo, 0, sizeof(_deviceInfo) );

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

		VulkanLoader::LoadInstance( _vkInstance );
		VulkanLoader::LoadDevice( _vkDevice, OUT _deviceFnTable );

		vkGetPhysicalDeviceFeatures( _vkPhysicalDevice, OUT &_deviceInfo.features );
		vkGetPhysicalDeviceProperties( _vkPhysicalDevice, OUT &_deviceInfo.properties );
		vkGetPhysicalDeviceMemoryProperties( _vkPhysicalDevice, OUT &_deviceInfo.memoryProperties );

		// get api version
		{
			uint	major = VK_VERSION_MAJOR( GetDeviceProperties().apiVersion );
			uint	minor = VK_VERSION_MINOR( GetDeviceProperties().apiVersion );

			if ( major == 1 )
			{
				if ( minor == 0 )	_vkVersion = EShaderLangFormat::Vulkan_100;	else
				if ( minor == 1 )	_vkVersion = EShaderLangFormat::Vulkan_110;	else
				if ( minor == 2 )	_vkVersion = EShaderLangFormat::Vulkan_120;
			}
			CHECK( _vkVersion != EShaderLangFormat::Unknown );
		}

		CHECK( _LoadInstanceExtensions() );
		CHECK( _LoadDeviceExtensions() );

		_enableDebugUtils			= HasExtension( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
		_enableMeshShaderNV			= HasDeviceExtension( VK_NV_MESH_SHADER_EXTENSION_NAME );
		_enableRayTracingNV			= HasDeviceExtension( VK_NV_RAY_TRACING_EXTENSION_NAME );
		_enableShadingRateImageNV	= HasDeviceExtension( VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME );
		_samplerMirrorClamp			= HasDeviceExtension( VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME );

		// load extensions
		if ( _vkVersion >= EShaderLangFormat::Vulkan_110 )
		{
			VkPhysicalDeviceFeatures2	feat2		= {};
			void **						next_feat	= &feat2.pNext;
			feat2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			
			if ( _enableMeshShaderNV )
			{
				*next_feat	= &_deviceInfo.meshShaderFeatures;
				next_feat	= &_deviceInfo.meshShaderFeatures.pNext;
				_deviceInfo.meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
			}
			if ( _enableShadingRateImageNV )
			{
				*next_feat	= &_deviceInfo.shadingRateImageFeatures;
				next_feat	= &_deviceInfo.shadingRateImageFeatures.pNext;
				_deviceInfo.shadingRateImageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV;
			}
			vkGetPhysicalDeviceFeatures2( GetVkPhysicalDevice(), &feat2 );

			_enableMeshShaderNV			= (_deviceInfo.meshShaderFeatures.meshShader or _deviceInfo.meshShaderFeatures.taskShader);
			_enableShadingRateImageNV	= _deviceInfo.shadingRateImageFeatures.shadingRateImage;


			VkPhysicalDeviceProperties2	props2		= {};
			void **						next_props	= &props2.pNext;
			props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

			if ( _enableMeshShaderNV )
			{
				*next_props	= &_deviceInfo.meshShaderProperties;
				next_props	= &_deviceInfo.meshShaderProperties.pNext;
				_deviceInfo.meshShaderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
			}
			if ( _enableShadingRateImageNV )
			{
				*next_props	= &_deviceInfo.shadingRateImageProperties;
				next_props	= &_deviceInfo.shadingRateImageProperties.pNext;
				_deviceInfo.shadingRateImageProperties.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV;
			}
			if ( _enableRayTracingNV )
			{
				*next_props	= &_deviceInfo.rayTracingProperties;
				next_props	= &_deviceInfo.rayTracingProperties.pNext;
				_deviceInfo.rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
			}
			vkGetPhysicalDeviceProperties2( GetVkPhysicalDevice(), &props2 );

			// TODO: check if extensions enebaled
		}

		// add shader stages
		if ( _deviceInfo.features.tessellationShader )
			_graphicsShaderStages |= (EResourceState::_TessControlShader | EResourceState::_TessEvaluationShader);

		if ( _deviceInfo.features.geometryShader )
			_graphicsShaderStages |= (EResourceState::_GeometryShader);

		if ( _enableMeshShaderNV )
			_graphicsShaderStages |= (EResourceState::_MeshTaskShader | EResourceState::_MeshShader);

		//if ( _enableRayTracingNV )
		//	_allReadableStages |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV;


		// validate constants
		CHECK( GetDeviceLimits().maxPushConstantsSize >= FG_MaxPushConstantsSize );
		CHECK( GetDeviceLimits().maxVertexInputAttributes >= FG_MaxVertexAttribs );
		CHECK( GetDeviceLimits().maxVertexInputBindings >= FG_MaxVertexBuffers );
		CHECK( GetDeviceLimits().maxViewports >= FG_MaxViewports );
		CHECK( GetDeviceLimits().maxColorAttachments >= FG_MaxColorBuffers );
		CHECK( GetDeviceLimits().maxBoundDescriptorSets >= FG_MaxDescriptorSets );
		CHECK( (GetDeviceLimits().maxDescriptorSetUniformBuffersDynamic + GetDeviceLimits().maxDescriptorSetStorageBuffersDynamic) >= FG_MaxBufferDynamicOffsets );
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
	HasExtension
=================================================
*/
	bool VDevice::HasExtension (StringView name) const
	{
		return !!_instanceExtensions.count( name );
	}

/*
=================================================
	HasDeviceExtension
=================================================
*/
	bool VDevice::HasDeviceExtension (StringView name) const
	{
		return !!_deviceExtensions.count( name );
	}
	
/*
=================================================
	_LoadInstanceExtensions
=================================================
*/
	bool VDevice::_LoadInstanceExtensions ()
	{
		if ( not _instanceExtensions.empty() )
			return true;
		
		uint	count = 0;
		VK_CALL( vkEnumerateInstanceExtensionProperties( null, OUT &count, null ) );

		if ( count == 0 )
			return true;

		Array< VkExtensionProperties >	inst_ext;
		inst_ext.resize( count );

		VK_CALL( vkEnumerateInstanceExtensionProperties( null, OUT &count, OUT inst_ext.data() ) );

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
	bool VDevice::_LoadDeviceExtensions ()
	{
		CHECK_ERR( _vkPhysicalDevice );

		if ( not _deviceExtensions.empty() )
			return true;

		uint	count = 0;
		VK_CALL( vkEnumerateDeviceExtensionProperties( _vkPhysicalDevice, null, OUT &count, null ) );

		if ( count == 0 )
			return true;

		Array< VkExtensionProperties >	dev_ext;
		dev_ext.resize( count );

		VK_CHECK( vkEnumerateDeviceExtensionProperties( _vkPhysicalDevice, null, OUT &count, OUT dev_ext.data() ) );

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
	bool VDevice::SetObjectName (uint64_t id, NtStringView name, VkObjectType type) const
	{
		if ( name.empty() or id == VK_NULL_HANDLE )
			return false;

		if ( _enableDebugUtils )
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

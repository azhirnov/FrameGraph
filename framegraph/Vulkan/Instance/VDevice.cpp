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
		_vkVersion{ EShaderLangFormat::Unknown }
	{
		VulkanDeviceFn_Init( &_deviceFnTable );

		StaticArray<VkQueueFamilyProperties, 16>	queue_properties;
		uint										count = uint(queue_properties.size());
		vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, INOUT &count, null );	// to shutup validation warnings
		vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, INOUT &count, OUT queue_properties.data() );

		for (auto& src : vdi.queues)
		{
			auto&	props = queue_properties[src.familyIndex];

			VDeviceQueueInfo	dst = {};
			dst.handle		= BitCast<VkQueue>( src.id );
			dst.familyFlags	= BitCast<VkQueueFlags>( src.familyFlags );
			dst.familyIndex	= EQueueFamily(src.familyIndex);
			dst.priority	= src.priority;
			dst.debugName	= src.debugName;
			dst.minImageTransferGranularity	= { props.minImageTransferGranularity.width,
												props.minImageTransferGranularity.height,
												props.minImageTransferGranularity.depth };

			CHECK( dst.familyIndex < EQueueFamily::_Count );

			_vkQueues.push_back( std::move(dst) );
		}

		VulkanLoader::LoadInstance( _vkInstance );
		VulkanLoader::LoadDevice( _vkDevice, OUT _deviceFnTable );

		vkGetPhysicalDeviceFeatures( _vkPhysicalDevice, OUT &_deviceFeatures );
		vkGetPhysicalDeviceProperties( _vkPhysicalDevice, OUT &_deviceProperties );
		vkGetPhysicalDeviceMemoryProperties( _vkPhysicalDevice, OUT &_deviceMemoryProperties );

		// get api version
		{
			uint	major = VK_VERSION_MAJOR( _deviceProperties.apiVersion );
			uint	minor = VK_VERSION_MINOR( _deviceProperties.apiVersion );

			if ( major == 1 )
			{
				if ( minor == 0 )	_vkVersion = EShaderLangFormat::Vulkan_100;	else
				if ( minor == 1 )	_vkVersion = EShaderLangFormat::Vulkan_110;
			}
			CHECK( _vkVersion != EShaderLangFormat::Unknown );
		}

		CHECK( _LoadInstanceExtensions() );
		CHECK( _LoadDeviceExtensions() );

		_enableDebugUtils	= HasExtension( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
		_enableMeshShaderNV	= HasDeviceExtension( VK_NV_MESH_SHADER_EXTENSION_NAME );
		_enableRayTracingNV	= HasDeviceExtension( VK_NV_RAY_TRACING_EXTENSION_NAME );

		// load extensions
		if ( _vkVersion >= EShaderLangFormat::Vulkan_110 )
		{
			void **						next_props	= null;
			VkPhysicalDeviceProperties2	props2		= {};
			props2.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			next_props		= &props2.pNext;

			_deviceMeshShaderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
			_deviceRayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
			_deviceShadingRateImageProperties.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV;
			
			if ( _enableMeshShaderNV )
			{
				*next_props	= &_deviceMeshShaderProperties;
				next_props	= &_deviceMeshShaderProperties.pNext;
			}

			*next_props	= &_deviceShadingRateImageProperties;
			next_props	= &_deviceShadingRateImageProperties.pNext;
			
			if ( _enableRayTracingNV )
			{
				*next_props	= &_deviceRayTracingProperties;
				next_props	= &_deviceRayTracingProperties.pNext;
			}

			vkGetPhysicalDeviceProperties2( GetVkPhysicalDevice(), &props2 );
		}


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
	bool VDevice::_LoadInstanceExtensions () const
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
	bool VDevice::_LoadDeviceExtensions () const
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
	bool VDevice::SetObjectName (uint64_t id, StringView name, VkObjectType type) const
	{
		if ( name.empty() or id == VK_NULL_HANDLE )
			return false;

		if ( _enableDebugUtils )
		{
			VkDebugUtilsObjectNameInfoEXT	info = {};
			info.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			info.objectType		= type;
			info.objectHandle	= id;
			info.pObjectName	= name.data();

			VK_CALL( vkSetDebugUtilsObjectNameEXT( _vkDevice, &info ));
			return true;
		}

		return false;
	}


}	// FG

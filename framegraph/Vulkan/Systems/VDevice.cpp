// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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

		for (auto& src : vdi.queues)
		{
			VDeviceQueueInfo	dst = {};
			dst.id			= BitCast<VkQueue>( src.id );
			dst.familyFlags	= BitCast<VkQueueFlags>( src.familyFlags );
			dst.familyIndex	= src.familyIndex;
			dst.priority	= src.priority;
			dst.debugName	= src.debugName;

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

		_enableDebugMarkers = HasExtension( VK_EXT_DEBUG_MARKER_EXTENSION_NAME );

		CHECK( _LoadInstanceLayers() );
		CHECK( _LoadInstanceExtensions() );
		CHECK( _LoadDeviceExtensions() );


		// validate constants
		CHECK( GetDeviceLimits().maxPushConstantsSize >= FG_MaxPushConstantsSize );
		CHECK( GetDeviceLimits().maxVertexInputAttributes >= FG_MaxAttribs );
		CHECK( GetDeviceLimits().maxVertexInputBindings >= FG_MaxVertexBuffers );
		CHECK( GetDeviceLimits().maxViewports >= FG_MaxViewports );
		CHECK( GetDeviceLimits().maxColorAttachments >= FG_MaxColorBuffers );
		CHECK( GetDeviceLimits().maxBoundDescriptorSets >= FG_MaxDescriptorSets );
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
	HasLayer
=================================================
*/
	bool VDevice::HasLayer (StringView name) const
	{
		// TODO: optimize search
		for (auto& layer : _instanceLayers)
		{
			if ( name == layer.layerName )
				return true;
		}
		return false;
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
	_LoadInstanceLayers
=================================================
*/
	bool VDevice::_LoadInstanceLayers () const
	{
		if ( not _instanceLayers.empty() )
			return true;
		
		uint	count = 0;
		VK_CALL( vkEnumerateInstanceLayerProperties( OUT &count, null ) );

		if ( count == 0 )
			return true;

		_instanceLayers.resize( count );
		VK_CALL( vkEnumerateInstanceLayerProperties( OUT &count, OUT _instanceLayers.data() ) );

		return true;
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
	bool VDevice::SetObjectName (uint64_t id, StringView name, VkDebugReportObjectTypeEXT type) const
	{
		if ( name.empty() or id == VK_NULL_HANDLE or not _enableDebugMarkers )
			return false;

		VkDebugMarkerObjectNameInfoEXT	info = {};
		info.sType			= VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		info.objectType		= type;
		info.object			= id;
		info.pObjectName	= name.data();

		VK_CALL( vkDebugMarkerSetObjectNameEXT( _vkDevice, &info ) );
		return true;
	}


}	// FG

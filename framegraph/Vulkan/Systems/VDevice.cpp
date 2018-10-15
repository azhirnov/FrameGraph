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
        _vkSurface{ BitCast<VkSurfaceKHR>( vdi.surface )},
		_vkVersion{ EShaderLangFormat::Unknown }
	{
		VulkanDeviceFn_Init( &_deviceFnTable );

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
		CHECK( _perFrame.empty() );
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
	DeleteImage
=================================================
*/
	void VDevice::DeleteImage (VkImage image) const
	{
		if ( image != VK_NULL_HANDLE )
			_perFrame[_currFrame].readyToDelete.images.push_back( image );
	}
	
/*
=================================================
	DeleteImageView
=================================================
*/
	void VDevice::DeleteImageView (VkImageView view) const
	{
		if ( view != VK_NULL_HANDLE )
			_perFrame[_currFrame].readyToDelete.imageViews.push_back( view );
	}
	
/*
=================================================
	DeleteBuffer
=================================================
*/
	void VDevice::DeleteBuffer (VkBuffer buf) const
	{
		if ( buf != VK_NULL_HANDLE )
			_perFrame[_currFrame].readyToDelete.buffers.push_back( buf );
	}
	
/*
=================================================
	DeletePipeline
=================================================
*/
	void VDevice::DeletePipeline (VkPipeline ppln) const
	{
		if ( ppln != VK_NULL_HANDLE )
			_perFrame[_currFrame].readyToDelete.pipelines.push_back( ppln );
	}

/*
=================================================
	FreeExternalBuffer
=================================================
*/
	void VDevice::FreeExternalBuffer (VkBuffer buf) const
	{
		// TODO
		ASSERT(false);
	}
	
/*
=================================================
	FreeExternalImage
=================================================
*/
	void VDevice::FreeExternalImage (VkImage image) const
	{
		// TODO
		ASSERT(false);
	}
	
/*
=================================================
	_Initialize
=================================================
*/
	void VDevice::_Initialize (uint swapchainLength)
	{
		ASSERT( _perFrame.empty() );

		_perFrame.resize( swapchainLength );
		_currFrame = 0;
	}
	
/*
=================================================
	_Deinitialize
=================================================
*/
	void VDevice::_Deinitialize ()
	{
		for (auto& frame : _perFrame) {
			_DeleteResources( frame );
		}
		_perFrame.clear();
	}

/*
=================================================
	_OnBeginFrame
=================================================
*/
	void VDevice::_OnBeginFrame (uint frameIdx)
	{
		_currFrame = frameIdx;

		auto&	frame = _perFrame[_currFrame];

		_DeleteResources( frame );
	}
	
/*
=================================================
	_DeleteResources
=================================================
*/
	void VDevice::_DeleteResources (INOUT PerFrame &frame) const
	{
		for (auto& buf_id : frame.readyToDelete.buffers) {
			vkDestroyBuffer( _vkDevice, buf_id, null );
		}
		for (auto& view_id : frame.readyToDelete.imageViews) {
			vkDestroyImageView( _vkDevice, view_id, null );
		}
		for (auto& img_id : frame.readyToDelete.images) {
			vkDestroyImage( _vkDevice, img_id, null );
		}
		for (auto& ppln_id : frame.readyToDelete.pipelines) {
			vkDestroyPipeline( _vkDevice, ppln_id, null );
		}

		frame.readyToDelete.buffers.clear();
		frame.readyToDelete.images.clear();
		frame.readyToDelete.imageViews.clear();
		frame.readyToDelete.pipelines.clear();
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

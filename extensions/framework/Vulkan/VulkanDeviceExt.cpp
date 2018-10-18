// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VulkanDeviceExt.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Algorithms/EnumUtils.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VulkanDeviceExt::VulkanDeviceExt () :
		_debugCallback{ VK_NULL_HANDLE },
		_debugReportSupported{ false },
		_debugMarkersSupported{ false },
		_breakOnValidationError{ true }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VulkanDeviceExt::~VulkanDeviceExt ()
	{
	}
		
/*
=================================================
	_OnInstanceCreated
=================================================
*/
	void VulkanDeviceExt::_OnInstanceCreated (Array<const char*> &&, Array<const char*> &&instanceExtensions)
	{
		for (auto& ext : instanceExtensions) {
			_instanceExtensions.insert( ext );
		}

		_debugReportSupported = HasInstanceExtension( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
	}

/*
=================================================
	_OnLogicalDeviceCreated
=================================================
*/
	void VulkanDeviceExt::_OnLogicalDeviceCreated (Array<const char *> &&extensions)
	{
		for (auto& ext : extensions) {
			_deviceExtensions.insert( ext );
		}

		_debugMarkersSupported = HasDeviceExtension( VK_EXT_DEBUG_MARKER_EXTENSION_NAME );

		vkGetPhysicalDeviceProperties( GetVkPhysicalDevice(), OUT &_properties.main );
		vkGetPhysicalDeviceMemoryProperties( GetVkPhysicalDevice(), OUT &_deviceMemoryProperties );
		
		if ( VK_VERSION_MAJOR( _properties.main.apiVersion ) == 1 and
			 VK_VERSION_MINOR( _properties.main.apiVersion ) >  0 )
		{
			void **						next_props	= null;
			VkPhysicalDeviceProperties2	props2		= {};

			props2.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			next_props		= &props2.pNext;

			*next_props					= &_properties.deviceID;
			next_props					= &_properties.deviceID.pNext;
			_properties.deviceID.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
			
			*next_props					= &_properties.subgroup;
			next_props					= &_properties.subgroup.pNext;
			_properties.subgroup.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
			
			*next_props						= &_properties.maintenance3;
			next_props						= &_properties.maintenance3.pNext;
			_properties.maintenance3.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
			
			*next_props						= &_properties.meshShader;
			next_props						= &_properties.meshShader.pNext;
			_properties.meshShader.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
			
			*next_props							= &_properties.shadingRateImage;
			next_props							= &_properties.shadingRateImage.pNext;
			_properties.shadingRateImage.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV;
			
			*next_props						= &_properties.rayTracing;
			next_props						= &_properties.rayTracing.pNext;
			_properties.rayTracing.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAYTRACING_PROPERTIES_NVX;

			

			vkGetPhysicalDeviceProperties2( GetVkPhysicalDevice(), &props2 );
		}

		_WriteDeviceInfo();
	}

/*
=================================================
	_BeforeDestroy
=================================================
*/
	void VulkanDeviceExt::_BeforeDestroy ()
	{
		if ( _debugCallback ) {
			vkDestroyDebugReportCallbackEXT( GetVkInstance(), _debugCallback, null );
		}

		_instanceExtensions.clear();
		_deviceExtensions.clear();

		_debugCallback			= VK_NULL_HANDLE;
		_debugReportSupported	= false;
		_callback				= DebugReport_t();
		_debugMarkersSupported	= false;
	}
	
/*
=================================================
	_AfterDestroy
=================================================
*/
	void VulkanDeviceExt::_AfterDestroy ()
	{
	}
	
/*
=================================================
	_WriteDeviceInfo
=================================================
*/
	void VulkanDeviceExt::_WriteDeviceInfo ()
	{
		FG_LOGI( "apiVersion:  "s << ToString(VK_VERSION_MAJOR( GetDeviceProperties().apiVersion )) << '.' <<
				 ToString(VK_VERSION_MINOR( GetDeviceProperties().apiVersion )) << ' ' <<
				 ToString(VK_VERSION_PATCH( GetDeviceProperties().apiVersion )) );
		
		FG_LOGI( "vendorName:  "s << _GetVendorNameByID( GetDeviceProperties().vendorID ) );
		FG_LOGI( "deviceName:  "s << GetDeviceProperties().deviceName );

	}

/*
=================================================
	DebugReportObjectTypeToString
=================================================
*/
	static String  DebugReportObjectTypeToString (VkDebugReportObjectTypeEXT objType)
	{
		ENABLE_ENUM_CHECKS();
		switch ( objType )
		{
			case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT			: return "Instance";
			case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT	: return "PhysicalDevice";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT				: return "Device";
			case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT				: return "Queue";
			case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT			: return "Semaphore";
			case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT		: return "CommandBuffer";
			case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT				: return "Fence";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT		: return "DeviceMemory";
			case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT				: return "Buffer";
			case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT				: return "Image";
			case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT				: return "Event";
			case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT			: return "QueryPool";
			case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT		: return "BufferView";
			case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT			: return "ImageView";
			case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT		: return "ShaderModule";
			case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT		: return "PipelineCache";
			case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT	: return "PipelineLayout";
			case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT		: return "RenderPass";
			case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT			: return "Pipeline";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT	: return "DescriptorSetLayout";
			case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT			: return "Sampler";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT	: return "DescriptorPool";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT		: return "DescriptorSet";
			case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT		: return "Framebuffer";
			case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT		: return "CommandPool";
			case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT		: return "Surface";
			case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT		: return "Swapchain";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT		: return "DebugReport";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT		: return "Display";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT	: return "DisplayMode";
			case VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT	: return "ObjectTableNvx";
			case VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT : return "IndirectCommandsLayoutNvx";
			case VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_NVX_EXT :	return "AccelerationStructureNvx";	

			case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT :
			case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT :
			case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT :
			case VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT :
			case VK_DEBUG_REPORT_OBJECT_TYPE_RANGE_SIZE_EXT :
			case VK_DEBUG_REPORT_OBJECT_TYPE_MAX_ENUM_EXT :
				break;	// used to shutup compilation warnings
		}
		DISABLE_ENUM_CHECKS();

		CHECK(	objType >= VK_DEBUG_REPORT_OBJECT_TYPE_BEGIN_RANGE_EXT and
				objType <= VK_DEBUG_REPORT_OBJECT_TYPE_END_RANGE_EXT );
		return "unknown";
	}

/*
=================================================
	_DebugReportCallback
=================================================
*/
	VkBool32 VKAPI_CALL VulkanDeviceExt::_DebugReportCallback (VkDebugReportFlagsEXT flags,
																VkDebugReportObjectTypeEXT objectType,
																uint64_t object,
																size_t /*location*/,
																int32_t /*messageCode*/,
																const char* pLayerPrefix,
																const char* pMessage,
																void* pUserData)
	{
		static_cast<VulkanDeviceExt *>(pUserData)->_DebugReport( flags, DebugReportObjectTypeToString(objectType), object, pLayerPrefix, pMessage );
		return VK_FALSE;
	}
	
/*
=================================================
	_DebugReport
=================================================
*/
	void VulkanDeviceExt::_DebugReport (VkDebugReportFlagsEXT flags, StringView objectType, uint64_t object, StringView layerPrefix, StringView message) const
	{
		if ( _callback )
			return _callback({ flags, objectType, object, layerPrefix, message });
		

		if ( _breakOnValidationError and !!(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) )
		{
			FG_LOGE( "Error in object '"s << objectType << "' (" << ToString(object) << "), layer: " << layerPrefix << ", message: " << message );
		}
		else
		{
			FG_LOGI( "Info message in object '"s << objectType << "' (" << ToString(object) << "), layer: " << layerPrefix << ", message: " << message );
		}
	}

/*
=================================================
	CreateDebugCallback
=================================================
*/
	bool VulkanDeviceExt::CreateDebugCallback (VkDebugReportFlagsEXT flags, DebugReport_t &&callback)
	{
		CHECK_ERR( GetVkInstance() );
		CHECK_ERR( not _debugCallback );
	
		if ( not _debugReportSupported )
			return false;

		VkDebugReportCallbackCreateInfoEXT	dbg_callback_info = {};

		dbg_callback_info.sType			= VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		dbg_callback_info.flags			= flags;
		dbg_callback_info.pfnCallback	= _DebugReportCallback;
		dbg_callback_info.pUserData		= this;

		VK_CHECK( vkCreateDebugReportCallbackEXT( GetVkInstance(), &dbg_callback_info, null, OUT &_debugCallback ) );

		_callback = std::move(callback);
		return true;
	}
	
/*
=================================================
	GetMemoryTypeIndex
=================================================
*/
	bool VulkanDeviceExt::GetMemoryTypeIndex (const uint memoryTypeBits, const VkMemoryPropertyFlags flags, OUT uint &memoryTypeIndex) const
	{
		memoryTypeIndex = ~0u;

		for (uint32_t i = 0; i < _deviceMemoryProperties.memoryTypeCount; ++i)
		{
			const auto&		mem_type = _deviceMemoryProperties.memoryTypes[i];

			if ( ((memoryTypeBits >> i) & 1) == 1		and
				 EnumEq( mem_type.propertyFlags, flags ) )
			{
				memoryTypeIndex = i;
				return true;
			}
		}
		return false;
	}
	
/*
=================================================
	CompareMemoryTypes
=================================================
*/
	bool VulkanDeviceExt::CompareMemoryTypes (const uint memoryTypeBits, const VkMemoryPropertyFlags flags, const uint memoryTypeIndex) const
	{
		CHECK_ERR( memoryTypeIndex < _deviceMemoryProperties.memoryTypeCount );

		const auto&		mem_type = _deviceMemoryProperties.memoryTypes[ memoryTypeIndex ];

		if ( ((memoryTypeBits >> memoryTypeIndex) & 1) == 1	and
			 EnumEq( mem_type.propertyFlags, flags ) )
		{
			return true;
		}
		return false;
	}
	
/*
=================================================
	SetObjectName
=================================================
*/
	bool VulkanDeviceExt::SetObjectName (uint64_t id, StringView name, VkDebugReportObjectTypeEXT type) const
	{
		if ( name.empty() or id == VK_NULL_HANDLE or not _debugMarkersSupported )
			return false;

		VkDebugMarkerObjectNameInfoEXT	info = {};
		info.sType			= VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		info.objectType		= type;
		info.object			= id;
		info.pObjectName	= name.data();

		VK_CALL( vkDebugMarkerSetObjectNameEXT( GetVkDevice(), &info ));
		return true;
	}
	
/*
=================================================
	SetBreakOnValidationError
=================================================
*/
	void VulkanDeviceExt::SetBreakOnValidationError (bool value)
	{
		_breakOnValidationError = value;
	}


}	// FG

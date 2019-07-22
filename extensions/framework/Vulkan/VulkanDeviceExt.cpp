// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VulkanDeviceExt.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Algorithms/EnumUtils.h"

namespace FGC
{

/*
=================================================
	constructor
=================================================
*/
	VulkanDeviceExt::VulkanDeviceExt ()
	{
		_tempObjectDbgInfos.reserve( 16 );
		_tempString.reserve( 1024 );

		memset( &_properties, 0, sizeof(_properties) );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VulkanDeviceExt::~VulkanDeviceExt ()
	{
		_BeforeDestroy();
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

		_debugReportSupported	= HasInstanceExtension( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
		_debugUtilsSupported	= HasInstanceExtension( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
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

		_debugMarkersSupported	= HasDeviceExtension( VK_EXT_DEBUG_MARKER_EXTENSION_NAME );
		_memoryBudgetSupported	= HasDeviceExtension( VK_EXT_MEMORY_BUDGET_EXTENSION_NAME );

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
			
			if ( _deviceExtensions.count( VK_NV_MESH_SHADER_EXTENSION_NAME ) )
			{
				*next_props						= &_properties.meshShader;
				next_props						= &_properties.meshShader.pNext;
				_properties.meshShader.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
			}

			if ( _deviceExtensions.count( VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME ) )
			{
				*next_props							= &_properties.shadingRateImage;
				next_props							= &_properties.shadingRateImage.pNext;
				_properties.shadingRateImage.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV;
			}

			if ( _deviceExtensions.count( VK_NV_RAY_TRACING_EXTENSION_NAME ) )
			{
				*next_props						= &_properties.rayTracing;
				next_props						= &_properties.rayTracing.pNext;
				_properties.rayTracing.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
			}

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
		if ( _debugReportCallback ) {
			vkDestroyDebugReportCallbackEXT( GetVkInstance(), _debugReportCallback, null );
		}

		if ( _debugUtilsMessenger ) {
			vkDestroyDebugUtilsMessengerEXT( GetVkInstance(), _debugUtilsMessenger, null );
		}

		_instanceExtensions.clear();
		_deviceExtensions.clear();

		_debugReportCallback	= VK_NULL_HANDLE;
		_debugUtilsMessenger	= VK_NULL_HANDLE;
		_callback				= DebugReport_t();
		_debugReportSupported	= false;
		_debugUtilsSupported	= false;
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
		FG_LOGI( "apiVersion:    "s << ToString(VK_VERSION_MAJOR( GetDeviceProperties().apiVersion )) << '.' <<
				 ToString(VK_VERSION_MINOR( GetDeviceProperties().apiVersion )) << '.' <<
				 ToString(VK_VERSION_PATCH( GetDeviceProperties().apiVersion )) );

		FG_LOGI( "driverVersion: "s << ToString(VK_VERSION_MAJOR( GetDeviceProperties().driverVersion )) << '.' <<
				 ToString(VK_VERSION_MINOR( GetDeviceProperties().driverVersion )) );
		
		FG_LOGI( "vendorName:    "s << _GetVendorNameByID( GetDeviceProperties().vendorID ) );
		FG_LOGI( "deviceName:    "s << GetDeviceProperties().deviceName );
	}

/*
=================================================
	ObjectTypeToString
=================================================
*/
	ND_ static StringView  ObjectTypeToString (VkObjectType objType)
	{
		ENABLE_ENUM_CHECKS();
		switch ( objType )
		{
			case VK_OBJECT_TYPE_INSTANCE			: return "Instance";
			case VK_OBJECT_TYPE_PHYSICAL_DEVICE		: return "PhysicalDevice";
			case VK_OBJECT_TYPE_DEVICE				: return "Device";
			case VK_OBJECT_TYPE_QUEUE				: return "Queue";
			case VK_OBJECT_TYPE_SEMAPHORE			: return "Semaphore";
			case VK_OBJECT_TYPE_COMMAND_BUFFER		: return "CommandBuffer";
			case VK_OBJECT_TYPE_FENCE				: return "Fence";
			case VK_OBJECT_TYPE_DEVICE_MEMORY		: return "DeviceMemory";
			case VK_OBJECT_TYPE_BUFFER				: return "Buffer";
			case VK_OBJECT_TYPE_IMAGE				: return "Image";
			case VK_OBJECT_TYPE_EVENT				: return "Event";
			case VK_OBJECT_TYPE_QUERY_POOL			: return "QueryPool";
			case VK_OBJECT_TYPE_BUFFER_VIEW			: return "BufferView";
			case VK_OBJECT_TYPE_IMAGE_VIEW			: return "ImageView";
			case VK_OBJECT_TYPE_SHADER_MODULE		: return "ShaderModule";
			case VK_OBJECT_TYPE_PIPELINE_CACHE		: return "PipelineCache";
			case VK_OBJECT_TYPE_PIPELINE_LAYOUT		: return "PipelineLayout";
			case VK_OBJECT_TYPE_RENDER_PASS			: return "RenderPass";
			case VK_OBJECT_TYPE_PIPELINE			: return "Pipeline";
			case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: return "DescriptorSetLayout";
			case VK_OBJECT_TYPE_SAMPLER				: return "Sampler";
			case VK_OBJECT_TYPE_DESCRIPTOR_POOL		: return "DescriptorPool";
			case VK_OBJECT_TYPE_DESCRIPTOR_SET		: return "DescriptorSet";
			case VK_OBJECT_TYPE_FRAMEBUFFER			: return "Framebuffer";
			case VK_OBJECT_TYPE_COMMAND_POOL		: return "CommandPool";
			case VK_OBJECT_TYPE_SURFACE_KHR			: return "Surface";
			case VK_OBJECT_TYPE_SWAPCHAIN_KHR		: return "Swapchain";
			case VK_OBJECT_TYPE_DISPLAY_KHR			: return "Display";
			case VK_OBJECT_TYPE_DISPLAY_MODE_KHR	: return "DisplayMode";
			case VK_OBJECT_TYPE_OBJECT_TABLE_NVX	: return "ObjectTableNvx";
			case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX: return "IndirectCommandsLayoutNvx";
			case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV	: return "AccelerationStructureNv";
			case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT	: return "DebugReportCallback";
			case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT	: return "DebugUtilsMessenger";
			case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION	: return "SamplerYcBr";
			case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL: return "PerformanceConfigIntel";

			case VK_OBJECT_TYPE_UNKNOWN :
			case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE :
			case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT :
			case VK_OBJECT_TYPE_RANGE_SIZE :
			case VK_OBJECT_TYPE_MAX_ENUM :
				break;	// used to shutup compilation warnings
		}
		DISABLE_ENUM_CHECKS();

		CHECK(	objType >= VK_OBJECT_TYPE_BEGIN_RANGE and
				objType <= VK_OBJECT_TYPE_END_RANGE );
		return "unknown";
	}

/*
=================================================
	DebugReportObjectTypeToString
=================================================
*/
	ND_ static VkObjectType  DebugReportObjectTypeToObjectType (VkDebugReportObjectTypeEXT objType)
	{
		ENABLE_ENUM_CHECKS();
		switch ( objType )
		{
			case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT :				return VK_OBJECT_TYPE_UNKNOWN;
			case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT :				return VK_OBJECT_TYPE_INSTANCE;
			case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT :		return VK_OBJECT_TYPE_PHYSICAL_DEVICE;
			case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT :				return VK_OBJECT_TYPE_DEVICE;
			case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT :				return VK_OBJECT_TYPE_QUEUE;
			case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT :			return VK_OBJECT_TYPE_SEMAPHORE;
			case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT :		return VK_OBJECT_TYPE_COMMAND_BUFFER;
			case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT :				return VK_OBJECT_TYPE_FENCE;
			case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT :		return VK_OBJECT_TYPE_DEVICE_MEMORY;
			case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT :				return VK_OBJECT_TYPE_BUFFER;
			case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT :				return VK_OBJECT_TYPE_IMAGE;
			case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT :				return VK_OBJECT_TYPE_EVENT;
			case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT :			return VK_OBJECT_TYPE_QUERY_POOL;
			case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT :			return VK_OBJECT_TYPE_BUFFER_VIEW;
			case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT :			return VK_OBJECT_TYPE_IMAGE_VIEW;
			case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT :		return VK_OBJECT_TYPE_SHADER_MODULE;
			case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT :		return VK_OBJECT_TYPE_PIPELINE_CACHE;
			case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT :		return VK_OBJECT_TYPE_PIPELINE_LAYOUT;
			case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT :			return VK_OBJECT_TYPE_RENDER_PASS;
			case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT :				return VK_OBJECT_TYPE_PIPELINE;
			case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT : return VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
			case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT :				return VK_OBJECT_TYPE_SAMPLER;
			case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT :		return VK_OBJECT_TYPE_DESCRIPTOR_POOL;
			case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT :		return VK_OBJECT_TYPE_DESCRIPTOR_SET;
			case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT :			return VK_OBJECT_TYPE_FRAMEBUFFER;
			case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT :			return VK_OBJECT_TYPE_COMMAND_POOL;
			case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT :			return VK_OBJECT_TYPE_SURFACE_KHR;
			case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT :		return VK_OBJECT_TYPE_SWAPCHAIN_KHR;
			case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT :	return VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT;
			case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT :					return VK_OBJECT_TYPE_DISPLAY_KHR;
			case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT :				return VK_OBJECT_TYPE_DISPLAY_MODE_KHR;
			case VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT :				return VK_OBJECT_TYPE_OBJECT_TABLE_NVX;
			case VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT :	return VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX;
			case VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT_EXT :			return VK_OBJECT_TYPE_VALIDATION_CACHE_EXT;
			case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT :		return VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION;
			case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT :	return VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE;
			case VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV_EXT :	return VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV;
			case VK_DEBUG_REPORT_OBJECT_TYPE_RANGE_SIZE_EXT :
			case VK_DEBUG_REPORT_OBJECT_TYPE_MAX_ENUM_EXT :					break;
		}
		DISABLE_ENUM_CHECKS();
		return VK_OBJECT_TYPE_MAX_ENUM;
	}

/*
=================================================
	_DebugReportCallback
=================================================
*/
	VKAPI_ATTR VkBool32 VKAPI_CALL
		VulkanDeviceExt::_DebugReportCallback (VkDebugReportFlagsEXT flags,
											   VkDebugReportObjectTypeEXT objectType,
											   uint64_t object,
											   size_t /*location*/,
											   int32_t /*messageCode*/,
											   const char* /*pLayerPrefix*/,
											   const char* pMessage,
											   void* pUserData)
	{
		auto* self = static_cast<VulkanDeviceExt *>(pUserData);
		
		self->_tempObjectDbgInfos.resize( 1 );
		self->_tempObjectDbgInfos[0] = { ObjectTypeToString(DebugReportObjectTypeToObjectType( objectType )), "", object };

		self->_DebugReport({ self->_tempObjectDbgInfos, pMessage,
							 EnumEq( flags, VK_DEBUG_REPORT_ERROR_BIT_EXT )
						   });
		return VK_FALSE;
	}
	
/*
=================================================
	_DebugUtilsCallback
=================================================
*/
	VKAPI_ATTR VkBool32 VKAPI_CALL
		VulkanDeviceExt::_DebugUtilsCallback (VkDebugUtilsMessageSeverityFlagBitsEXT		messageSeverity,
											  VkDebugUtilsMessageTypeFlagsEXT				/*messageTypes*/,
											  const VkDebugUtilsMessengerCallbackDataEXT*	pCallbackData,
											  void*											pUserData)
	{
		auto* self = static_cast<VulkanDeviceExt *>(pUserData);
		
		self->_tempObjectDbgInfos.resize( pCallbackData->objectCount );

		for (uint i = 0; i < pCallbackData->objectCount; ++i)
		{
			auto&	obj = pCallbackData->pObjects[i];

			self->_tempObjectDbgInfos[i] = { ObjectTypeToString( obj.objectType ),
											 obj.pObjectName ? StringView{obj.pObjectName} : StringView{},
											 obj.objectHandle };
		}
		
		self->_DebugReport({ self->_tempObjectDbgInfos, pCallbackData->pMessage,
							 EnumEq( messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
						   });
		return VK_FALSE;
	}

/*
=================================================
	_DebugReport
=================================================
*/
	void VulkanDeviceExt::_DebugReport (const DebugReport &msg)
	{
		if ( _callback )
			return _callback( msg );

		String&		str = _tempString;
		str.clear();

		str << msg.message << '\n';

		for (auto& obj : msg.objects)
		{
			str << "object{ " << obj.type << ", \"" << obj.name << "\", " << ToString(obj.handle) << " }\n";
		}
		str << "----------------------------\n";

		if ( _breakOnValidationError and msg.isError )
			FG_LOGE( str )
		else
			FG_LOGI( str );
	}

/*
=================================================
	CreateDebugReportCallback
=================================================
*/
	bool VulkanDeviceExt::CreateDebugReportCallback (VkDebugReportFlagsEXT flags, DebugReport_t &&callback)
	{
		CHECK_ERR( GetVkInstance() );
		CHECK_ERR( not _debugReportCallback );
		CHECK_ERR( not _debugUtilsMessenger );
	
		if ( not _debugReportSupported )
			return false;

		VkDebugReportCallbackCreateInfoEXT	info = {};
		info.sType			= VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		info.flags			= flags;
		info.pfnCallback	= _DebugReportCallback;
		info.pUserData		= this;

		VK_CHECK( vkCreateDebugReportCallbackEXT( GetVkInstance(), &info, null, OUT &_debugReportCallback ) );

		_callback = std::move(callback);
		return true;
	}
	
/*
=================================================
	CreateDebugUtilsCallback
=================================================
*/
	bool VulkanDeviceExt::CreateDebugUtilsCallback (VkDebugUtilsMessageSeverityFlagsEXT severity, DebugReport_t &&callback)
	{
		CHECK_ERR( GetVkInstance() );
		CHECK_ERR( not _debugReportCallback );
		CHECK_ERR( not _debugUtilsMessenger );
	
		if ( not _debugUtilsSupported )
			return false;

		VkDebugUtilsMessengerCreateInfoEXT	info = {};
		info.sType				= VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		info.messageSeverity	= severity;
		info.messageType		= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT	 |
								  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
								  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		info.pfnUserCallback	= _DebugUtilsCallback;
		info.pUserData			= this;

		VK_CHECK( vkCreateDebugUtilsMessengerEXT( GetVkInstance(), &info, NULL, &_debugUtilsMessenger ));

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
		memoryTypeIndex = UMax;

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
	GetMemoryUsage
=================================================
*/
	ArrayView<VulkanDeviceExt::HeapInfo>  VulkanDeviceExt::GetMemoryUsage () const
	{
		if ( not _memoryBudgetSupported )
			return {};

		VkPhysicalDeviceMemoryProperties2			props		= {};
		VkPhysicalDeviceMemoryBudgetPropertiesEXT	mem_budget	= {};

		props.sType			= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
		props.pNext			= &mem_budget;
		mem_budget.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

		vkGetPhysicalDeviceMemoryProperties2( GetVkPhysicalDevice(), OUT &props );

		_heapInfos.resize( props.memoryProperties.memoryHeapCount );

		for (size_t i = 0; i < _heapInfos.size(); ++i)
		{
			_heapInfos[i].flags		= props.memoryProperties.memoryHeaps[i].flags;
			_heapInfos[i].total		= BytesU{props.memoryProperties.memoryHeaps[i].size};
			_heapInfos[i].budget	= BytesU{mem_budget.heapBudget[i]};
			_heapInfos[i].used		= BytesU{mem_budget.heapUsage[i]};
		}

		return _heapInfos;
	}

/*
=================================================
	SetObjectName
=================================================
*/
	bool VulkanDeviceExt::SetObjectName (uint64_t id, StringView name, VkObjectType type) const
	{
		if ( name.empty() or id == VK_NULL_HANDLE )
			return false;

		if ( _debugUtilsSupported )
		{
			VkDebugUtilsObjectNameInfoEXT	info = {};
			info.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			info.objectType		= type;
			info.objectHandle	= id;
			info.pObjectName	= name.data();

			VK_CALL( vkSetDebugUtilsObjectNameEXT( GetVkDevice(), &info ));
		}
		else
		if ( _debugMarkersSupported )
		{
			VkDebugMarkerObjectNameInfoEXT	info = {};
			info.sType			= VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
			info.objectType		= VkDebugReportObjectTypeEXT(type);
			info.object			= id;
			info.pObjectName	= name.data();

			VK_CALL( vkDebugMarkerSetObjectNameEXT( GetVkDevice(), &info ));
		}
		
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


}	// FGC

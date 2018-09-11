// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "VulkanDevice.h"
#include "stl/include/ToString.h"
#include "stl/include/StringUtils.h"
#include "stl/include/StaticString.h"
#include "stl/include/EnumUtils.h"
#include "stl/include/Cast.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VulkanDevice::VulkanDevice () :
		_vkDevice{ VK_NULL_HANDLE },
		_vkSurface{ VK_NULL_HANDLE },
		_vkPhysicalDevice{ VK_NULL_HANDLE },
		_vkInstance{ VK_NULL_HANDLE },
		_debugReportSupported{ false },
		_usedSharedInstance{ false }
	{
		VulkanDeviceFn_Init( &_deviceFnTable );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VulkanDevice::~VulkanDevice ()
	{
		Destroy();
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VulkanDevice::Create (UniquePtr<IVulkanSurface> &&surf, StringView appName, uint version,
							   StringView deviceName, ArrayView<QueueInfo> queues)
	{
		CHECK_ERR( surf );

		CHECK_ERR( _CreateInstance( appName, std::move(surf->GetRequiredExtensions()), version ));
		CHECK_ERR( _ChooseGpuDevice( deviceName ));

		_vkSurface = surf->Create( _vkInstance );
		CHECK_ERR( _vkSurface );
		
		CHECK_ERR( _SetupQueues( queues ));
		CHECK_ERR( _CreateDevice() );
		
		vkGetPhysicalDeviceFeatures( _vkPhysicalDevice, OUT &_deviceFeatures );
		vkGetPhysicalDeviceProperties( _vkPhysicalDevice, OUT &_deviceProperties );
		vkGetPhysicalDeviceMemoryProperties( _vkPhysicalDevice, OUT &_deviceMemoryProperties );

		return true;
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VulkanDevice::Create (VkInstance instance, UniquePtr<IVulkanSurface> &&surf, StringView deviceName, ArrayView<QueueInfo> queues)
	{
		CHECK_ERR( instance );

		_usedSharedInstance	= true;
		_vkInstance			= instance;
		
		CHECK_ERR( VulkanLoader::Initialize() );
		VulkanLoader::LoadInstance( _vkInstance );

		CHECK_ERR( _ChooseGpuDevice( deviceName ));

		if ( surf ) {
			_vkSurface = surf->Create( _vkInstance );
			CHECK_ERR( _vkSurface );
		}

		CHECK_ERR( _SetupQueues( queues ));
		CHECK_ERR( _CreateDevice() );
		
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VulkanDevice::Destroy ()
	{
		_DestroyDevice();
	}

/*
=================================================
	_CreateInstance
=================================================
*/
	bool VulkanDevice::_CreateInstance (StringView appName, Array<const char*> &&instanceExtensions, uint version)
	{
		CHECK_ERR( not _vkInstance );
		CHECK_ERR( VulkanLoader::Initialize() );
		
		Array< const char* >	instance_layers		= { "VK_LAYER_LUNARG_standard_validation",
														"VK_LAYER_LUNARG_assistant_layer",
														"VK_LAYER_LUNARG_core_validation",
														"VK_LAYER_GOOGLE_threading",
														"VK_LAYER_GOOGLE_unique_objects",
														"VK_LAYER_LUNARG_parameter_validation",
														"VK_LAYER_LUNARG_object_tracker",
														"VK_LAYER_ARM_mali_perf_doc" };

		Array< const char* >	instance_extensions	= std::move(instanceExtensions);
		
		instance_extensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );
		instance_extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
		instance_extensions.push_back( VK_EXT_DEBUG_MARKER_EXTENSION_NAME );
		instance_extensions.push_back( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );

        _ValidateInstanceLayers( INOUT instance_layers );
		_ValidateInstanceExtensions( INOUT instance_extensions );


		VkApplicationInfo		app_info = {};
		app_info.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.apiVersion			= version;
		app_info.pApplicationName	= appName.data();
		app_info.applicationVersion	= 0;
		app_info.pEngineName		= "FrameGraph";
		app_info.engineVersion		= 0;
		
		VkInstanceCreateInfo			instance_create_info = {};
		instance_create_info.sType						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pApplicationInfo			= &app_info;
		instance_create_info.enabledExtensionCount		= uint32_t(instance_extensions.size());
		instance_create_info.ppEnabledExtensionNames	= instance_extensions.data();
		instance_create_info.enabledLayerCount			= uint32_t(instance_layers.size());
		instance_create_info.ppEnabledLayerNames		= instance_layers.data();

		VK_CHECK( vkCreateInstance( &instance_create_info, null, OUT &_vkInstance ) );

		VulkanLoader::LoadInstance( _vkInstance );

		for (auto& ext : instance_extensions)
		{
			if ( StringView(ext) == VK_EXT_DEBUG_REPORT_EXTENSION_NAME )
				_debugReportSupported = true;
		}
		return true;
	}

/*
=================================================
    _ValidateInstanceLayers
=================================================
*/
    void VulkanDevice::_ValidateInstanceLayers (INOUT Array<const char*> &layers) const
    {
        Array<VkLayerProperties> inst_layers;

        // load supported layers
        uint32_t	count = 0;
        VK_CALL( vkEnumerateInstanceLayerProperties( OUT &count, null ) );

        if (count == 0)
        {
            layers.clear();
            return;
        }

        inst_layers.resize( count );
        VK_CALL( vkEnumerateInstanceLayerProperties( OUT &count, OUT inst_layers.data() ) );


        // validate
        for (auto iter = layers.begin(); iter != layers.end();)
        {
            bool    found = false;

            for (auto& prop : inst_layers)
            {
                if ( StringView(*iter) == prop.layerName ) {
                    found = true;
                    break;
                }
            }

            if ( not found )
            {
                FG_LOGI( "Vulkan layer \""s << (*iter) << "\" not supported and will be removed" );

                iter = layers.erase( iter );
            }
			else
				++iter;
        }
    }

/*
=================================================
	_ValidateInstanceExtensions
=================================================
*/
	void VulkanDevice::_ValidateInstanceExtensions (INOUT Array<const char*> &extensions) const
	{
		using ExtName_t = StaticString<VK_MAX_EXTENSION_NAME_SIZE>;

		HashSet<ExtName_t>	instance_extensions;


        // load supported extensions
		uint32_t	count = 0;
		VK_CALL( vkEnumerateInstanceExtensionProperties( null, OUT &count, null ) );

		if ( count == 0 )
		{
			extensions.clear();
			return;
		}

		Array< VkExtensionProperties >		inst_ext;
		inst_ext.resize( count );

		VK_CALL( vkEnumerateInstanceExtensionProperties( null, OUT &count, OUT inst_ext.data() ) );

		for (auto& ext : inst_ext) {
			instance_extensions.insert( StringView(ext.extensionName) );
		}


		// validate
		for (auto iter = extensions.begin(); iter != extensions.end();)
		{
			if ( instance_extensions.find( ExtName_t{*iter} ) == instance_extensions.end() )
			{
				FG_LOGI( "Vulkan instance extension \""s << (*iter) << "\" not supported and will be removed" );

				iter = extensions.erase( iter );
			}
			else
				 ++iter;
		}
	}
	
/*
=================================================
	_ValidateDeviceExtensions
=================================================
*/
    void VulkanDevice::_ValidateDeviceExtensions (INOUT Array<const char*> &extensions) const
    {
        // load supported device extensions
        uint32_t	count = 0;
        VK_CALL( vkEnumerateDeviceExtensionProperties( _vkPhysicalDevice, null, OUT &count, null ) );

        if ( count == 0 )
        {
            extensions.clear();
            return;
        }

        Array< VkExtensionProperties >	dev_ext;
        dev_ext.resize( count );

        VK_CALL( vkEnumerateDeviceExtensionProperties( _vkPhysicalDevice, null, OUT &count, OUT dev_ext.data() ));


        // validate
        for (auto iter = extensions.begin(); iter != extensions.end();)
        {
            bool    found = false;

            for (auto& ext : dev_ext)
            {
                if ( StringView(*iter) == ext.extensionName )
                {
                    found = true;
                    break;
                }
            }

            if ( not found )
            {
                FG_LOGI( "Vulkan device extension \""s << (*iter) << "\" not supported and will be removed" );

                iter = extensions.erase( iter );
            }
			else
				++iter;
        }
	}

/*
=================================================
	GetVendorNameByID
----
	from https://www.reddit.com/r/vulkan/comments/4ta9nj/is_there_a_comprehensive_list_of_the_names_and/
=================================================
*/
	ND_ static StringView  GetVendorNameByID (uint id)
	{
		switch ( id )
		{
			case 0x1002 :		return "AMD";
			case 0x1010 :		return "ImgTec";
			case 0x10DE :		return "NVIDIA";
			case 0x13B5 :		return "ARM";
			case 0x5143 :		return "Qualcomm";
			case 0x8086 :		return "INTEL";
		}
		return "unknown";
	}
	
/*
=================================================
	_ChooseGpuDevice
=================================================
*/
	bool VulkanDevice::_ChooseGpuDevice (StringView deviceName)
	{
		CHECK_ERR( _vkInstance );
		CHECK_ERR( not _vkPhysicalDevice );
		
		uint32_t						count	= 0;
		Array< VkPhysicalDevice >		devices;
		
		VK_CALL( vkEnumeratePhysicalDevices( _vkInstance, OUT &count, null ) );
		CHECK_ERR( count > 0 );

		devices.resize( count );
		VK_CALL( vkEnumeratePhysicalDevices( _vkInstance, OUT &count, OUT devices.data() ) );
		
        VkPhysicalDevice	match_name_device	= VK_NULL_HANDLE;
        VkPhysicalDevice	high_perf_device	= VK_NULL_HANDLE;
		float				max_performance		= 0.0f;

		for (auto& dev : devices)
		{
			VkPhysicalDeviceProperties			prop	 = {};
			VkPhysicalDeviceFeatures			feat	 = {};
			VkPhysicalDeviceMemoryProperties	mem_prop = {};
			
			vkGetPhysicalDeviceProperties( dev, OUT &prop );
			vkGetPhysicalDeviceFeatures( dev, OUT &feat );
			vkGetPhysicalDeviceMemoryProperties( dev, OUT &mem_prop );

			const bool	is_gpu		  = (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU or
										 prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
			const bool	is_integrated = prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

			if ( not is_gpu )
				continue;

																									// magic function:
			const float		perf	= //float(info.globalMemory.Mb()) / 1024.0f +							// commented becouse of bug on Intel (incorrect heap size)
										float(prop.limits.maxComputeSharedMemorySize >> 10) / 64.0f +		// big local cache is goode
										float(is_gpu and not is_integrated ? 4 : 1) +						// 
										float(prop.limits.maxComputeWorkGroupInvocations) / 1024.0f +		// 
										float(feat.tessellationShader + feat.geometryShader);

			const bool		nmatch	=	not deviceName.empty()							and
										HasSubStringIC( prop.deviceName, deviceName )	or
										HasSubStringIC( GetVendorNameByID( prop.vendorID ), deviceName );

			if ( perf > max_performance ) {
				max_performance		= perf;
				high_perf_device	= dev;
			}

			if ( nmatch and not match_name_device ) {
				match_name_device	= dev;
			}
		}
			
		_vkPhysicalDevice = (match_name_device ? match_name_device : high_perf_device);
		CHECK_ERR( _vkPhysicalDevice );
		
		vkGetPhysicalDeviceFeatures( _vkPhysicalDevice, OUT &_deviceFeatures );
		vkGetPhysicalDeviceProperties( _vkPhysicalDevice, OUT &_deviceProperties );
		vkGetPhysicalDeviceMemoryProperties( _vkPhysicalDevice, OUT &_deviceMemoryProperties );

		_WriteDeviceInfo();
		return true;
	}
	
/*
=================================================
	_WriteDeviceInfo
=================================================
*/
	void VulkanDevice::_WriteDeviceInfo ()
	{
		FG_LOGI( "apiVersion:  "s << ToString(VK_VERSION_MAJOR( _deviceProperties.apiVersion )) << '.' <<
				 ToString(VK_VERSION_MINOR( _deviceProperties.apiVersion )) << ' ' <<
				 ToString(VK_VERSION_PATCH( _deviceProperties.apiVersion )) );
		
		FG_LOGI( "vendorName:  "s << GetVendorNameByID( _deviceProperties.vendorID ) );
		FG_LOGI( "deviceName:  "s << _deviceProperties.deviceName );
	}

/*
=================================================
	_SetupQueues
=================================================
*/
	bool VulkanDevice::_SetupQueues (ArrayView<QueueInfo> queues)
	{
		CHECK_ERR( _vkQueues.empty() );

		// setup default queue
		if ( queues.empty() )
		{
			uint			index	= 0;
			VkQueueFlags	flags	= VkQueueFlags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
			
			if ( _vkSurface )
				flags |= VK_QUEUE_PRESENT_BIT;

			CHECK_ERR( _ChooseQueueIndex( INOUT flags, OUT index ));

			_vkQueues.push_back({ flags, index, 0.0f });
			return true;
		}


		for (auto& q : queues)
		{
			uint			index	= 0;
			VkQueueFlags	flags	= q.flags;
			CHECK_ERR( _ChooseQueueIndex( INOUT flags, OUT index ));

			_vkQueues.push_back({ flags, index, q.priority });
		}

		return true;
	}
	
/*
=================================================
	_CreateDevice
=================================================
*/
	bool VulkanDevice::_CreateDevice ()
	{
		CHECK_ERR( _vkPhysicalDevice );
		CHECK_ERR( not _vkDevice );
		CHECK_ERR( not _vkQueues.empty() );

		// setup device create info
		VkDeviceCreateInfo		device_info	= {};
		device_info.sType		= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;


		// setup extensions
		Array<const char *>		device_extensions = { VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
													  VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
													  VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		_ValidateDeviceExtensions( INOUT device_extensions );

		if ( not device_extensions.empty() )
		{
			device_info.enabledExtensionCount	= uint32_t(device_extensions.size());
			device_info.ppEnabledExtensionNames	= device_extensions.data();
		}


		// setup queues
		FixedArray< VkDeviceQueueCreateInfo, 8 >	queue_infos;
		
		for (auto& q : _vkQueues)
		{
			VkDeviceQueueCreateInfo		queue_create_info = {};
			queue_create_info.sType				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex	= q.index;
			queue_create_info.queueCount		= 1;
			queue_create_info.pQueuePriorities	= &q.priority;
			queue_infos.push_back( queue_create_info );
		}

		device_info.queueCreateInfoCount	= uint(queue_infos.size());
		device_info.pQueueCreateInfos		= queue_infos.data();


		// setup features
		VkPhysicalDeviceFeatures	enabled_features = {};
		vkGetPhysicalDeviceFeatures( _vkPhysicalDevice, OUT &enabled_features );
		device_info.pEnabledFeatures = &enabled_features;

		VK_CHECK( vkCreateDevice( _vkPhysicalDevice, &device_info, null, OUT &_vkDevice ));
		

		VulkanLoader::LoadDevice( _vkDevice, OUT _deviceFnTable );

		for (auto& q : _vkQueues) {
			vkGetDeviceQueue( _vkDevice, q.index, 0, OUT &q.id );
		}

		return true;
	}

/*
=================================================
	DebugReportObjectTypeToString
=================================================
*/
	String  DebugReportObjectTypeToString (VkDebugReportObjectTypeEXT objType)
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
			case VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT	: return "IndirectCommandsLayoutNvx";

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
	VkBool32 VKAPI_CALL VulkanDevice::_DebugReportCallback (VkDebugReportFlagsEXT flags,
															VkDebugReportObjectTypeEXT objectType,
															uint64_t object,
															size_t /*location*/,
															int32_t /*messageCode*/,
															const char* pLayerPrefix,
															const char* pMessage,
															void* pUserData)
	{
		static_cast<VulkanDevice *>(pUserData)->_DebugReport( flags, DebugReportObjectTypeToString(objectType), object, pLayerPrefix, pMessage );
		return VK_FALSE;
	}
	
/*
=================================================
	_DebugReport
=================================================
*/
	void VulkanDevice::_DebugReport (VkDebugReportFlagsEXT flags, StringView objectType, uint64_t object, StringView layerPrefix, StringView message) const
	{
		if ( _callback )
			return _callback({ flags, objectType, object, layerPrefix, message });
		

		if ( (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) )
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
	bool VulkanDevice::CreateDebugCallback (VkDebugReportFlagsEXT flags, DebugReport_t &&callback)
	{
		CHECK_ERR( _vkDevice );
		CHECK_ERR( not _debugCallback );
		CHECK_ERR( _debugReportSupported );
		
		VkDebugReportCallbackCreateInfoEXT	dbg_callback_info = {};

		dbg_callback_info.sType			= VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		dbg_callback_info.flags			= flags;
		dbg_callback_info.pfnCallback	= _DebugReportCallback;
		dbg_callback_info.pUserData		= this;

		VK_CHECK( vkCreateDebugReportCallbackEXT( _vkInstance, &dbg_callback_info, null, OUT &_debugCallback ) );

		_callback = std::move(callback);
		return true;
	}

/*
=================================================
	_ChooseQueueIndex
=================================================
*/
	bool VulkanDevice::_ChooseQueueIndex (INOUT VkQueueFlags &requiredFlags, OUT uint32_t &index) const
	{
		Array< VkQueueFamilyProperties >	queue_family_props;
		uint32_t							count = 0;

		vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, OUT &count, null );
		CHECK_ERR( count > 0 );

		queue_family_props.resize( count );
		vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, OUT &count, OUT queue_family_props.data() );
		

		for (size_t i = 0; i < queue_family_props.size(); ++i)
		{
			VkBool32		supports_present = false;
			const auto&		prop			 = queue_family_props[i];
			VkQueueFlags	curr_flags		 = prop.queueFlags;
			
			if ( _vkSurface )
			{
				VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _vkPhysicalDevice, uint32_t(i), _vkSurface, OUT &supports_present ));

				if ( supports_present )
					curr_flags |= VK_QUEUE_PRESENT_BIT;
			}

			if ( EnumEq( curr_flags, requiredFlags ) )
			{
				requiredFlags	= curr_flags;
				index			= uint32_t(i);
				return true;
			}
		}

		RETURN_ERR( "no suitable queue family found!" );
	}

/*
=================================================
	_DestroyDevice
=================================================
*/
	void VulkanDevice::_DestroyDevice ()
	{
		if ( _debugCallback ) {
			vkDestroyDebugReportCallbackEXT( _vkInstance, _debugCallback, null );
		}

		if ( _vkDevice ) {
			vkDestroyDevice( _vkDevice, null );
			VulkanLoader::ResetDevice( OUT _deviceFnTable );
		}

		if ( _vkInstance )
		{
			if ( not _usedSharedInstance )
				vkDestroyInstance( _vkInstance, null );

			VulkanLoader::Unload();
		}

		_vkInstance				= VK_NULL_HANDLE;
		_vkPhysicalDevice		= VK_NULL_HANDLE;
		_vkDevice				= VK_NULL_HANDLE;
		_debugCallback			= VK_NULL_HANDLE;
		_debugReportSupported	= false;
		_usedSharedInstance		= false;
		
		_vkQueues.clear();
		_callback		= DebugReport_t();
	}

}	// FG

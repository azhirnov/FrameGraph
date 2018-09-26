// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VulkanDevice.h"
#include "stl/include/StaticString.h"
#include "stl/include/StringUtils.h"
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
	GetRecomendedInstanceLayers
=================================================
*/
	ArrayView<const char*>  VulkanDevice::GetRecomendedInstanceLayers ()
	{
		static const char*	instance_layers[] = {
			"VK_LAYER_LUNARG_standard_validation",
			"VK_LAYER_LUNARG_assistant_layer",
			"VK_LAYER_LUNARG_core_validation",
			"VK_LAYER_GOOGLE_threading",
			"VK_LAYER_GOOGLE_unique_objects",
			"VK_LAYER_LUNARG_parameter_validation",
			"VK_LAYER_LUNARG_object_tracker",
			"VK_LAYER_ARM_mali_perf_doc"
		};
		return instance_layers;
	}
	
/*
=================================================
	GetRecomendedInstanceExtensions
=================================================
*/
	ArrayView<const char*>  VulkanDevice::GetRecomendedInstanceExtensions ()
	{
		static const char*	instance_extensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
			VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
		};
		return instance_extensions;
	}

/*
=================================================
	GetRecomendedDeviceExtensions
=================================================
*/
	ArrayView<const char*>  VulkanDevice::GetRecomendedDeviceExtensions ()
	{
		static const char *	device_extensions[] = {
			VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
			VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
			VK_EXT_DEBUG_MARKER_EXTENSION_NAME
		};
		return device_extensions;
	}

/*
=================================================
	Create
=================================================
*/
	bool VulkanDevice::Create (UniquePtr<IVulkanSurface>&&	surface,
							   StringView					appName,
							   const uint					version,
							   StringView					deviceName,
							   ArrayView<QueueCreateInfo>	queues,
							   ArrayView<const char*>		instanceLayers,
							   ArrayView<const char*>		instanceExtensions,
							   ArrayView<const char*>		deviceExtensions)
	{
		CHECK_ERR( surface );

		Array<const char*>	inst_ext { surface->GetRequiredExtensions() };
		inst_ext.insert( inst_ext.end(), instanceExtensions.begin(), instanceExtensions.end() );

		CHECK_ERR( _CreateInstance( appName, instanceLayers, std::move(inst_ext), version ));
		CHECK_ERR( _ChooseGpuDevice( deviceName ));

		_vkSurface = surface->Create( _vkInstance );
		CHECK_ERR( _vkSurface );
		
		CHECK_ERR( _SetupQueues( queues ));
		CHECK_ERR( _CreateDevice( deviceExtensions ));

		return true;
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VulkanDevice::Create (VkInstance					instance,
							   UniquePtr<IVulkanSurface>&&	surface,
							   StringView					deviceName,
							   ArrayView<QueueCreateInfo>	queues,
							   ArrayView<const char*>		deviceExtensions)
	{
		CHECK_ERR( instance );

		_usedSharedInstance	= true;
		_vkInstance			= instance;
		
		CHECK_ERR( VulkanLoader::Initialize() );
		VulkanLoader::LoadInstance( _vkInstance );

		CHECK_ERR( _ChooseGpuDevice( deviceName ));

		if ( surface ) {
			_vkSurface = surface->Create( _vkInstance );
			CHECK_ERR( _vkSurface );
		}

		CHECK_ERR( _SetupQueues( queues ));
		CHECK_ERR( _CreateDevice( deviceExtensions ));
		
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
	bool VulkanDevice::_CreateInstance (StringView appName, ArrayView<const char*> layers, Array<const char*> &&extensions, uint version)
	{
		CHECK_ERR( not _vkInstance );
		CHECK_ERR( VulkanLoader::Initialize() );
		
		Array< const char* >	instance_layers;
		instance_layers.assign( layers.begin(), layers.end() );

        _ValidateInstanceLayers( INOUT instance_layers );
		_ValidateInstanceExtensions( INOUT extensions );


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
		instance_create_info.enabledExtensionCount		= uint(extensions.size());
		instance_create_info.ppEnabledExtensionNames	= extensions.data();
		instance_create_info.enabledLayerCount			= uint(instance_layers.size());
		instance_create_info.ppEnabledLayerNames		= instance_layers.data();

		VK_CHECK( vkCreateInstance( &instance_create_info, null, OUT &_vkInstance ) );

		VulkanLoader::LoadInstance( _vkInstance );

		_OnInstanceCreated( std::move(instance_layers), std::move(extensions) );
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
        uint	count = 0;
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
		uint	count = 0;
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
        uint	count = 0;
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
	_GetVendorNameByID
----
	from https://www.reddit.com/r/vulkan/comments/4ta9nj/is_there_a_comprehensive_list_of_the_names_and/
=================================================
*/
	StringView  VulkanDevice::_GetVendorNameByID (uint id)
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
		
		uint						count	= 0;
		Array< VkPhysicalDevice >	devices;
		
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
                                        (HasSubStringIC( prop.deviceName, deviceName )	or
                                         HasSubStringIC( _GetVendorNameByID( prop.vendorID ), deviceName ));

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

		return true;
	}

/*
=================================================
	_SetupQueues
=================================================
*/
	bool VulkanDevice::_SetupQueues (ArrayView<QueueCreateInfo> queues)
	{
		CHECK_ERR( _vkQueues.empty() );

		// setup default queue
		if ( queues.empty() )
		{
			uint			family_index	= 0;
			VkQueueFlags	flags			= VkQueueFlags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
			
			if ( _vkSurface )
				flags |= VK_QUEUE_PRESENT_BIT;

			CHECK_ERR( _ChooseQueueIndex( INOUT flags, OUT family_index ));

			_vkQueues.push_back({ VK_NULL_HANDLE, family_index, ~0u, flags, 0.0f });
			return true;
		}


		for (auto& q : queues)
		{
			uint			family_index	= 0;
			VkQueueFlags	flags			= q.flags;
			CHECK_ERR( _ChooseQueueIndex( INOUT flags, OUT family_index ));

			_vkQueues.push_back({ VK_NULL_HANDLE, family_index, ~0u, flags, q.priority });
		}

		return true;
	}
	
/*
=================================================
	_CreateDevice
=================================================
*/
	bool VulkanDevice::_CreateDevice (ArrayView<const char*> extensions)
	{
		CHECK_ERR( _vkPhysicalDevice );
		CHECK_ERR( not _vkDevice );
		CHECK_ERR( not _vkQueues.empty() );

		// setup device create info
		VkDeviceCreateInfo		device_info	= {};
		device_info.sType		= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;


		// setup extensions
		Array<const char *>		device_extensions;
		device_extensions.assign( extensions.begin(), extensions.end() );

		if ( _vkSurface )
			device_extensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );

		_ValidateDeviceExtensions( INOUT device_extensions );

		if ( not device_extensions.empty() )
		{
			device_info.enabledExtensionCount	= uint(device_extensions.size());
			device_info.ppEnabledExtensionNames	= device_extensions.data();
		}


		// setup queues
		Array< VkDeviceQueueCreateInfo >	queue_infos;
		Array< FixedArray<float,16> >		priorities;
		{
			uint	max_queue_families = 0;
			vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, OUT &max_queue_families, null );

			queue_infos.resize( max_queue_families );
			priorities.resize( max_queue_families );

			for (size_t i = 0; i < queue_infos.size(); ++i)
			{
				auto&	ci = queue_infos[i];

				ci.sType			= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				ci.pNext			= null;
				ci.queueFamilyIndex	= uint(i);
				ci.queueCount		= 0;
				ci.pQueuePriorities	= priorities[i].data();
			}

			for (auto& q : _vkQueues)
			{
				q.queueIndex = (queue_infos[ q.familyIndex ].queueCount++);
				priorities[ q.familyIndex ].push_back( q.priority );
			}

			// remove unused queues
			for (auto iter = queue_infos.begin(); iter != queue_infos.end();)
			{
				if ( iter->queueCount == 0 )
					iter = queue_infos.erase( iter );
				else
					++iter;
			}
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
			vkGetDeviceQueue( _vkDevice, q.familyIndex, q.queueIndex, OUT &q.id );
		}

		_OnLogicalDeviceCreated( std::move(device_extensions) );
		return true;
	}

/*
=================================================
	_ChooseQueueIndex
=================================================
*/
	bool VulkanDevice::_ChooseQueueIndex (INOUT VkQueueFlags &requiredFlags, OUT uint &index) const
	{
		Array< VkQueueFamilyProperties >	queue_family_props;
		uint								count = 0;

		vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, OUT &count, null );
		CHECK_ERR( count > 0 );

		queue_family_props.resize( count );
		vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, OUT &count, OUT queue_family_props.data() );

		// validate required flags
		{
			// if the capabilities of a queue family include VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT,
			// then reporting the VK_QUEUE_TRANSFER_BIT capability separately for that queue family is optional.
			if ( requiredFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT) )
				requiredFlags &= ~VK_QUEUE_TRANSFER_BIT;
		}

		for (size_t i = 0; i < queue_family_props.size(); ++i)
		{
			VkBool32		supports_present = false;
			const auto&		prop			 = queue_family_props[i];
			VkQueueFlags	curr_flags		 = prop.queueFlags;
			
			if ( _vkSurface )
			{
				VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _vkPhysicalDevice, uint(i), _vkSurface, OUT &supports_present ));

				if ( supports_present )
					curr_flags |= VK_QUEUE_PRESENT_BIT;
			}

			if ( EnumEq( curr_flags, requiredFlags ) )
			{
				requiredFlags	= curr_flags;
				index			= uint(i);
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
		_BeforeDestroy();

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
		_usedSharedInstance		= false;
		_vkQueues.clear();

		_AfterDestroy();
	}

}	// FG

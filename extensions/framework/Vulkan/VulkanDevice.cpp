// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef FG_ENABLE_VULKAN

# include "framework/Vulkan/VulkanDevice.h"
# include "stl/Containers/StaticString.h"
# include "stl/Algorithms/StringUtils.h"
# include <bitset>

namespace FGC
{
	
	bool  VulkanDevice::InstanceVersion::operator == (const InstanceVersion &rhs) const
	{
		return major == rhs.major and minor == rhs.minor;
	}

	bool  VulkanDevice::InstanceVersion::operator >  (const InstanceVersion &rhs) const
	{
		return	major != rhs.major ? major > rhs.major :
									 minor > rhs.minor;
	}

	bool  VulkanDevice::InstanceVersion::operator >= (const InstanceVersion &rhs) const
	{
		return not (rhs > *this);
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	constructor
=================================================
*/
	VulkanDevice::VulkanDevice () :
		_vkLogicalDevice{ VK_NULL_HANDLE },
		_vkPhysicalDevice{ VK_NULL_HANDLE },
		_vkInstance{ VK_NULL_HANDLE }
	{
		std::memset( &_features, 0, sizeof(_features) );
		std::memset( &_properties, 0, sizeof(_properties) );

		VulkanDeviceFn_Init( &_deviceFnTable );
	}

/*
=================================================
	destructor
=================================================
*/
	VulkanDevice::~VulkanDevice ()
	{
		CHECK( not _vkInstance );
		CHECK( not _vkLogicalDevice );
	}

/*
=================================================
	HasInstanceExtension
=================================================
*/
	bool  VulkanDevice::HasInstanceExtension (StringView name) const
	{
		return !!_instanceExtensions.count( name );
	}

/*
=================================================
	HasDeviceExtension
=================================================
*/
	bool  VulkanDevice::HasDeviceExtension (StringView name) const
	{
		return !!_deviceExtensions.count( name );
	}

/*
=================================================
	SetObjectName
=================================================
*/
	bool  VulkanDevice::SetObjectName (uint64_t id, NtStringView name, VkObjectType type) const
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

			VK_CALL( vkSetDebugUtilsObjectNameEXT( _vkLogicalDevice, &info ));
			return true;
		}

		return false;
	}
	
/*
=================================================
	GetQueueFamilies
=================================================
*/
	void  VulkanDevice::GetQueueFamilies (VQueueMask mask, OUT VQueueFamilyIndices_t &result) const
	{
		result.clear();

		for (uint i = 0; i <= uint(mask); ++i)
		{
			if ( not AllBits( mask, i ))
				continue;

			auto	q = GetQueue( VQueueType(i) );
			if ( not q )
				continue;

			result.push_back( q->familyIndex );
		}
	}
//-----------------------------------------------------------------------------

	
		
/*
=================================================
	constructor
=================================================
*/
	VulkanDeviceInitializer::VulkanDeviceInitializer ()
	{
		_tempObjectDbgInfos.reserve( 16 );
		_tempString.reserve( 1024 );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VulkanDeviceInitializer::~VulkanDeviceInitializer ()
	{
		DestroyDebugCallback();
	}

/*
=================================================
	GetInstanceVersion
=================================================
*/
	VulkanDeviceInitializer::InstanceVersion  VulkanDeviceInitializer::GetInstanceVersion () const
	{
	#ifdef VK_VERSION_1_1
		uint	ver = VK_MAKE_VERSION( 1, 0, 0 );
		Unused( vkEnumerateInstanceVersion( OUT &ver ));

		return InstanceVersion{ VK_VERSION_MAJOR(ver), VK_VERSION_MINOR(ver) };
	#else
		return InstanceVersion{ 1, 0 };
	#endif
	}
	
/*
=================================================
	CreateInstance
=================================================
*/
	bool  VulkanDeviceInitializer::CreateInstance (NtStringView appName, NtStringView engineName, ArrayView<const char*> layers,
											  ArrayView<const char*> extensions, InstanceVersion version, uint appVer, uint engineVer)
	{
		CHECK_ERR( not _vkInstance );
		CHECK_ERR( VulkanLoader::Initialize() );
		
		uint	vk_ver = VK_MAKE_VERSION( version.major, version.minor, 0 );
		_ValidateInstanceVersion( INOUT vk_ver );
		_UpdateInstanceVersion( vk_ver );

		Array< const char* >	instance_layers;
		instance_layers.assign( layers.begin(), layers.end() );

		Array< const char* >	instance_extensions;
		ArrayView<const char*>	default_inst_ext = _GetInstanceExtensions( _vkVersion );
		instance_extensions.assign( default_inst_ext.begin(), default_inst_ext.end() );
		instance_extensions.insert( instance_extensions.end(), extensions.begin(), extensions.end() );

		_ValidateInstanceLayers( INOUT instance_layers );
		_ValidateInstanceExtensions( INOUT instance_extensions );
		
		VkApplicationInfo		app_info = {};
		app_info.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.apiVersion			= vk_ver;
		app_info.pApplicationName	= appName.c_str();
		app_info.applicationVersion	= appVer;
		app_info.pEngineName		= engineName.c_str();
		app_info.engineVersion		= engineVer;

		VkInstanceCreateInfo	instance_create_info = {};
		instance_create_info.sType						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pApplicationInfo			= &app_info;
		instance_create_info.enabledExtensionCount		= uint(instance_extensions.size());
		instance_create_info.ppEnabledExtensionNames	= instance_extensions.data();
		instance_create_info.enabledLayerCount			= uint(instance_layers.size());
		instance_create_info.ppEnabledLayerNames		= instance_layers.data();

		VK_CHECK( vkCreateInstance( &instance_create_info, null, OUT &_vkInstance ));

		CHECK_ERR( VulkanLoader::LoadInstance( _vkInstance ));

		for (auto inst : instance_extensions) {
			_instanceExtensions.insert( inst );
		}

		_LogPhysicalDevices();
		VulkanLoader::SetupInstanceBackwardCompatibility( vk_ver );
		return true;
	}
	
/*
=================================================
	CreateInstance
=================================================
*/
	bool  VulkanDeviceInitializer::CreateInstance (UniquePtr<IVulkanSurface> surface, NtStringView appName, NtStringView engineName, ArrayView<const char*> instanceLayers,
													ArrayView<const char*> instanceExtensions, InstanceVersion version, uint appVer, uint engineVer)
	{
		CHECK_ERR( surface );

		Array<const char*>	inst_ext { surface->GetRequiredExtensions() };
		inst_ext.insert( inst_ext.end(), instanceExtensions.begin(), instanceExtensions.end() );

		CHECK_ERR( CreateInstance( appName, engineName, instanceLayers, inst_ext, version, appVer, engineVer ));
	
		_vkSurface = BitCast<VkSurfaceKHR>( surface->Create( BitCast<IVulkanSurface::InstanceVk_t>( _vkInstance )));
		CHECK_ERR( _vkSurface );

		return true;
	}

/*
=================================================
	SetInstance
=================================================
*/
	bool  VulkanDeviceInitializer::SetInstance (VkInstance newInstance, ArrayView<const char*> instanceExtensions)
	{
		CHECK_ERR( not _vkInstance );
		CHECK_ERR( newInstance );
		CHECK_ERR( VulkanLoader::Initialize() );

		_vkInstance			= newInstance;
		_usedSharedInstance	= true;

		CHECK_ERR( VulkanLoader::LoadInstance( _vkInstance ));
		
		for (auto inst : instanceExtensions) {
			_instanceExtensions.insert( inst );
		}
		
		uint	vk_ver = VK_MAKE_VERSION( 1, 2, 0 );
		_ValidateInstanceVersion( INOUT vk_ver );
		_UpdateInstanceVersion( vk_ver );

		_LogPhysicalDevices();
		VulkanLoader::SetupInstanceBackwardCompatibility( vk_ver );
		return true;
	}
	
/*
=================================================
	SetInstance
=================================================
*/
	bool  VulkanDeviceInitializer::SetInstance (UniquePtr<IVulkanSurface> surface, VkInstance value, ArrayView<const char*> instanceExtensions)
	{
		CHECK_ERR( surface );
		CHECK_ERR( SetInstance( value, instanceExtensions ));

		_vkSurface = BitCast<VkSurfaceKHR>( surface->Create( BitCast<IVulkanSurface::InstanceVk_t>( _vkInstance )));
		CHECK_ERR( _vkSurface );

		return true;
	}

/*
=================================================
	DestroyInstance
=================================================
*/
	bool  VulkanDeviceInitializer::DestroyInstance ()
	{
		if ( not _vkInstance )
			return false;
		
		DestroyDebugCallback();
		
		if ( _vkSurface ) {
			vkDestroySurfaceKHR( _vkInstance, _vkSurface, null );
		}

		if ( not _usedSharedInstance )
		{
			vkDestroyInstance( _vkInstance, null );
		}
		VulkanLoader::Unload();
		
		_vkSurface			= VK_NULL_HANDLE;
		_vkInstance			= VK_NULL_HANDLE;
		_vkPhysicalDevice	= VK_NULL_HANDLE;
		_usedSharedInstance	= false;
		
		_instanceExtensions.clear();

		FG_LOGD( "Destroyed Vulkan instance" );
		return true;
	}

/*
=================================================
	WithPhysicalDevices
=================================================
*/
namespace {
	template <typename Fn>
	void  WithPhysicalDevices (const VulkanDevice &vulkan, Fn &&fn)
	{
		uint								count	= 0;
		FixedArray< VkPhysicalDevice, 16 >	devices;
		
		VK_CALL( vkEnumeratePhysicalDevices( vulkan.GetVkInstance(), OUT &count, null ));
		CHECK_ERR( count > 0, void());

		devices.resize( count );
		count = uint(devices.size());
		VK_CALL( vkEnumeratePhysicalDevices( vulkan.GetVkInstance(), OUT &count, OUT devices.data() ));

		for (auto& dev : devices)
		{
			VkPhysicalDeviceProperties			prop	 = {};
			VkPhysicalDeviceFeatures			feat	 = {};
			VkPhysicalDeviceMemoryProperties	mem_prop = {};
			
			vkGetPhysicalDeviceProperties( dev, OUT &prop );
			vkGetPhysicalDeviceFeatures( dev, OUT &feat );
			vkGetPhysicalDeviceMemoryProperties( dev, OUT &mem_prop );

			fn( dev, prop, feat, mem_prop );
		}
	}
}
/*
=================================================
	GetVendorNameByID
----
	from https://www.reddit.com/r/vulkan/comments/4ta9nj/is_there_a_comprehensive_list_of_the_names_and/
=================================================
*/
namespace {
	ND_ StringView  GetVendorNameByID (uint id)
	{
		switch ( id )
		{
			case 0x1002 :	return "AMD";
			case 0x1010 :	return "ImgTec";
			case 0x10DE :	return "NVIDIA";
			case 0x13B5 :	return "ARM";
			case 0x5143 :	return "Qualcomm";
			case 0x8086 :	return "INTEL";
		}
		return "unknown";
	}
}
/*
=================================================
	CalcTotalMemory
=================================================
*/
namespace {
	ND_ BytesU  CalcTotalMemory (VkPhysicalDeviceMemoryProperties memProps)
	{
		BytesU	total;

		for (uint32_t j = 0; j < memProps.memoryTypeCount; ++j)
		{
			if ( AllBits( memProps.memoryTypes[j].propertyFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ))
			{
				const uint32_t idx = memProps.memoryTypes[j].heapIndex;

				if ( AllBits( memProps.memoryHeaps[idx].flags, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ))
				{
					total += BytesU( memProps.memoryHeaps[idx].size );
					memProps.memoryHeaps[idx].size = 0;
				}
			}
		}
		return total;
	}
}
/*
=================================================
	ChooseDevice
=================================================
*/
	bool  VulkanDeviceInitializer::ChooseDevice (StringView deviceName)
	{
		CHECK_ERR( _vkInstance );
		CHECK_ERR( not _vkLogicalDevice );

		VkPhysicalDevice	pdev = VK_NULL_HANDLE;

		WithPhysicalDevices( *this,
			[deviceName, &pdev] (VkPhysicalDevice dev, auto& prop, auto&...)
			{
				bool	math = not deviceName.empty()							and
							   (HasSubStringIC( prop.deviceName, deviceName )	or
								HasSubStringIC( GetVendorNameByID( prop.vendorID ), deviceName ));

				if ( math and not pdev )
					pdev = dev;
			});
		
		if ( pdev == VK_NULL_HANDLE )
			return false;

		_vkPhysicalDevice = pdev;
		return true;
	}
	
/*
=================================================
	ChooseHighPerformanceDevice
=================================================
*/
	bool  VulkanDeviceInitializer::ChooseHighPerformanceDevice ()
	{
		CHECK_ERR( _vkInstance );
		CHECK_ERR( not _vkLogicalDevice );
		
		VkPhysicalDevice	any_device			= VK_NULL_HANDLE;
		VkPhysicalDevice	high_perf_device	= VK_NULL_HANDLE;
		float				max_performance		= 0.0f;

		WithPhysicalDevices( *this,
			[&any_device, &high_perf_device, &max_performance] (VkPhysicalDevice dev, auto& prop, auto& feat, auto&)
			{
				const bool	is_gpu		  = (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU or
											 prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
				const bool	is_integrated = prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

				//const BytesU global_mem   = CalcTotalMemory( mem );
																										// magic function:
				const float	perf	=	//float(global_mem.Mb()) / 1024.0f +								// commented because of bug on Intel (incorrect heap size)
										float(prop.limits.maxComputeSharedMemorySize >> 10) / 64.0f +		// big local cache is good
										float(is_gpu and not is_integrated ? 4 : 1) +						// 
										float(prop.limits.maxComputeWorkGroupInvocations) / 1024.0f +		// 
										float(feat.tessellationShader + feat.geometryShader);
				
				if ( perf > max_performance ) {
					max_performance		= perf;
					high_perf_device	= dev;
				}

				if ( not any_device )
					any_device = dev;
			});

		_vkPhysicalDevice = (high_perf_device ? high_perf_device : any_device);
		CHECK_ERR( _vkPhysicalDevice );

		return true;
	}
	
/*
=================================================
	SetPhysicalDevice
=================================================
*/
	bool  VulkanDeviceInitializer::SetPhysicalDevice (VkPhysicalDevice newPhysicalDevice)
	{
		CHECK_ERR( _vkInstance );
		CHECK_ERR( not _vkLogicalDevice );
		CHECK_ERR( newPhysicalDevice );

		_vkPhysicalDevice = newPhysicalDevice;
		return true;
	}
	
/*
=================================================
	DeviceFeatures
=================================================
*/
namespace {
	struct DeviceFeatures
	{
		VkPhysicalDeviceFeatures								main;
		//VkDeviceGeneratedCommandsFeaturesNVX					generatedCommands;
		VkPhysicalDeviceMultiviewFeatures						multiview;
		//VkPhysicalDeviceShaderAtomicInt64FeaturesKHR			shaderAtomicI64;
		VkPhysicalDevice8BitStorageFeaturesKHR					storage8Bit;
		VkPhysicalDevice16BitStorageFeatures					storage16Bit;
		VkPhysicalDeviceSamplerYcbcrConversionFeatures			samplerYcbcrConversion;
		VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT		blendOpAdvanced;
		VkPhysicalDeviceConditionalRenderingFeaturesEXT			conditionalRendering;
		VkPhysicalDeviceShaderDrawParameterFeatures				shaderDrawParameters;

		#ifdef VK_NV_mesh_shader
		VkPhysicalDeviceMeshShaderFeaturesNV					meshShader;
		#endif

		VkPhysicalDeviceDescriptorIndexingFeaturesEXT			descriptorIndexing;
		//VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT		vertexAttribDivisor;
		//VkPhysicalDeviceASTCDecodeFeaturesEXT					astcDecode;
		
		#ifdef VK_KHR_vulkan_memory_model
		VkPhysicalDeviceVulkanMemoryModelFeaturesKHR			memoryModel;
		#endif

		#ifdef VK_EXT_inline_uniform_block
		VkPhysicalDeviceInlineUniformBlockFeaturesEXT			inlineUniformBlock;
		#endif

		#ifdef VK_NV_representative_fragment_test
		VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV	representativeFragmentTest;
		#endif

		//VkPhysicalDeviceExclusiveScissorFeaturesNV			exclusiveScissorTest;
		//VkPhysicalDeviceCornerSampledImageFeaturesNV			cornerSampledImage;
		//VkPhysicalDeviceComputeShaderDerivativesFeaturesNV	computeShaderDerivatives;

		#ifdef VK_NV_fragment_shader_barycentric
		VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV		fragmentShaderBarycentric;
		#endif

		#ifdef VK_NV_shader_image_footprint
		VkPhysicalDeviceShaderImageFootprintFeaturesNV			shaderImageFootprint;
		#endif

		#ifdef VK_NV_shading_rate_image
		VkPhysicalDeviceShadingRateImageFeaturesNV				shadingRateImage;
		#endif
		
		#ifdef VK_KHR_shader_float16_int8
		VkPhysicalDeviceShaderFloat16Int8FeaturesKHR			shaderFloat16Int8;
		#endif

		#ifdef VK_KHR_timeline_semaphore
		VkPhysicalDeviceTimelineSemaphoreFeaturesKHR			timelineSemaphore;
		#endif
		
		#ifdef VK_KHR_buffer_device_address
		VkPhysicalDeviceBufferDeviceAddressFeaturesKHR			bufferDeviceAddress;
		#endif
		
		#ifdef VK_KHR_shader_atomic_int64
		VkPhysicalDeviceShaderAtomicInt64FeaturesKHR			shaderAtomicInt64;
		#endif

		DeviceFeatures () {
			std::memset( this, 0, sizeof(*this) );
		}
	};

	struct EnabledExtensions
	{
		bool	multiview						: 1;
		bool	storage8Bit						: 1;
		bool	storage16Bit					: 1;
		bool	samplerYcbcr					: 1;
		bool	blendOpAdvanced					: 1;
		bool	descriptorIndexing				: 1;
		bool	meshShaderNV					: 1;
		bool	memoryModel						: 1;
		bool	inlineUniformBlock				: 1;
		bool	representativeFragmentTestNV	: 1;
		bool	fragmentShaderBarycentricNV		: 1;
		bool	imageFootprintNV				: 1;
		bool	shadingRateImageNV				: 1;
		bool	shaderFloat16Int8				: 1;
		bool	timelineSemaphore				: 1;
		bool	bufferDeviceAddress				: 1;
		bool	shaderAtomicInt64				: 1;
		
		EnabledExtensions () {
			std::memset( this, 0, sizeof(*this) );
		}
	};
}
/*
=================================================
	SetupDeviceFeatures
----
	enable all available features for extensions
=================================================
*/
namespace {
	bool  SetupDeviceFeatures (VkPhysicalDevice pdev, DeviceFeatures &features, void** nextExt, ArrayView<const char*> extensions, const VulkanDevice::InstanceVersion &version)
	{
		VkPhysicalDeviceFeatures2	feat2		= {};
		void **						next_feat	= &feat2.pNext;;
		feat2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		
		EnabledExtensions	enabled;
		const bool			is_vk_1_1	= (version >= VulkanDevice::InstanceVersion{1,1});
		const bool			is_vk_1_2	= (version >= VulkanDevice::InstanceVersion{1,2});
		
		Unused( is_vk_1_1, is_vk_1_2 );

		for (StringView ext : extensions)
		{
			#ifdef VK_KHR_multiview
			if ( ext == VK_KHR_MULTIVIEW_EXTENSION_NAME )					{ enabled.multiview = true;						continue; }
			#endif
			#ifdef VK_KHR_8bit_storage
			if ( ext == VK_KHR_8BIT_STORAGE_EXTENSION_NAME )				{ enabled.storage8Bit = true;					continue; }
			#endif
			#ifdef VK_KHR_16bit_storage
			if ( ext == VK_KHR_16BIT_STORAGE_EXTENSION_NAME )				{ enabled.storage16Bit = true;					continue; }
			#endif
			#ifdef VK_KHR_sampler_ycbcr_conversion
			if ( ext == VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME )	{ enabled.samplerYcbcr = true;					continue; }
			#endif
			#ifdef VK_EXT_blend_operation_advanced
			if ( ext == VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME )	{ enabled.blendOpAdvanced = true;				continue; }
			#endif
			#ifdef VK_EXT_descriptor_indexing
			if ( ext == VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME )			{ enabled.descriptorIndexing = true;			continue; }
			#endif
			#ifdef VK_NV_mesh_shader
			if ( ext == VK_NV_MESH_SHADER_EXTENSION_NAME )					{ enabled.meshShaderNV = true;					continue; }
			#endif
			#ifdef VK_KHR_vulkan_memory_model
			if ( ext == VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME )			{ enabled.memoryModel = true;					continue; }
			#endif
			#ifdef VK_EXT_inline_uniform_block
			if ( ext == VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME )		{ enabled.inlineUniformBlock = true;			continue; }
			#endif
			#ifdef VK_NV_representative_fragment_test
			if ( ext == VK_NV_REPRESENTATIVE_FRAGMENT_TEST_EXTENSION_NAME )	{ enabled.representativeFragmentTestNV = true;	continue; }
			#endif
			#ifdef VK_NV_fragment_shader_barycentric
			if ( ext == VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME )	{ enabled.fragmentShaderBarycentricNV = true;	continue; }
			#endif
			#ifdef VK_NV_shader_image_footprint
			if ( ext == VK_NV_SHADER_IMAGE_FOOTPRINT_EXTENSION_NAME )		{ enabled.imageFootprintNV = true;				continue; }
			#endif
			#ifdef VK_NV_shading_rate_image
			if ( ext == VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME )			{ enabled.shadingRateImageNV = true;			continue; }
			#endif
			#ifdef VK_KHR_shader_float16_int8
			if ( ext == VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME )			{ enabled.shaderFloat16Int8 = true;				continue; }
			#endif
			#ifdef VK_KHR_timeline_semaphore
			if ( ext == VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME )			{ enabled.timelineSemaphore = true;				continue; }
			#endif
			#ifdef VK_KHR_buffer_device_address
			if ( ext == VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME )		{ enabled.bufferDeviceAddress = true;			continue; }
			#endif
			#ifdef VK_KHR_shader_atomic_int64
			if ( ext == VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME )			{ enabled.shaderAtomicInt64 = true;				continue; }
			#endif
		}
		
		#ifdef VK_KHR_multiview
		if ( enabled.multiview )
		{
			*next_feat	= *nextExt		= &features.multiview;
			next_feat	= nextExt		= &features.multiview.pNext;
			features.multiview.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
		}
		#endif
		#ifdef VK_KHR_8bit_storage
		if ( enabled.storage8Bit )
		{
			*next_feat	= *nextExt		= &features.storage8Bit;
			next_feat	= nextExt		= &features.storage8Bit.pNext;
			features.storage8Bit.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR;
		}
		#endif
		#ifdef VK_KHR_16bit_storage
		if ( enabled.storage16Bit )
		{
			*next_feat	= *nextExt		= &features.storage16Bit;
			next_feat	= nextExt		= &features.storage16Bit.pNext;
			features.storage16Bit.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
		}
		#endif
		#ifdef VK_KHR_sampler_ycbcr_conversion
		if ( enabled.samplerYcbcr )
		{
			*next_feat	= *nextExt					= &features.samplerYcbcrConversion;
			next_feat	= nextExt					= &features.samplerYcbcrConversion.pNext;
			features.samplerYcbcrConversion.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES;
		}
		#endif
		#ifdef VK_EXT_blend_operation_advanced
		if ( enabled.blendOpAdvanced )
		{
			*next_feat	= *nextExt			= &features.blendOpAdvanced;
			next_feat	= nextExt			= &features.blendOpAdvanced.pNext;
			features.blendOpAdvanced.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT;
		}
		#endif
		#ifdef VK_EXT_descriptor_indexing
		if ( enabled.descriptorIndexing or is_vk_1_2 )
		{
			*next_feat	= *nextExt				= &features.descriptorIndexing;
			next_feat	= nextExt				= &features.descriptorIndexing.pNext;
			features.descriptorIndexing.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
		}
		#endif
		#ifdef VK_NV_mesh_shader
		if ( enabled.meshShaderNV )
		{
			*next_feat	= *nextExt		= &features.meshShader;
			next_feat	= nextExt		= &features.meshShader.pNext;
			features.meshShader.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
		}
		#endif
		#ifdef VK_KHR_vulkan_memory_model
		if ( enabled.memoryModel )
		{
			*next_feat	= *nextExt		= &features.memoryModel;
			next_feat	= nextExt		= &features.memoryModel.pNext;
			features.memoryModel.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES_KHR;
		}
		#endif
		#ifdef VK_EXT_inline_uniform_block
		if ( enabled.inlineUniformBlock )
		{
			*next_feat	= *nextExt				= &features.inlineUniformBlock;
			next_feat	= nextExt				= &features.inlineUniformBlock.pNext;
			features.inlineUniformBlock.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES_EXT;
		}
		#endif
		#ifdef VK_NV_representative_fragment_test
		if ( enabled.representativeFragmentTestNV )
		{
			*next_feat	= *nextExt						= &features.representativeFragmentTest;
			next_feat	= nextExt						= &features.representativeFragmentTest.pNext;
			features.representativeFragmentTest.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV;
		}
		#endif
		#ifdef VK_NV_fragment_shader_barycentric
		if ( enabled.fragmentShaderBarycentricNV )
		{
			*next_feat	= *nextExt						= &features.fragmentShaderBarycentric;
			next_feat	= nextExt						= &features.fragmentShaderBarycentric.pNext;
			features.fragmentShaderBarycentric.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV;
		}
		#endif
		#ifdef VK_NV_shader_image_footprint
		if ( enabled.imageFootprintNV )
		{
			*next_feat	= *nextExt				= &features.shaderImageFootprint;
			next_feat	= nextExt				= &features.shaderImageFootprint.pNext;
			features.shaderImageFootprint.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV;
		}
		#endif
		#ifdef VK_NV_shading_rate_image
		if ( enabled.shadingRateImageNV )
		{
			*next_feat	= *nextExt			= &features.shadingRateImage;
			next_feat	= nextExt			= &features.shadingRateImage.pNext;
			features.shadingRateImage.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV;
		}
		#endif
		#ifdef VK_KHR_shader_float16_int8
		if ( enabled.shaderFloat16Int8 or is_vk_1_2 )
		{
			*next_feat	= *nextExt			= &features.shaderFloat16Int8;
			next_feat	= nextExt			= &features.shaderFloat16Int8.pNext;
			features.shaderFloat16Int8.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR;
		}
		#endif
		#ifdef VK_KHR_timeline_semaphore
		if ( enabled.timelineSemaphore or is_vk_1_2 )
		{
			*next_feat	= *nextExt			= &features.timelineSemaphore;
			next_feat	= nextExt			= &features.timelineSemaphore.pNext;
			features.timelineSemaphore.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;
		}
		#endif
		#ifdef VK_KHR_buffer_device_address
		if ( enabled.bufferDeviceAddress or is_vk_1_2 )
		{
			*next_feat	= *nextExt			= &features.bufferDeviceAddress;
			next_feat	= nextExt			= &features.bufferDeviceAddress.pNext;
			features.bufferDeviceAddress.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
		}
		#endif
		#ifdef VK_KHR_shader_atomic_int64
		if ( enabled.shaderAtomicInt64 or is_vk_1_2 )
		{
			*next_feat	= *nextExt			= &features.shaderAtomicInt64;
			next_feat	= nextExt			= &features.shaderAtomicInt64.pNext;
			features.shaderAtomicInt64.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES_KHR;
		}
		#endif
		
		*next_feat	= *nextExt				= &features.shaderDrawParameters;
		next_feat	= nextExt				= &features.shaderDrawParameters.pNext;
		features.shaderDrawParameters.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES;

		vkGetPhysicalDeviceFeatures2KHR( pdev, OUT &feat2 );
		return true;
	}
}
/*
=================================================
	CreateLogicalDevice
=================================================
*/
	bool  VulkanDeviceInitializer::CreateLogicalDevice (ArrayView<QueueCreateInfo> queues, ArrayView<const char*> extensions)
	{
		CHECK_ERR( _vkPhysicalDevice );
		CHECK_ERR( not _vkLogicalDevice );

		CHECK_ERR( _SetupQueues( queues ));
		
		// setup device create info
		VkDeviceCreateInfo		device_info	= {};
		void **					next_ext	= const_cast<void **>( &device_info.pNext );
		device_info.sType		= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;


		// setup extensions
		Array<const char *>		device_extensions;
		ArrayView<const char*>	default_dev_ext = _GetDeviceExtensions( _vkVersion );
		device_extensions.assign( default_dev_ext.begin(), default_dev_ext.end() );
		device_extensions.insert( device_extensions.end(), extensions.begin(), extensions.end() );

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
				auto&	qinfo = queue_infos[ q.familyIndex ];
				auto&	prior = priorities[ q.familyIndex ];

				CHECK_ERR( qinfo.queueCount < priorities[0].capacity() );

				q.queueIndex = qinfo.queueCount++;
				prior.push_back( q.priority );
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
		DeviceFeatures	features;
		vkGetPhysicalDeviceFeatures( _vkPhysicalDevice, OUT &features.main );
		device_info.pEnabledFeatures = &features.main;
		
		if ( _vkVersion >= InstanceVersion{1,1} or HasInstanceExtension( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ))
		{
			CHECK_ERR( SetupDeviceFeatures( _vkPhysicalDevice, features, INOUT next_ext, device_extensions, _vkVersion ));
		}

		VK_CHECK( vkCreateDevice( _vkPhysicalDevice, &device_info, null, OUT &_vkLogicalDevice ));
		

		CHECK_ERR( VulkanLoader::LoadDevice( _vkLogicalDevice, OUT _deviceFnTable ));

		for (auto& q : _vkQueues) {
			vkGetDeviceQueue( _vkLogicalDevice, q.familyIndex, q.queueIndex, OUT &q.handle );
		}
		
		for (auto* ext : device_extensions) {
			_deviceExtensions.insert( ext );
		}
		
		CHECK_ERR( _InitDeviceFeatures() );
		CHECK_ERR( _SetupQueueTypes() );
		return true;
	}

/*
=================================================
	SetLogicalDevice
=================================================
*/
	bool  VulkanDeviceInitializer::SetLogicalDevice (VkDevice newDevice, ArrayView<const char*> extensions)
	{
		CHECK_ERR( _vkPhysicalDevice );
		CHECK_ERR( not _vkLogicalDevice );
		CHECK_ERR( not _vkQueues.empty() );
		CHECK_ERR( newDevice );

		_vkLogicalDevice = newDevice;

		CHECK_ERR( VulkanLoader::LoadDevice( _vkLogicalDevice, OUT _deviceFnTable ));

		for (auto ext : extensions) {
			_deviceExtensions.insert( ext );
		}

		// TODO: get queues

		CHECK_ERR( _InitDeviceFeatures() );
		CHECK_ERR( _SetupQueueTypes() );
		return true;
	}
	
/*
=================================================
	DestroyLogicalDevice
=================================================
*/
	bool  VulkanDeviceInitializer::DestroyLogicalDevice ()
	{
		if ( not _vkLogicalDevice )
			return false;
		
		vkDestroyDevice( _vkLogicalDevice, null );
		VulkanLoader::ResetDevice( OUT _deviceFnTable );

		_vkLogicalDevice = VK_NULL_HANDLE;
		_vkQueues.clear();
		
		_deviceExtensions.clear();

		FG_LOGD( "Destroyed Vulkan logical device" );
		return true;
	}
	
/*
=================================================
	_UpdateInstanceVersion
=================================================
*/
	void  VulkanDeviceInitializer::_UpdateInstanceVersion (uint ver)
	{
		_vkVersion = { VK_VERSION_MAJOR(ver), VK_VERSION_MINOR(ver) };

		// Vulkan loader has backward compatibility for some extensions for example:
		// 'VK_KHR_get_physical_device_properties2' extension added to core 1.1 and function 'vkGetPhysicalDeviceFeatures2KHR' may be removed in 1.1+,
		// so you need to use 'vkGetPhysicalDeviceFeatures2' function in 1.1+.
		// But engine uses only 'vkGetPhysicalDeviceFeatures2KHR' and vulkan loader copy pointer from 'vkGetPhysicalDeviceFeatures2' to 'vkGetPhysicalDeviceFeatures2KHR'.
		// The problem is if you use headers for 1.0 and actual version is 1.1+ then 'vkGetPhysicalDeviceFeatures2' is not presented in headers,
		// loader can not copy pointer from 'vkGetPhysicalDeviceFeatures2' and backward compatibility will be broken.
		bool	problem_detected = false;

	#if defined(VK_VERSION_1_2)
		if ( _vkVersion > InstanceVersion{1,2} )
			problem_detected = true;

	#elif defined(VK_VERSION_1_1)
		if ( _vkVersion > InstanceVersion{1,1} )
			problem_detected = true;

	#else
		if ( _vkVersion > InstanceVersion{1,0} )
			problem_detected = true;
	#endif

		if ( problem_detected )
			FG_LOGI( "Current Vulkan version is greater than headers version, some extensions may be missed" );
	}

/*
=================================================
	_InitDeviceFeatures
=================================================
*/
	bool  VulkanDeviceInitializer::_InitDeviceFeatures ()
	{
		vkGetPhysicalDeviceFeatures( _vkPhysicalDevice, OUT &_properties.features );
		vkGetPhysicalDeviceProperties( _vkPhysicalDevice, OUT &_properties.properties );
		vkGetPhysicalDeviceMemoryProperties( _vkPhysicalDevice, OUT &_properties.memoryProperties );

		// get api version
		{
			_vkVersion.major = VK_VERSION_MAJOR( _properties.properties.apiVersion );
			_vkVersion.minor = VK_VERSION_MINOR( _properties.properties.apiVersion );
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
		_features.bindMemory2				= _vkVersion >= InstanceVersion{1,1} or (HasDeviceExtension( VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME ) and HasDeviceExtension( VK_KHR_BIND_MEMORY_2_EXTENSION_NAME ));
		#endif
		#ifdef VK_KHR_dedicated_allocation
		_features.dedicatedAllocation		= _vkVersion >= InstanceVersion{1,1} or HasDeviceExtension( VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_descriptor_update_template
		_features.descriptorUpdateTemplate	= _vkVersion >= InstanceVersion{1,1} or HasDeviceExtension( VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_maintenance1
		const bool	has_maintenance1		= _vkVersion >= InstanceVersion{1,1} or HasDeviceExtension( VK_KHR_MAINTENANCE1_EXTENSION_NAME );
		_features.create2DArrayCompatible	= has_maintenance1;
		_features.commandPoolTrim			= has_maintenance1;
		_features.array2DCompatible			= has_maintenance1;
		#endif
		#ifdef VK_KHR_maintenance2
		const bool	has_maintenance2		= _vkVersion >= InstanceVersion{1,1} or HasDeviceExtension( VK_KHR_MAINTENANCE2_EXTENSION_NAME );
		_features.imageViewUsage			= has_maintenance2;
		_features.blockTexelView			= has_maintenance2;
		#endif
		#ifdef VK_KHR_maintenance3
		_features.maintenance3				= _vkVersion >= InstanceVersion{1,1} or HasDeviceExtension( VK_KHR_MAINTENANCE3_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_device_group
		_features.dispatchBase				= _vkVersion >= InstanceVersion{1,1} or HasDeviceExtension( VK_KHR_DEVICE_GROUP_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_create_renderpass2
		_features.renderPass2				= _vkVersion >= InstanceVersion{1,2} or HasDeviceExtension( VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_sampler_mirror_clamp_to_edge
		_features.samplerMirrorClamp		= _vkVersion >= InstanceVersion{1,2} or HasDeviceExtension( VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_draw_indirect_count
		_features.drawIndirectCount			= _vkVersion >= InstanceVersion{1,2} or HasDeviceExtension( VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME );
		#endif
		#ifdef VK_EXT_descriptor_indexing
		_features.descriptorIndexing		= _vkVersion >= InstanceVersion{1,2} or HasDeviceExtension( VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME );
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
		#ifdef VK_NV_shader_image_footprint
		_features.imageFootprintNV			= HasDeviceExtension( VK_NV_SHADER_IMAGE_FOOTPRINT_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_shader_clock
		_features.shaderClock				= HasDeviceExtension( VK_KHR_SHADER_CLOCK_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_shader_float16_int8
		_features.float16Arithmetic			= _vkVersion >= InstanceVersion{1,2} or HasDeviceExtension( VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_timeline_semaphore
		_features.timelineSemaphore			= _vkVersion >= InstanceVersion{1,2} or HasDeviceExtension( VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME ),
		#endif
		#ifdef VK_KHR_buffer_device_address
		_features.bufferAddress				= _vkVersion >= InstanceVersion{1,2} or HasDeviceExtension( VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME ),
		#endif
		#ifdef VK_KHR_depth_stencil_resolve
		_features.depthStencilResolve		= _vkVersion >= InstanceVersion{1,2} or HasDeviceExtension( VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_shader_atomic_int64
		_features.shaderAtomicInt64			= _vkVersion >= InstanceVersion{1,2} or HasDeviceExtension( VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_spirv_1_4
		_features.spirv14					= _vkVersion >= InstanceVersion{1,2} or HasDeviceExtension( VK_KHR_SPIRV_1_4_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_push_descriptor
		_features.pushDescriptor			= HasDeviceExtension( VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME );
		#endif
		#ifdef VK_KHR_vulkan_memory_model
		_features.memoryModel				= _vkVersion >= InstanceVersion{1,2} or HasDeviceExtension( VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME );
		#endif
		#ifdef VK_EXT_robustness2
		_features.robustness2				= HasDeviceExtension( VK_EXT_ROBUSTNESS_2_EXTENSION_NAME );
		#endif
		#ifdef VK_EXT_sampler_filter_minmax
		_features.samplerFilterMinmax		= _vkVersion >= InstanceVersion{1,2} or HasDeviceExtension( VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME );
		#endif

		// load extensions
		if ( _vkVersion >= InstanceVersion{1,1} or HasInstanceExtension( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ))
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
			#ifdef VK_NV_shader_image_footprint
			if ( _features.imageFootprintNV )
			{
				*next_feat	= &_properties.shaderImageFootprintFeatures;
				next_feat	= &_properties.shaderImageFootprintFeatures.pNext;
				_properties.shaderImageFootprintFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV;
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
			#ifdef VK_EXT_robustness2
			if ( _features.robustness2 )
			{
				*next_feat	= &_properties.robustness2Features;
				next_feat	= &_properties.robustness2Features.pNext;
				_properties.robustness2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
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
			#ifdef VK_NV_shader_image_footprint
			_features.imageFootprintNV		&= (_properties.shaderImageFootprintFeatures.imageFootprint == VK_TRUE);
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
			#ifdef VK_EXT_robustness2
			_features.robustness2			&= !!(_properties.robustness2Features.robustBufferAccess2 | _properties.robustness2Features.robustImageAccess2 | _properties.robustness2Features.nullDescriptor);
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
			//if ( _vkVersion >= InstanceVersion{1,2} )
			//{
			//	*next_props	= &_properties.properties120;
			//	next_props	= &_properties.properties120.pNext;
			//	_properties.properties120.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
			//
			//	*next_props	= &_properties.properties110;
			//	next_props	= &_properties.properties110.pNext;
			//	_properties.properties110.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
			//}
			#endif
			#ifdef VK_VERSION_1_1
			if ( _vkVersion >= InstanceVersion{1,1} )
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
			#ifdef VK_EXT_robustness2
			if ( _features.robustness2 )
			{
				*next_props	= &_properties.robustness2Properties;
				next_props	= &_properties.robustness2Properties.pNext;
				_properties.robustness2Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_EXT;
			}
			#endif
			#ifdef VK_KHR_maintenance3
			if ( _features.maintenance3 )
			{
				*next_props	= &_properties.maintenance3Properties;
				next_props	= &_properties.maintenance3Properties.pNext;
				_properties.maintenance3Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES_KHR;
			}
			#endif
			#ifdef VK_EXT_sampler_filter_minmax
			if ( _features.samplerFilterMinmax )
			{
				*next_props	= &_properties.samplerFilerMinmaxProperties;
				next_props	= &_properties.samplerFilerMinmaxProperties.pNext;
				_properties.samplerFilerMinmaxProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES_EXT;
			}
			#endif
			Unused( next_props );

			vkGetPhysicalDeviceProperties2KHR( GetVkPhysicalDevice(), OUT &props2 );
		}

		FG_LOGI( "Created vulkan device on GPU: "s << _properties.properties.deviceName
			<< "\n  version:                  " << ToString(_vkVersion.major) << '.' << ToString(_vkVersion.minor)
			<< "\n  ---------- 1.1 ----------"
			<< "\n  bindMemory2:              " << ToString( _features.bindMemory2 )
			<< "\n  dedicatedAllocation:      " << ToString( _features.dedicatedAllocation )
			<< "\n  descriptorUpdateTemplate: " << ToString( _features.descriptorUpdateTemplate )
			<< "\n  imageViewUsage:           " << ToString( _features.imageViewUsage )
			<< "\n  create2DArrayCompatible:  " << ToString( _features.create2DArrayCompatible )
			<< "\n  commandPoolTrim:          " << ToString( _features.commandPoolTrim )
			<< "\n  dispatchBase:             " << ToString( _features.dispatchBase )
			<< "\n  array2DCompatible:        " << ToString( _features.array2DCompatible )
			<< "\n  blockTexelView:           " << ToString( _features.blockTexelView )
			<< "\n  maintenance3:             " << ToString( _features.maintenance3 )
			<< "\n  ---------- 1.2 ----------"
			<< "\n  samplerMirrorClamp:       " << ToString( _features.samplerMirrorClamp )
			<< "\n  shaderAtomicInt64:        " << ToString( _features.shaderAtomicInt64 )
			<< "\n  float16Arithmetic:        " << ToString( _features.float16Arithmetic )
			<< "\n  bufferAddress:            " << ToString( _features.bufferAddress )
			<< "\n  descriptorIndexing:       " << ToString( _features.descriptorIndexing )
			<< "\n  renderPass2:              " << ToString( _features.renderPass2 )
			<< "\n  depthStencilResolve:      " << ToString( _features.depthStencilResolve )
			<< "\n  drawIndirectCount:        " << ToString( _features.drawIndirectCount )
			<< "\n  spirv14:                  " << ToString( _features.spirv14 )
			<< "\n  memoryModel:              " << ToString( _features.memoryModel )
			<< "\n  samplerFilterMinmax       " << ToString( _features.samplerFilterMinmax )
			<< "\n  ---------- Ext ----------"
			<< "\n  surface:                  " << ToString( _features.surface )
			<< "\n  surfaceCaps2:             " << ToString( _features.surfaceCaps2 )
			<< "\n  swapchain:                " << ToString( _features.swapchain )
			<< "\n  debug_utils:              " << ToString( _features.debugUtils )
			<< "\n  meshShaderNV:             " << ToString( _features.meshShaderNV )
			<< "\n  rayTracingNV:             " << ToString( _features.rayTracingNV )
			<< "\n  shadingRateImageNV:       " << ToString( _features.shadingRateImageNV )
			//<< "\n  inlineUniformBlock:       " << ToString( _features.inlineUniformBlock )
			<< "\n  shaderClock:              " << ToString( _features.shaderClock )
			<< "\n  timelineSemaphore:        " << ToString( _features.timelineSemaphore )
			<< "\n  pushDescriptor:           " << ToString( _features.pushDescriptor )
			<< "\n  robustness2:              " << ToString( _features.robustness2 )
			<< "\n  ----------"
		);

		VulkanLoader::SetupDeviceBackwardCompatibility( _properties.properties.apiVersion, INOUT _deviceFnTable );
		return true;
	}
	
/*
=================================================
	_SetupQueueTypes
=================================================
*/
	bool  VulkanDeviceInitializer::_SetupQueueTypes ()
	{
		for (VQueueType t = Zero; t < VQueueType::_Count; t = VQueueType(uint(t) + 1))
		{
			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case VQueueType::Graphics :			_AddGraphicsQueue();		break;
				case VQueueType::AsyncCompute :		_AddAsyncComputeQueue();	break;
				case VQueueType::AsyncTransfer :	_AddAsyncTransferQueue();	break;
				case VQueueType::_Count :
				case VQueueType::Unknown :
				default :							CHECK( !"unknown queue type" );
			}
			END_ENUM_CHECKS();
		}
		return true;
	}
	
	
/*
=================================================
	IsUniqueQueue
=================================================
*/
namespace {
	ND_ bool  IsUniqueQueue (VulkanDevice::VQueuePtr ptr, ArrayView<VulkanDevice::VQueuePtr> qtypes)
	{
		for (auto& q : qtypes) {
			if ( q == ptr )
				return false;
		}
		return true;
	}
}
/*
=================================================
	_AddGraphicsQueue
=================================================
*/
	bool  VulkanDeviceInitializer::_AddGraphicsQueue ()
	{
		VQueuePtr	best_match;
		VQueuePtr	compatible;
		VQueuePtr	best_presentable;
		VQueuePtr	presentable;

		for (auto& queue : _vkQueues)
		{
			const bool	is_unique		= IsUniqueQueue( &queue, _queueTypes );
			const bool	has_graphics	= AllBits( queue.familyFlags, VK_QUEUE_GRAPHICS_BIT );
			VkBool32	supports_present = false;

			if ( has_graphics )
			{
				if ( _vkSurface )
				{
					VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _vkPhysicalDevice, queue.familyIndex, _vkSurface, OUT &supports_present ));
					presentable = &queue;
				}

				compatible = &queue;

				if ( is_unique )
				{
					if ( supports_present )
						best_presentable = &queue;

					best_match = &queue;
					break;
				}
			}
		}
		
		if ( best_presentable )
			best_match = best_presentable;
		
		if ( presentable )
			best_match = presentable;

		if ( not best_match )
			best_match = compatible;

		if ( not best_match )
			return false;
		
		_queueTypes[ uint(VQueueType::Graphics) ] = best_match;
		return true;
	}
	
/*
=================================================
	_AddAsyncComputeQueue
=================================================
*/
	bool  VulkanDeviceInitializer::_AddAsyncComputeQueue ()
	{
		VQueuePtr	unique;
		VQueuePtr	best_match;
		VQueuePtr	compatible;

		for (auto& queue : _vkQueues)
		{
			const bool	is_unique		= IsUniqueQueue( &queue, _queueTypes );
			const bool	has_compute		= AllBits( queue.familyFlags, VK_QUEUE_COMPUTE_BIT );
			const bool	has_graphics	= AllBits( queue.familyFlags, VK_QUEUE_GRAPHICS_BIT );

			// compute without graphics
			if ( has_compute and not has_graphics )
			{
				compatible = &queue;

				if ( is_unique ) {
					best_match = &queue;
					break;
				}
			}
			else
			
			// any unique queue that supports compute
			if ( (has_compute or has_graphics) and is_unique )
			{
				unique = &queue;
			}
		}

		// unique compute/graphics queue is better than non-unique compute queue
		if ( not best_match )
			best_match = unique;
		
		if ( not best_match )
			best_match = compatible;
		
		if ( not best_match )
			return false;

		_queueTypes[ uint(VQueueType::AsyncCompute) ] = best_match;
		return true;
	}
	
/*
=================================================
	_AddAsyncTransferQueue
=================================================
*/
	bool  VulkanDeviceInitializer::_AddAsyncTransferQueue ()
	{
		VQueuePtr	unique;
		VQueuePtr	best_match;
		VQueuePtr	compatible;

		for (auto& queue : _vkQueues)
		{
			const bool	is_unique			= IsUniqueQueue( &queue, _queueTypes );
			const bool	has_transfer		= AllBits( queue.familyFlags, VK_QUEUE_TRANSFER_BIT );
			const bool	supports_transfer	= AnyBits( queue.familyFlags, VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT );

			// transfer without graphics or compute
			if ( has_transfer and not supports_transfer )
			{
				compatible = &queue;

				if ( is_unique ) {
					best_match = &queue;
					break;
				}
			}
			else
			
			// any unique queue that supports transfer
			if ( (has_transfer or supports_transfer) and is_unique )
			{
				unique = &queue;
			}
		}
		
		// unique compute/graphics queue is better than non-unique transfer queue
		if ( not best_match )
			best_match = unique;
		
		if ( not best_match )
			best_match = compatible;
		
		if ( not best_match )
			return false;

		_queueTypes[ uint(VQueueType::AsyncTransfer) ] = best_match;
		return true;
	}

/*
=================================================
	_ChooseQueueIndex
=================================================
*/
	bool  VulkanDeviceInitializer::_ChooseQueueIndex (ArrayView<VkQueueFamilyProperties> queueFamilyProps, INOUT VkQueueFlagBits &requiredFlags, OUT uint &familyIndex) const
	{
		// validate required flags
		{
			// if the capabilities of a queue family include VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT,
			// then reporting the VK_QUEUE_TRANSFER_BIT capability separately for that queue family is optional.
			if ( AnyBits( requiredFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ))
				requiredFlags &= ~VK_QUEUE_TRANSFER_BIT;
		}

		Pair<VkQueueFlagBits, uint>		compatible { Zero, UMax };

		for (size_t i = 0; i < queueFamilyProps.size(); ++i)
		{
			const auto&		prop		 = queueFamilyProps[i];
			VkQueueFlagBits	curr_flags	 = BitCast<VkQueueFlagBits>( prop.queueFlags );

			if ( curr_flags == requiredFlags )
			{
				requiredFlags	= curr_flags;
				familyIndex		= uint(i);
				return true;
			}

			if ( AllBits( curr_flags, requiredFlags ) and 
				 (compatible.first == 0 or BitCount(compatible.first) > BitCount(curr_flags)) )
			{
				compatible = { curr_flags, uint(i) };
			}
		}

		if ( compatible.first ) {
			requiredFlags	= compatible.first;
			familyIndex		= compatible.second;
			return true;
		}

		return false;
	}

/*
=================================================
	_SetupQueues
=================================================
*/
	bool  VulkanDeviceInitializer::_SetupQueues (ArrayView<QueueCreateInfo> queues)
	{
		CHECK_ERR( _vkQueues.empty() );
		
		constexpr uint	MaxQueueFamilies = 32;
		using QueueFamilyProperties_t	= FixedArray< VkQueueFamilyProperties, MaxQueueFamilies >;
		using QueueCount_t				= FixedArray< uint, MaxQueueFamilies >;

		uint	count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, OUT &count, null );
		CHECK_ERR( count > 0 );
		
		QueueFamilyProperties_t  queue_family_props;
		queue_family_props.resize( Min( count, queue_family_props.capacity() ));
		vkGetPhysicalDeviceQueueFamilyProperties( _vkPhysicalDevice, OUT &count, OUT queue_family_props.data() );


		// setup default queue
		if ( queues.empty() )
		{
			uint			family_index	= 0;
			VkQueueFlagBits	flags			= VK_QUEUE_GRAPHICS_BIT;

			CHECK_ERR( _ChooseQueueIndex( queue_family_props, INOUT flags, OUT family_index ));

			VQueue	vq;
			vq.type			= VQueueType::Graphics;
			vq.familyIndex	= family_index;
			vq.familyFlags	= flags;
			vq.minImageTransferGranularity = { queue_family_props[family_index].minImageTransferGranularity.width,
											   queue_family_props[family_index].minImageTransferGranularity.height,
											   queue_family_props[family_index].minImageTransferGranularity.depth };
			
			if ( _vkSurface )
			{
				VkBool32	supports_present = false;
				VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _vkPhysicalDevice, family_index, _vkSurface, OUT &supports_present ));
				vq.supportsPresent = supports_present;
			}

			_vkQueues.push_back( std::move(vq) );
			return true;
		}

		QueueCount_t	qcount;	qcount.resize( queue_family_props.size() );

		for (auto& q : queues)
		{
			uint			family_index	= 0;
			VkQueueFlagBits	flags			= q.flags;
			if ( not _ChooseQueueIndex( queue_family_props, INOUT flags, OUT family_index ))
				continue;

			if ( qcount[family_index]++ < queue_family_props[family_index].queueCount )
			{
				VQueue	vq;
				vq.familyIndex	= family_index;
				vq.familyFlags	= flags;
				vq.priority		= q.priority;
				vq.debugName	= q.debugName;
				vq.minImageTransferGranularity = { queue_family_props[family_index].minImageTransferGranularity.width,
												   queue_family_props[family_index].minImageTransferGranularity.height,
												   queue_family_props[family_index].minImageTransferGranularity.depth };
				
				if ( _vkSurface )
				{
					VkBool32	supports_present = false;
					VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _vkPhysicalDevice, family_index, _vkSurface, OUT &supports_present ));
					vq.supportsPresent = supports_present;
				}

				_vkQueues.push_back( std::move(vq) );
			}
		}
		
		CHECK_ERR( _vkQueues.size() );
		return true;
	}

/*
=================================================
	_ValidateInstanceVersion
=================================================
*/
	void  VulkanDeviceInitializer::_ValidateInstanceVersion (INOUT uint &version) const
	{
		const uint	min_ver		= VK_API_VERSION_1_0;
		const uint	old_ver		= version;
		uint		current_ver	= 0;

	#ifdef VK_VERSION_1_1
		VK_CALL( vkEnumerateInstanceVersion( OUT &current_ver ));
	#else
		current_ver = min_ver;
	#endif

		version = Min( Max( version, min_ver ), Max( current_ver, min_ver ));
		
		if ( old_ver != version )
			FG_LOGI( "Vulkan instance version changed to: "s << ToString(VK_VERSION_MAJOR(version)) << '.' << ToString(VK_VERSION_MINOR(version)) );
	}

/*
=================================================
	_ValidateInstanceLayers
=================================================
*/
	void  VulkanDeviceInitializer::_ValidateInstanceLayers (INOUT Array<const char*> &layers) const
	{
		Array<VkLayerProperties> inst_layers;

		// load supported layers
		uint	count = 0;
		VK_CALL( vkEnumerateInstanceLayerProperties( OUT &count, null ));

		if (count == 0)
		{
			layers.clear();
			return;
		}

		inst_layers.resize( count );
		VK_CALL( vkEnumerateInstanceLayerProperties( OUT &count, OUT inst_layers.data() ));


		// validate
		for (auto iter = layers.begin(); iter != layers.end();)
		{
			bool	found = false;

			for (auto& prop : inst_layers)
			{
				if ( StringView(*iter) == prop.layerName ) {
					found = true;
					break;
				}
			}

			if ( not found )
			{
				FG_LOGI( "Removed layer '"s << (*iter) << "'" );

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
	void  VulkanDeviceInitializer::_ValidateInstanceExtensions (INOUT Array<const char*> &extensions) const
	{
		using ExtName_t = StaticString<VK_MAX_EXTENSION_NAME_SIZE>;

		HashSet<ExtName_t>	instance_extensions;


		// load supported extensions
		uint	count = 0;
		VK_CALL( vkEnumerateInstanceExtensionProperties( null, OUT &count, null ));

		if ( count == 0 )
		{
			extensions.clear();
			return;
		}

		Array< VkExtensionProperties >		inst_ext;
		inst_ext.resize( count );

		VK_CALL( vkEnumerateInstanceExtensionProperties( null, OUT &count, OUT inst_ext.data() ));

		for (auto& ext : inst_ext) {
			instance_extensions.insert( StringView(ext.extensionName) );
		}


		// validate
		for (auto iter = extensions.begin(); iter != extensions.end();)
		{
			if ( instance_extensions.find( ExtName_t{*iter} ) == instance_extensions.end() )
			{
				FG_LOGI( "Removed instance extension '"s << (*iter) << "'" );

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
	void  VulkanDeviceInitializer::_ValidateDeviceExtensions (INOUT Array<const char*> &extensions) const
	{
		// load supported device extensions
		uint	count = 0;
		VK_CALL( vkEnumerateDeviceExtensionProperties( _vkPhysicalDevice, null, OUT &count, null ));

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
			bool	found = false;

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
				FG_LOGI( "Removed device extension '"s << (*iter) << "'" );

				iter = extensions.erase( iter );
			}
			else
				++iter;
		}
	}
	
/*
=================================================
	_LogPhysicalDevices
=================================================
*/
	void  VulkanDeviceInitializer::_LogPhysicalDevices () const
	{
		uint								count	= 0;
		FixedArray< VkPhysicalDevice, 16 >	devices;
		
		VK_CALL( vkEnumeratePhysicalDevices( GetVkInstance(), OUT &count, null ));
		CHECK_ERR( count > 0, void());

		devices.resize( count );
		count = uint(devices.size());
		VK_CALL( vkEnumeratePhysicalDevices( GetVkInstance(), OUT &count, OUT devices.data() ));

		for (auto& dev : devices)
		{
			VkPhysicalDeviceProperties	prop = {};
			vkGetPhysicalDeviceProperties( dev, OUT &prop );

			FG_LOGI( "Found Vulkan device: "s << prop.deviceName );
		}
	}

/*
=================================================
	CreateDebugCallback
=================================================
*/
	bool  VulkanDeviceInitializer::CreateDebugCallback (VkDebugUtilsMessageSeverityFlagsEXT severity, DebugReport_t &&callback)
	{
		CHECK_ERR( GetVkInstance() );
		CHECK_ERR( not _debugUtilsMessenger );
	
		if ( not _features.debugUtils )
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
	DestroyDebugCallback
=================================================
*/
	void  VulkanDeviceInitializer::DestroyDebugCallback ()
	{
		if ( GetVkInstance() and _debugUtilsMessenger ) {
			vkDestroyDebugUtilsMessengerEXT( GetVkInstance(), _debugUtilsMessenger, null );
		}

		_debugUtilsMessenger = VK_NULL_HANDLE;
	}
	
/*
=================================================
	GetMemoryTypeIndex
=================================================
*/
	bool VulkanDeviceInitializer::GetMemoryTypeIndex (const uint memoryTypeBits, const VkMemoryPropertyFlags flags, OUT uint &memoryTypeIndex) const
	{
		memoryTypeIndex = UMax;

		auto&	mem_props = GetProperties().memoryProperties;

		for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
		{
			const auto&		mem_type = mem_props.memoryTypes[i];

			if ( ((memoryTypeBits >> i) & 1) == 1		and
				 AllBits( mem_type.propertyFlags, flags ))
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
	bool VulkanDeviceInitializer::CompareMemoryTypes (const uint memoryTypeBits, const VkMemoryPropertyFlags flags, const uint memoryTypeIndex) const
	{
		auto&	mem_props = GetProperties().memoryProperties;

		CHECK_ERR( memoryTypeIndex < mem_props.memoryTypeCount );

		const auto&		mem_type = mem_props.memoryTypes[ memoryTypeIndex ];

		if ( ((memoryTypeBits >> memoryTypeIndex) & 1) == 1	and
			 AllBits( mem_type.propertyFlags, flags ))
		{
			return true;
		}
		return false;
	}

/*
=================================================
	_ObjectTypeToString
=================================================
*/
	StringView  VulkanDeviceInitializer::_ObjectTypeToString (VkObjectType objType)
	{
		BEGIN_ENUM_CHECKS();
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
				
			#ifdef VK_NV_device_generated_commands
			case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV : return "IndirectCommandsLayoutNv";
			#elif defined(VK_NVX_device_generated_commands)
			case VK_OBJECT_TYPE_OBJECT_TABLE_NVX	: return "ObjectTableNvx";
			case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX: return "IndirectCommandsLayoutNvx";
			#endif

			#ifdef VK_NV_ray_tracing
			case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV	: return "AccelerationStructureNv";
			#endif

			#ifdef VK_EXT_debug_report
			case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT	: return "DebugReportCallback";
			#endif

			#ifdef VK_EXT_debug_utils
			case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT	: return "DebugUtilsMessenger";
			#endif

			case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION	: return "SamplerYcBr";
			#ifdef VK_INTEL_performance_query
			case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL: return "PerformanceConfigIntel";
			#endif

			#ifdef VK_EXT_private_data
			case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT		: return "PrivateDataSlot";
			#endif

			#ifdef VK_KHR_deferred_host_operations
			case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR		: return "DeferredOperation";
			#endif

			#ifndef VK_VERSION_1_2
			case VK_OBJECT_TYPE_RANGE_SIZE :
			#endif

			#ifdef VK_KHR_descriptor_update_template
			case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR	: return "DescriptorUpdateTemplate";
			#endif

			case VK_OBJECT_TYPE_UNKNOWN :
			case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT :
			case VK_OBJECT_TYPE_MAX_ENUM :
				break;	// used to shutup compilation warnings
		}
		END_ENUM_CHECKS();
		return "unknown";
	}

/*
=================================================
	_DebugUtilsCallback
=================================================
*/
	VKAPI_ATTR VkBool32 VKAPI_CALL
		VulkanDeviceInitializer::_DebugUtilsCallback (VkDebugUtilsMessageSeverityFlagBitsEXT			messageSeverity,
												 VkDebugUtilsMessageTypeFlagsEXT				/*messageTypes*/,
												 const VkDebugUtilsMessengerCallbackDataEXT*	pCallbackData,
												 void*											pUserData)
	{
		auto* self = static_cast<VulkanDeviceInitializer *>(pUserData);
		
		self->_tempObjectDbgInfos.resize( pCallbackData->objectCount );

		for (uint i = 0; i < pCallbackData->objectCount; ++i)
		{
			auto&	obj = pCallbackData->pObjects[i];

			self->_tempObjectDbgInfos[i] = { _ObjectTypeToString( obj.objectType ),
											 obj.pObjectName ? StringView{obj.pObjectName} : StringView{},
											 obj.objectHandle };
		}
		
		self->_DebugReport({ self->_tempObjectDbgInfos, pCallbackData->pMessage,
							 AllBits( messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
						   });
		return VK_FALSE;
	}

/*
=================================================
	_DebugReport
=================================================
*/
	void  VulkanDeviceInitializer::_DebugReport (const DebugReport &msg)
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
	_GetInstanceExtensions
=================================================
*/
	ArrayView<const char*>  VulkanDeviceInitializer::_GetInstanceExtensions (InstanceVersion ver)
	{
		if ( ver >= InstanceVersion{1,2} )
			return GetInstanceExtensions_v120();

		if ( ver == InstanceVersion{1,1} )
			return GetInstanceExtensions_v110();

		return GetInstanceExtensions_v100();
	}
	
/*
=================================================
	_GetDeviceExtensions
=================================================
*/
	ArrayView<const char*>  VulkanDeviceInitializer::_GetDeviceExtensions (InstanceVersion ver)
	{
		if ( ver >= InstanceVersion{1,2} )
			return GetDeviceExtensions_v120();

		if ( ver == InstanceVersion{1,1} )
			return GetDeviceExtensions_v110();

		return GetDeviceExtensions_v100();
	}

/*
=================================================
	GetRecomendedInstanceLayers
=================================================
*/
	ArrayView<const char*>	VulkanDeviceInitializer::GetRecomendedInstanceLayers ()
	{
		#ifdef PLATFORM_ANDROID
			static const char*	instance_layers[] = {
				"VK_LAYER_KHRONOS_validation",

				// may be unsupported:
				"VK_LAYER_ARM_MGD",
				"VK_LAYER_ARM_mali_perf_doc"
			};
		#else
			static const char*	instance_layers[] = {
				"VK_LAYER_KHRONOS_validation"
			};
		#endif
		return instance_layers;
	}
	
/*
=================================================
	GetInstanceExtensions_v100
=================================================
*/
	ArrayView<const char*>	VulkanDeviceInitializer::GetInstanceExtensions_v100 ()
	{
		static const char*	instance_extensions[] =
		{
			#ifdef VK_KHR_surface
				VK_KHR_SURFACE_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_get_physical_device_properties2
				VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_get_surface_capabilities2
				VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_debug_utils
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			#elif defined VK_EXT_debug_report
				VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_device_group_creation
				VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
			#endif
		};
		return instance_extensions;
	}
	
/*
=================================================
	GetInstanceExtensions_v110
=================================================
*/
	ArrayView<const char*>	VulkanDeviceInitializer::GetInstanceExtensions_v110 ()
	{
		static const char*	instance_extensions[] =
		{
			#ifdef VK_KHR_surface
				VK_KHR_SURFACE_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_get_surface_capabilities2
				VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_debug_utils
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			#endif
		};
		return instance_extensions;
	}
	
/*
=================================================
	GetInstanceExtensions_v120
=================================================
*/
	ArrayView<const char*>	VulkanDeviceInitializer::GetInstanceExtensions_v120 ()
	{
		static const char*	instance_extensions[] =
		{
			#ifdef VK_KHR_surface
				VK_KHR_SURFACE_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_get_surface_capabilities2
				VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_debug_utils
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			#endif
		};
		return instance_extensions;
	}
	
/*
=================================================
	GetDeviceExtensions_v100
=================================================
*/
	ArrayView<const char*>	VulkanDeviceInitializer::GetDeviceExtensions_v100 ()
	{
		static const char *	device_extensions[] =
		{
			#ifdef VK_KHR_swapchain
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_get_memory_requirements2
				VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_bind_memory2
				VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_dedicated_allocation
				VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_multiview
				VK_KHR_MULTIVIEW_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_maintenance1
				VK_KHR_MAINTENANCE1_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_maintenance2
				VK_KHR_MAINTENANCE2_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_maintenance3
				VK_KHR_MAINTENANCE3_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_create_renderpass2
				VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_sampler_ycbcr_conversion
				VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_descriptor_update_template
				VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_device_group
				VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_depth_range_unrestricted
				VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_filter_cubic
				VK_IMG_FILTER_CUBIC_EXTENSION_NAME,
				VK_EXT_FILTER_CUBIC_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_sampler_filter_minmax
				VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_shader_stencil_export
				VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME,	// TODO
			#endif
		};
		return device_extensions;
	}
	
/*
=================================================
	GetDeviceExtensions_v110
=================================================
*/
	ArrayView<const char*>	VulkanDeviceInitializer::GetDeviceExtensions_v110 ()
	{
		static const char *	device_extensions[] =
		{
			#ifdef VK_KHR_swapchain
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_create_renderpass2
				VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_draw_indirect_count
				VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_8bit_storage
				VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_sample_locations
				VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_push_descriptor
				VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_shader_atomic_int64
				VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_shader_float16_int8
				VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_shader_float_controls
				VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_blend_operation_advanced
				VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_maintenance1
				VK_KHR_MAINTENANCE1_EXTENSION_NAME,		// required for VK_EXT_inline_uniform_block
			#endif
			#ifdef VK_EXT_inline_uniform_block
				VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_descriptor_indexing
				VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_memory_budget
				VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_shader_clock
				VK_KHR_SHADER_CLOCK_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_timeline_semaphore
				VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_subgroup_size_control
				VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_performance_query
				VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_filter_cubic
				VK_IMG_FILTER_CUBIC_EXTENSION_NAME,
				VK_EXT_FILTER_CUBIC_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_spirv_1_4
				VK_KHR_SPIRV_1_4_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_depth_stencil_resolve
				VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_buffer_device_address
				VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_depth_range_unrestricted
				VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_sampler_filter_minmax
				VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_shader_stencil_export
				VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME,
			#endif

			// Vendor specific extensions
			#ifdef VK_NV_mesh_shader
				VK_NV_MESH_SHADER_EXTENSION_NAME,
			#endif
			#ifdef VK_NV_shader_image_footprint
				VK_NV_SHADER_IMAGE_FOOTPRINT_EXTENSION_NAME,
			#endif
			#ifdef VK_NV_shading_rate_image
				VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME,
			#endif
			#ifdef VK_NV_fragment_shader_barycentric
				VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME,
			#endif
			#ifdef VK_NV_ray_tracing
				VK_NV_RAY_TRACING_EXTENSION_NAME,
			#endif

			#ifdef VK_AMD_shader_info
				VK_AMD_SHADER_INFO_EXTENSION_NAME,
			#endif
			#ifdef VK_AMD_shader_core_properties
				VK_AMD_SHADER_CORE_PROPERTIES_EXTENSION_NAME,
			#endif
			#ifdef VK_AMD_shader_core_properties2
				VK_AMD_SHADER_CORE_PROPERTIES_2_EXTENSION_NAME,
			#endif
			#ifdef VK_AMD_rasterization_order
				VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME,
			#endif
		};
		return device_extensions;
	}
	
/*
=================================================
	GetDeviceExtensions_v120
=================================================
*/
	ArrayView<const char*>	VulkanDeviceInitializer::GetDeviceExtensions_v120 ()
	{
		static const char *	device_extensions[] =
		{
			#ifdef VK_KHR_swapchain
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_sample_locations
				VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_push_descriptor
				VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_blend_operation_advanced
				VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_maintenance1
				VK_KHR_MAINTENANCE1_EXTENSION_NAME,		// required for VK_EXT_inline_uniform_block
			#endif
			#ifdef VK_EXT_inline_uniform_block
				VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_memory_budget
				VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_shader_clock
				VK_KHR_SHADER_CLOCK_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_subgroup_size_control
				VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME,
			#endif
			#ifdef VK_KHR_performance_query
				VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_filter_cubic
				VK_IMG_FILTER_CUBIC_EXTENSION_NAME,
				VK_EXT_FILTER_CUBIC_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_depth_range_unrestricted
				VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME,
			#endif
			#ifdef VK_EXT_extended_dynamic_state
				VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,	// TODO
			#endif
			#ifdef VK_EXT_shader_stencil_export
				VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME,
			#endif

			// Vendor specific extensions
			#ifdef VK_NV_mesh_shader
				VK_NV_MESH_SHADER_EXTENSION_NAME,
			#endif
			#ifdef VK_NV_shader_image_footprint
				VK_NV_SHADER_IMAGE_FOOTPRINT_EXTENSION_NAME,
			#endif
			#ifdef VK_NV_shading_rate_image
				VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME,
			#endif
			#ifdef VK_NV_fragment_shader_barycentric
				VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME,
			#endif
			#ifdef VK_NV_ray_tracing
				VK_NV_RAY_TRACING_EXTENSION_NAME,
			#endif

			#ifdef VK_AMD_shader_info
				VK_AMD_SHADER_INFO_EXTENSION_NAME,
			#endif
			#ifdef VK_AMD_shader_core_properties
				VK_AMD_SHADER_CORE_PROPERTIES_EXTENSION_NAME,
			#endif
			#ifdef VK_AMD_shader_core_properties2
				VK_AMD_SHADER_CORE_PROPERTIES_2_EXTENSION_NAME,
			#endif
			#ifdef VK_AMD_rasterization_order
				VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME,
			#endif
		};
		return device_extensions;
	}

}	// FGC

#endif	// FG_ENABLE_VULKAN

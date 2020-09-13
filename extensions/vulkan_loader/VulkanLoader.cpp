// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VulkanLoader.h"
#include "VulkanCheckError.h"
#include "stl/Algorithms/Cast.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Containers/Singleton.h"


# if defined(PLATFORM_WINDOWS) or defined(VK_USE_PLATFORM_WIN32_KHR)

#	include "stl/Platforms/WindowsHeader.h"

	using SharedLib_t	= HMODULE;

# else
//#elif defined(PLATFORM_LINUX) or defined(PLATFORM_ANDROID)
#	include <dlfcn.h>
#   include <linux/limits.h>

	using SharedLib_t	= void*;
# endif


namespace FGC
{

#	define VK_LOG	FG_LOGD

#	define VKLOADER_STAGE_FNPOINTER
#	 include "vk_loader/fn_vulkan_lib.h"
#	 include "vk_loader/fn_vulkan_inst.h"
#	undef  VKLOADER_STAGE_FNPOINTER

#	define VKLOADER_STAGE_DUMMYFN
#	 include "vk_loader/fn_vulkan_lib.h"
#	 include "vk_loader/fn_vulkan_inst.h"
#	 include "vk_loader/fn_vulkan_dev.h"
#	undef  VKLOADER_STAGE_DUMMYFN

/*
=================================================
	VulkanLib
=================================================
*/
namespace {
	struct VulkanLib
	{
		SharedLib_t		module		= null;
		VkInstance		instance	= VK_NULL_HANDLE;
		int				refCounter	= 0;
	};
}
/*
=================================================
	Initialize
----
	must be externally synchronized!
=================================================
*/
	bool VulkanLoader::Initialize (NtStringView libName)
	{
		VulkanLib *		lib = Singleton< VulkanLib >();
		
		if ( lib->module and lib->refCounter > 0 )
		{
			++lib->refCounter;
			return true;
		}

#	ifdef PLATFORM_WINDOWS
		if ( not libName.empty() )
			lib->module = ::LoadLibraryA( libName.c_str() );

		if ( lib->module == null )
			lib->module = ::LoadLibraryA( "vulkan-1.dll" );
		
		if ( lib->module == null )
			return false;
		
		// write library path to log
		{
			char	buf[MAX_PATH] = "";
			CHECK( ::GetModuleFileNameA( lib->module, buf, DWORD(CountOf(buf)) ) != FALSE );

			FG_LOGI( "Vulkan library path: \""s << buf << '"' );
		}

		const auto	Load =	[module = lib->module] (OUT auto& outResult, const char *procName, auto dummy)
							{
								using FN = decltype(dummy);
								FN	result = BitCast<FN>( ::GetProcAddress( module, procName ));
								outResult = result ? result : dummy;
							};

#	else
		if ( not libName.empty() )
			lib->module = ::dlopen( libName.c_str(), RTLD_LAZY | RTLD_LOCAL );

		if ( lib->module == null )
			lib->module = ::dlopen( "libvulkan.so", RTLD_LAZY | RTLD_LOCAL );
		
		if ( lib->module == null )
			lib->module = ::dlopen( "libvulkan.so.1", RTLD_LAZY | RTLD_LOCAL );
		
		if ( lib->module == null )
			return false;
		
		// write library path to log
#		ifndef PLATFORM_ANDROID
		{
			char	buf[PATH_MAX] = "";
			CHECK( dlinfo( lib->module, RTLD_DI_ORIGIN, buf ) == 0 );
			
			FG_LOGI( "Vulkan library path: \""s << buf << '"' );
		}
#		endif

		const auto	Load =	[module = lib->module] (OUT auto& outResult, const char *procName, auto dummy)
							{
								using FN = decltype(dummy);
								FN	result = BitCast<FN>( ::dlsym( module, procName ));
								outResult = result ? result : dummy;
							};
#	endif

		++lib->refCounter;
		

#		define VKLOADER_STAGE_GETADDRESS
#		 include "vk_loader/fn_vulkan_lib.h"
#		undef  VKLOADER_STAGE_GETADDRESS

		ASSERT( _var_vkCreateInstance != &Dummy_vkCreateInstance );
		ASSERT( _var_vkGetInstanceProcAddr != &Dummy_vkGetInstanceProcAddr );

		return true;
	}
	
/*
=================================================
	LoadInstance
----
	must be externally synchronized!
	warning: multiple instances are not supported!
=================================================
*/
	bool VulkanLoader::LoadInstance (VkInstance instance)
	{
		VulkanLib *		lib = Singleton< VulkanLib >();

		CHECK_ERR( instance );
		CHECK_ERR( lib->instance == null or lib->instance == instance );
		CHECK_ERR( _var_vkGetInstanceProcAddr != &Dummy_vkGetInstanceProcAddr );

		if ( lib->instance == instance )
			return true;	// functions already loaded for this instance

		lib->instance = instance;

		const auto	Load =	[instance] (OUT auto& outResult, const char *procName, auto dummy)
							{
								using FN = decltype(dummy);
								FN	result = BitCast<FN>( vkGetInstanceProcAddr( instance, procName ));
								outResult = result ? result : dummy;
							};
		
#		define VKLOADER_STAGE_GETADDRESS
#		 include "vk_loader/fn_vulkan_inst.h"
#		undef  VKLOADER_STAGE_GETADDRESS

		return true;
	}
	
/*
=================================================
	LoadDevice
----
	access to the 'vkGetDeviceProcAddr' must be externally synchronized!
=================================================
*/
	bool VulkanLoader::LoadDevice (VkDevice device, OUT VulkanDeviceFnTable &table)
	{
		CHECK_ERR( _var_vkGetDeviceProcAddr != &Dummy_vkGetDeviceProcAddr );

		const auto	Load =	[device] (OUT auto& outResult, const char *procName, auto dummy)
							{
								using FN = decltype(dummy);
								FN	result = BitCast<FN>( vkGetDeviceProcAddr( device, procName ));
								outResult = result ? result : dummy;
							};

#		define VKLOADER_STAGE_GETADDRESS
#		 include "vk_loader/fn_vulkan_dev.h"
#		undef  VKLOADER_STAGE_GETADDRESS

		return true;
	}
	
/*
=================================================
	ResetDevice
=================================================
*/
	void VulkanLoader::ResetDevice (OUT VulkanDeviceFnTable &table)
	{
		const auto	Load =	[] (OUT auto& outResult, const char *, auto dummy) {
								outResult = dummy;
							};

#		define VKLOADER_STAGE_GETADDRESS
#		 include "vk_loader/fn_vulkan_dev.h"
#		undef  VKLOADER_STAGE_GETADDRESS
	}

/*
=================================================
	Unload
----
	must be externally synchronized!
=================================================
*/
	void VulkanLoader::Unload ()
	{
		VulkanLib *		lib = Singleton< VulkanLib >();
		
		ASSERT( lib->refCounter > 0 );

		if ( (--lib->refCounter) != 0 )
			return;
		
#	ifdef PLATFORM_WINDOWS
		if ( lib->module != null )
		{
			::FreeLibrary( lib->module );
		}
#	else
		if ( lib->module != null )
		{
			::dlclose( lib->module );
		}
#	endif

		lib->instance	= null;
		lib->module		= null;
		
		const auto	Load =	[] (OUT auto& outResult, const char *, auto dummy) {
								outResult = dummy;
							};

#		define VKLOADER_STAGE_GETADDRESS
#		 include "vk_loader/fn_vulkan_lib.h"
#		 include "vk_loader/fn_vulkan_inst.h"
#		undef  VKLOADER_STAGE_GETADDRESS
	}

/*
=================================================
	SetupInstanceBackwardCompatibility
=================================================
*/
	void VulkanLoader::SetupInstanceBackwardCompatibility (uint version)
	{
		Unused( version );
		
	#define VK_COMPAT( _dst_, _src_ ) \
		ASSERT( _var_##_src_ != null ); \
		/*ASSERT( _var_##_dst_ == null );*/ \
		_var_##_dst_ = _var_##_src_

	#ifdef VK_VERSION_1_1
		if ( VK_VERSION_MAJOR(version) > 1 or (VK_VERSION_MAJOR(version) == 1 and VK_VERSION_MINOR(version) >= 1) )
		{
		// VK_KHR_get_physical_device_properties2
			VK_COMPAT( vkGetPhysicalDeviceFeatures2KHR,						vkGetPhysicalDeviceFeatures2 );
			VK_COMPAT( vkGetPhysicalDeviceProperties2KHR,					vkGetPhysicalDeviceProperties2 );
			VK_COMPAT( vkGetPhysicalDeviceFormatProperties2KHR,				vkGetPhysicalDeviceFormatProperties2 );
			VK_COMPAT( vkGetPhysicalDeviceImageFormatProperties2KHR,		vkGetPhysicalDeviceImageFormatProperties2 );
			VK_COMPAT( vkGetPhysicalDeviceQueueFamilyProperties2KHR,		vkGetPhysicalDeviceQueueFamilyProperties2 );
			VK_COMPAT( vkGetPhysicalDeviceMemoryProperties2KHR,				vkGetPhysicalDeviceMemoryProperties2 );
			VK_COMPAT( vkGetPhysicalDeviceSparseImageFormatProperties2KHR,	vkGetPhysicalDeviceSparseImageFormatProperties2 );
		}
	#endif
	#ifdef VK_VERSION_1_2
		if ( VK_VERSION_MAJOR(version) > 1 or (VK_VERSION_MAJOR(version) == 1 and VK_VERSION_MINOR(version) >= 2) )
		{
		}
	#endif

	#undef VK_COMPAT
	}
	
/*
=================================================
	SetupDeviceBackwardCompatibility
=================================================
*/
	void VulkanLoader::SetupDeviceBackwardCompatibility (uint version, INOUT VulkanDeviceFnTable &table)
	{
		Unused( version, table );
		
	#define VK_COMPAT( _dst_, _src_ ) \
		ASSERT( table._var_##_src_ != null ); \
		/*ASSERT( table._var_##_dst_ == null );*/ \
		table._var_##_dst_ = table._var_##_src_

	#ifdef VK_VERSION_1_1
		if ( VK_VERSION_MAJOR(version) > 1 or (VK_VERSION_MAJOR(version) == 1 and VK_VERSION_MINOR(version) >= 1) )
		{
		// VK_KHR_maintenance1
			VK_COMPAT( vkTrimCommandPoolKHR, vkTrimCommandPool );

		// VK_KHR_bind_memory2
			VK_COMPAT( vkBindBufferMemory2KHR,	vkBindBufferMemory2 );
			VK_COMPAT( vkBindImageMemory2KHR,	vkBindImageMemory2 );

		// VK_KHR_get_memory_requirements2
			VK_COMPAT( vkGetImageMemoryRequirements2KHR,		vkGetImageMemoryRequirements2 );
			VK_COMPAT( vkGetBufferMemoryRequirements2KHR,		vkGetBufferMemoryRequirements2 );
			VK_COMPAT( vkGetImageSparseMemoryRequirements2KHR,	vkGetImageSparseMemoryRequirements2 );

		// VK_KHR_sampler_ycbcr_conversion
			VK_COMPAT( vkCreateSamplerYcbcrConversionKHR,	vkCreateSamplerYcbcrConversion );
			VK_COMPAT( vkDestroySamplerYcbcrConversionKHR,	vkDestroySamplerYcbcrConversion );

		// VK_KHR_descriptor_update_template
			VK_COMPAT( vkCreateDescriptorUpdateTemplateKHR,		vkCreateDescriptorUpdateTemplate );
			VK_COMPAT( vkDestroyDescriptorUpdateTemplateKHR,	vkDestroyDescriptorUpdateTemplate );
			VK_COMPAT( vkUpdateDescriptorSetWithTemplateKHR,	vkUpdateDescriptorSetWithTemplate );
			
		// VK_KHR_device_group
			VK_COMPAT( vkCmdDispatchBaseKHR, vkCmdDispatchBase );
		}
	#endif
	#ifdef VK_VERSION_1_2
		if ( VK_VERSION_MAJOR(version) > 1 or (VK_VERSION_MAJOR(version) == 1 and VK_VERSION_MINOR(version) >= 2) )
		{
		// VK_KHR_draw_indirect_count
			VK_COMPAT( vkCmdDrawIndirectCountKHR,		 vkCmdDrawIndirectCount );
			VK_COMPAT( vkCmdDrawIndexedIndirectCountKHR, vkCmdDrawIndexedIndirectCountKHR );

		// VK_KHR_create_renderpass2
			VK_COMPAT( vkCreateRenderPass2KHR,		vkCreateRenderPass2 );
			VK_COMPAT( vkCmdBeginRenderPass2KHR,	vkCmdBeginRenderPass2 );
			VK_COMPAT( vkCmdNextSubpass2KHR,		vkCmdNextSubpass2 );
			VK_COMPAT( vkCmdEndRenderPass2KHR,		vkCmdEndRenderPass2 );

		// VK_KHR_timeline_semaphore
			VK_COMPAT( vkGetSemaphoreCounterValueKHR,	vkGetSemaphoreCounterValue );
			VK_COMPAT( vkWaitSemaphoresKHR,				vkWaitSemaphores );
			VK_COMPAT( vkSignalSemaphoreKHR,			vkSignalSemaphore );

		// VK_KHR_buffer_device_address
			VK_COMPAT( vkGetBufferDeviceAddressKHR,					vkGetBufferDeviceAddress );
			VK_COMPAT( vkGetBufferOpaqueCaptureAddressKHR,			vkGetBufferOpaqueCaptureAddress );
			VK_COMPAT( vkGetDeviceMemoryOpaqueCaptureAddressKHR,	vkGetDeviceMemoryOpaqueCaptureAddress );
		}
	#endif
		
	#undef VK_COMPAT
	}
	
/*
=================================================
	VulkanDeviceFn_Init
=================================================
*/
	void VulkanDeviceFn::VulkanDeviceFn_Init (const VulkanDeviceFn &other)
	{
		_table = other._table;
	}
	
	void VulkanDeviceFn::VulkanDeviceFn_Init (VulkanDeviceFnTable *table)
	{
		_table = table;
	}

}	// FGC

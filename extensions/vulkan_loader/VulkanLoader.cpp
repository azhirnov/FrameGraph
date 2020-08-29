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
			lib->module = ::dlopen( libName.c_str(), RTLD_NOW | RTLD_LOCAL );

		if ( lib->module == null )
			lib->module = ::dlopen( "libvulkan.so", RTLD_NOW | RTLD_LOCAL );
		
		if ( lib->module == null )
			lib->module = ::dlopen( "libvulkan.so.1", RTLD_NOW | RTLD_LOCAL );
		
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
	#ifdef VK_VERSION_1_1
		if ( VK_VERSION_MAJOR(version) > 1 or (VK_VERSION_MAJOR(version) == 1 and VK_VERSION_MINOR(version) >= 1) )
		{
		// VK_KHR_get_physical_device_properties2
			_var_vkGetPhysicalDeviceFeatures2KHR					= _var_vkGetPhysicalDeviceFeatures2;
			_var_vkGetPhysicalDeviceProperties2KHR					= _var_vkGetPhysicalDeviceProperties2;
			_var_vkGetPhysicalDeviceFormatProperties2KHR			= _var_vkGetPhysicalDeviceFormatProperties2;
			_var_vkGetPhysicalDeviceImageFormatProperties2KHR		= _var_vkGetPhysicalDeviceImageFormatProperties2;
			_var_vkGetPhysicalDeviceQueueFamilyProperties2KHR		= _var_vkGetPhysicalDeviceQueueFamilyProperties2;
			_var_vkGetPhysicalDeviceMemoryProperties2KHR			= _var_vkGetPhysicalDeviceMemoryProperties2;
			_var_vkGetPhysicalDeviceSparseImageFormatProperties2KHR	= _var_vkGetPhysicalDeviceSparseImageFormatProperties2;
		}
	#endif
	#ifdef VK_VERSION_1_2
		if ( VK_VERSION_MAJOR(version) > 1 or (VK_VERSION_MAJOR(version) == 1 and VK_VERSION_MINOR(version) >= 2) )
		{
		}
	#endif
	}
	
/*
=================================================
	SetupDeviceBackwardCompatibility
=================================================
*/
	void VulkanLoader::SetupDeviceBackwardCompatibility (uint version, INOUT VulkanDeviceFnTable &table)
	{
		// for backward compatibility
	#ifdef VK_VERSION_1_1
		if ( VK_VERSION_MAJOR(version) > 1 or (VK_VERSION_MAJOR(version) == 1 and VK_VERSION_MINOR(version) >= 1) )
		{
		// VK_KHR_maintenance1
			table._var_vkTrimCommandPoolKHR	= table._var_vkTrimCommandPool;

		// VK_KHR_bind_memory2
			table._var_vkBindBufferMemory2KHR	= table._var_vkBindBufferMemory2;
			table._var_vkBindImageMemory2KHR	= table._var_vkBindImageMemory2;

		// VK_KHR_get_memory_requirements2
			table._var_vkGetImageMemoryRequirements2KHR		= table._var_vkGetImageMemoryRequirements2;
			table._var_vkGetBufferMemoryRequirements2KHR		= table._var_vkGetBufferMemoryRequirements2;
			table._var_vkGetImageSparseMemoryRequirements2KHR	= table._var_vkGetImageSparseMemoryRequirements2;

		// VK_KHR_sampler_ycbcr_conversion
			table._var_vkCreateSamplerYcbcrConversionKHR	= table._var_vkCreateSamplerYcbcrConversion;
			table._var_vkDestroySamplerYcbcrConversionKHR	= table._var_vkDestroySamplerYcbcrConversion;

		// VK_KHR_descriptor_update_template
			table._var_vkCreateDescriptorUpdateTemplateKHR	= table._var_vkCreateDescriptorUpdateTemplate;
			table._var_vkDestroyDescriptorUpdateTemplateKHR	= table._var_vkDestroyDescriptorUpdateTemplate;
			table._var_vkUpdateDescriptorSetWithTemplateKHR	= table._var_vkUpdateDescriptorSetWithTemplate;
			
		// VK_KHR_device_group
			table._var_vkCmdDispatchBaseKHR	= table._var_vkCmdDispatchBase;
		}
	#endif
	#ifdef VK_VERSION_1_2
		if ( VK_VERSION_MAJOR(version) > 1 or (VK_VERSION_MAJOR(version) == 1 and VK_VERSION_MINOR(version) >= 2) )
		{
		// VK_KHR_draw_indirect_count
			table._var_vkCmdDrawIndirectCountKHR			= table._var_vkCmdDrawIndirectCount;
			table._var_vkCmdDrawIndexedIndirectCountKHR	= table._var_vkCmdDrawIndexedIndirectCountKHR;

		// VK_KHR_create_renderpass2
			table._var_vkCreateRenderPass2KHR		= table._var_vkCreateRenderPass2;
			table._var_vkCmdBeginRenderPass2KHR	= table._var_vkCmdBeginRenderPass2;
			table._var_vkCmdNextSubpass2KHR		= table._var_vkCmdNextSubpass2;
			table._var_vkCmdEndRenderPass2KHR		= table._var_vkCmdEndRenderPass2;

		// VK_KHR_timeline_semaphore
			table._var_vkGetSemaphoreCounterValueKHR	= table._var_vkGetSemaphoreCounterValue;
			table._var_vkWaitSemaphoresKHR			= table._var_vkWaitSemaphores;
			table._var_vkSignalSemaphoreKHR			= table._var_vkSignalSemaphore;

		// VK_KHR_buffer_device_address
			table._var_vkGetBufferDeviceAddressKHR				= table._var_vkGetBufferDeviceAddress;
			table._var_vkGetBufferOpaqueCaptureAddressKHR			= table._var_vkGetBufferOpaqueCaptureAddress;
			table._var_vkGetDeviceMemoryOpaqueCaptureAddressKHR	= table._var_vkGetDeviceMemoryOpaqueCaptureAddress;
		}
	#endif
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

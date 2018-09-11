// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "framegraph/Public/Config.h"
#include "stl/include/Common.h"


#ifdef FG_VULKAN_STATIC

#	undef VK_NO_PROTOTYPES
#	include <vulkan/vulkan.h>

#else

#	if not defined(VK_NO_PROTOTYPES) and defined(VULKAN_CORE_H_)
#		error invalid configuration, define FG_VULKAN_STATIC or include vulkan.h after this file.
#	endif

#	ifndef VK_NO_PROTOTYPES
#		define VK_NO_PROTOTYPES
#	endif

#	include <vulkan/vulkan.h>


namespace FG
{

#	define VKLOADER_STAGE_DECLFNPOINTER
#	 include "fn_vulkan_lib.h"
#	 include "fn_vulkan_inst.h"
#	undef  VKLOADER_STAGE_DECLFNPOINTER

#	define VKLOADER_STAGE_INLINEFN
#	 include "fn_vulkan_lib.h"
#	 include "fn_vulkan_inst.h"
#	undef  VKLOADER_STAGE_INLINEFN



	//
	// Vulkan Device Functions Table
	//
	struct VulkanDeviceFnTable final
	{
		friend struct VulkanLoader;
		friend class VulkanDeviceFn;

	// variables
	private:
#		define VKLOADER_STAGE_FNPOINTER
#		 include "fn_vulkan_dev.h"
#		undef  VKLOADER_STAGE_FNPOINTER


	// methods
	public:
		VulkanDeviceFnTable () {}

		VulkanDeviceFnTable (const VulkanDeviceFnTable &) = delete;
		VulkanDeviceFnTable (VulkanDeviceFnTable &&) = delete;

		VulkanDeviceFnTable& operator = (const VulkanDeviceFnTable &) = delete;
		VulkanDeviceFnTable& operator = (VulkanDeviceFnTable &&) = delete;
	};



	//
	// Vulkan Device Functions
	//
	class VulkanDeviceFn
	{
	// variables
	private:
		VulkanDeviceFnTable *		_table;

	// methods
	public:
		VulkanDeviceFn () : _table{null} {}
		explicit VulkanDeviceFn (VulkanDeviceFnTable *table) : _table{table} {}

		void VulkanDeviceFn_Init (const VulkanDeviceFn &other);
		void VulkanDeviceFn_Init (VulkanDeviceFnTable *table);

#		define VKLOADER_STAGE_INLINEFN
#		 include "fn_vulkan_dev.h"
#		undef  VKLOADER_STAGE_INLINEFN
	};



	//
	// Vulkan Loader
	//
	struct VulkanLoader final
	{
		VulkanLoader () = delete;

		static bool Initialize (StringView libName = {});
		static void LoadInstance (VkInstance instance);
		static void Unload ();
		
		static void LoadDevice (VkDevice device, OUT VulkanDeviceFnTable &table);
		static void ResetDevice (OUT VulkanDeviceFnTable &table);
	};

}	// FG

#endif	// not FG_VULKAN_STATIC

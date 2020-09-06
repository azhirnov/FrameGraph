// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_ENABLE_VULKAN

# include "framework/Vulkan/VulkanDevice.h"

namespace FGC
{
	
	//
	// Vulkan Surface Helper
	//

	class VulkanSurface final
	{
	public:
		VulkanSurface () = delete;
		~VulkanSurface () = delete;

		ND_ static Array<const char*>	GetRequiredExtensions ();
		
		// Windows
#	if defined(PLATFORM_WINDOWS)
		ND_ static VkSurfaceKHR			CreateWin32Surface (VkInstance instance, void* hinstance, void* hwnd);
#	endif

		// Android
#	if defined(PLATFORM_ANDROID)
		ND_ static VkSurfaceKHR			CreateAndroidSurface (VkInstance instance, void* window);
#	endif
	};

}	// FGC

#endif	// FG_ENABLE_VULKAN

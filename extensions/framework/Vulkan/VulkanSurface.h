// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framework/Vulkan/VulkanDevice.h"

namespace FG
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
#	if defined(PLATFORM_WINDOWS) or defined(VK_USE_PLATFORM_WIN32_KHR)
		ND_ static Array<const char*>	GetWin32Extensions ();
		ND_ static VkSurfaceKHR			CreateWin32Surface (VkInstance instance, void* hinstance, void* hwnd);
#	endif
	};

}	// FG

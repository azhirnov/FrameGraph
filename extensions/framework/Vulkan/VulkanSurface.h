// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
#	if defined(PLATFORM_WINDOWS)
		ND_ static Array<const char*>	GetWin32Extensions ();
		ND_ static VkSurfaceKHR			CreateWin32Surface (VkInstance instance, void* hinstance, void* hwnd);
#	endif
	};

}	// FG

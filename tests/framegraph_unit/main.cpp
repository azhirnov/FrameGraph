// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/VFG.h"
#include "framework/Window/IWindow.h"
#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include <iostream>
#include "UnitTestCommon.h"

using namespace FG;

extern void UnitTest_Main ();
extern void UnitTest_FGMain (const FGThreadPtr &fg);


int main ()
{
	UnitTest_Main();

	{
		VulkanDeviceExt		vulkan;
		WindowPtr			window;
		FrameGraphPtr		fg_instance;
		FGThreadPtr			fg_thread;
		const uint2			wnd_size{ 800, 600 };
		
		// initialize window
		{
			#if defined( FG_ENABLE_GLFW )
				window.reset( new WindowGLFW() );

			#elif defined( FG_ENABLE_SDL2 )
				window.reset( new WindowSDL2() );
			
			#elif defined(FG_ENABLE_SFML)
				window.reset( new WindowSFML() );

			#else
			#	error Unknown window library!
			#endif

			CHECK_ERR( window->Create( wnd_size, "Test" ));
		}

		// initialize vulkan device
		{
			TEST( vulkan.Create( window->GetVulkanSurface(), "Test", "FrameGraph", VK_API_VERSION_1_1, "" ));
			vulkan.CreateDebugCallback( VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT );
		}

		// setup device info
		VulkanDeviceInfo		vulkan_info;
		VulkanSwapchainInfo		swapchain_info;
		{
			vulkan_info.instance		= BitCast<VkInstance_t>( vulkan.GetVkInstance() );
			vulkan_info.physicalDevice	= BitCast<VkPhysicalDevice_t>( vulkan.GetVkPhysicalDevice() );
			vulkan_info.device			= BitCast<VkDevice_t>( vulkan.GetVkDevice() );
			
			swapchain_info.surface		= BitCast<VkSurface_t>( vulkan.GetVkSurface() );
			swapchain_info.preTransform = {};

			for (auto& q : vulkan.GetVkQuues())
			{
				VulkanDeviceInfo::QueueInfo	qi;
				qi.id			= BitCast<VkQueue_t>( q.id );
				qi.familyFlags	= BitCast<VkQueueFlags_t>( q.flags );
				qi.familyIndex	= q.familyIndex;
				qi.priority		= q.priority;
				qi.debugName	= "";

				vulkan_info.queues.push_back( qi );
			}
		}

		// initialize framegraph
		{
			fg_instance = FrameGraph::CreateFrameGraph( vulkan_info );
			TEST( fg_instance );
			TEST( fg_instance->Initialize( 2 ));

			ThreadDesc	desc{ EThreadUsage::Graphics | EThreadUsage::Present | EThreadUsage::Transfer, CommandBatchID{"1"} };

			fg_thread = fg_instance->CreateThread( desc );
			TEST( fg_thread );
			TEST( fg_thread->CreateSwapchain( swapchain_info ));
			TEST( fg_thread->Initialize() );

			fg_instance->SetCompilationFlags( ECompilationFlags::EnableDebugger );
			fg_thread->SetCompilationFlags( ECompilationFlags::EnableDebugger );
		}

		UnitTest_FGMain( fg_thread );
		
		fg_thread->Deinitialize();
		fg_thread = null;

		fg_instance->Deinitialize();
		fg_instance = null;

		vulkan.Destroy();
		window->Destroy();
	}

    FG_LOGI( "Tests.FrameGraph finished" );
	
    std::cin.ignore();
	return 0;
}

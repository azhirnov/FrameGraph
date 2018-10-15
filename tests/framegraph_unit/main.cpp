// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/VFG.h"
#include "framework/Window/IWindow.h"
#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include <iostream>

using namespace FG;

extern void UnitTest_Main ();
extern void UnitTest_VkMain (const VulkanDeviceInfo &vdi);
extern void UnitTest_FGMain (const FrameGraphPtr &fg);


int main ()
{
	UnitTest_Main();

	{
		VulkanDeviceExt		vulkan;
		WindowPtr			window;
		FrameGraphPtr		frame_graph;
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
			vulkan.Create( window->GetVulkanSurface(), "Test", "FrameGraph" );
			vulkan.CreateDebugCallback( VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT );
		}

		// setup device info
		VulkanDeviceInfo	vulkan_info;
		{
			vulkan_info.instance		= BitCast<VkInstance_t>( vulkan.GetVkInstance() );
			vulkan_info.physicalDevice	= BitCast<VkPhysicalDevice_t>( vulkan.GetVkPhysicalDevice() );
			vulkan_info.device			= BitCast<VkDevice_t>( vulkan.GetVkDevice() );
			vulkan_info.surface			= BitCast<VkSurface_t>( vulkan.GetVkSurface() );
			//vulkan_info.surfaceTransform= 0;

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
			frame_graph = FrameGraph::CreateVulkanFrameGraph( vulkan_info );
			CHECK_ERR( frame_graph );

			CHECK_ERR( frame_graph->Initialize( 2 ) );

			frame_graph->SetCompilationFlags( ECompilationFlags::EnableDebugger );
		}

		UnitTest_VkMain( vulkan_info );
		UnitTest_FGMain( frame_graph );
		
		frame_graph->Deinitialize();
		frame_graph = null;

		vulkan.Destroy();
		window->Destroy();
	}

    FG_LOGI( "Tests.FrameGraph finished" );
	
    std::cin.ignore();
	return 0;
}

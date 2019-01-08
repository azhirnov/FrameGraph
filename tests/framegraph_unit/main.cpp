// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/VFG.h"
#include "framework/Window/IWindow.h"
#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include <iostream>
#include "UnitTest_Common.h"

using namespace FG;


extern void UnitTest_VertexInput ();
extern void UnitTest_ImageSwizzle ();
extern void UnitTest_PixelFormat ();
extern void UnitTest_ID ();
extern void UnitTest_VBuffer ();
extern void UnitTest_VImage ();

static void UnitTest_Main ()
{
	UnitTest_VertexInput();
	UnitTest_ImageSwizzle();
	UnitTest_PixelFormat();
	UnitTest_ID();
	UnitTest_VBuffer();
	UnitTest_VImage();
	FG_LOGI( "UnitTest_Main - finished" );
}
//-----------------------------------------------------------------------------



extern void UnitTest_VResourceManager (const FGThreadPtr &fg);

static void UnitTest_FGMain (const FGThreadPtr &fg)
{
	UnitTest_VResourceManager( fg );
	FG_LOGI( "UnitTest_FGMain - finished" );
}
//-----------------------------------------------------------------------------



int main ()
{
	UnitTest_Main();

	{
		VulkanDeviceExt		vulkan;
		WindowPtr			window;
		FGInstancePtr		fg_instance;
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
			//vulkan.CreateDebugReportCallback( DebugReportFlags_All );
			vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All );
		}

		// setup device info
		VulkanDeviceInfo						vulkan_info;
		FrameGraphThread::SwapchainCreateInfo	swapchain_info;
		{
			vulkan_info.instance		= BitCast<InstanceVk_t>( vulkan.GetVkInstance() );
			vulkan_info.physicalDevice	= BitCast<PhysicalDeviceVk_t>( vulkan.GetVkPhysicalDevice() );
			vulkan_info.device			= BitCast<DeviceVk_t>( vulkan.GetVkDevice() );
			
			VulkanSwapchainCreateInfo	swapchain_ci;
			swapchain_ci.surface		= BitCast<SurfaceVk_t>( vulkan.GetVkSurface() );
			swapchain_ci.surfaceSize	= window->GetSize();
			swapchain_info				= swapchain_ci;

			for (auto& q : vulkan.GetVkQuues())
			{
				VulkanDeviceInfo::QueueInfo	qi;
				qi.id			= BitCast<QueueVk_t>( q.id );
				qi.familyFlags	= BitCast<QueueFlagsVk_t>( q.flags );
				qi.familyIndex	= q.familyIndex;
				qi.priority		= q.priority;
				qi.debugName	= "";

				vulkan_info.queues.push_back( qi );
			}
		}

		// initialize framegraph
		{
			fg_instance = FrameGraphInstance::CreateFrameGraph( vulkan_info );
			TEST( fg_instance );
			TEST( fg_instance->Initialize( 2 ));

			ThreadDesc	desc{ EThreadUsage::Graphics | EThreadUsage::Present | EThreadUsage::Transfer };

			fg_thread = fg_instance->CreateThread( desc );
			TEST( fg_thread );
			TEST( fg_thread->Initialize( &swapchain_info ));

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

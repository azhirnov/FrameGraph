// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Android/WindowAndroid.h"
#include "stl/Math/Color.h"
#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Algorithms/StringUtils.h"
#include <thread>

using namespace FGC;


class FWApp1 final : public IWindowEventListener, public VulkanDeviceFn
{
private:
	VulkanDeviceExt		vulkan;
	VulkanSwapchainPtr	swapchain;
	WindowPtr			window;
	
	VkCommandPool		cmdPool	= VK_NULL_HANDLE;


public:
	FWApp1 ()
	{
		VulkanDeviceFn_Init( vulkan );
	}

	void OnResize (const uint2 &size) override
	{
		if ( Any( size == uint2(0) ))
			return;

		VK_CALL( vkDeviceWaitIdle( vulkan.GetVkDevice() ));

		VK_CALL( vkResetCommandPool( vulkan.GetVkDevice(), cmdPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT ));

		CHECK( swapchain->Recreate( size ));
	}
	
	void OnRefresh () override {}
	void OnDestroy () override {}
	void OnUpdate () override {}
	void OnMouseMove (const float2 &) override {}
	
	void OnKey (StringView key, EKeyAction action) override
	{
		if ( action == EKeyAction::Down )
			FG_LOGI( key );
	}


	bool Run (void* app = null)
	{
#	 if defined(FG_ENABLE_GLFW)
		window.reset( new WindowGLFW{} );

#	 elif defined(FG_ENABLE_SDL2)
		window.reset( new WindowSDL2{} );

#	 elif defined(PLATFORM_ANDROID)
		window.reset( new WindowAndroid{ Cast<android_app>(app) });

#	 else
#		error unknown window library!
#	 endif

		const String	title = "Test";

		// create window and vulkan device
		{
			CHECK_ERR( window->Create( { 800, 600 }, title ));
			window->AddListener( this );

			CHECK_ERR( vulkan.Create( window->GetVulkanSurface(), title, "Engine", VK_API_VERSION_1_0, {}, {} ));
		
			// this is a test and the test should fail for any validation error
			vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All,
											[] (const VulkanDeviceExt::DebugReport &rep) { CHECK_FATAL(not rep.isError); });
		}


		// initialize swapchain
		{
			VkFormat		color_fmt	= VK_FORMAT_UNDEFINED;
			VkColorSpaceKHR	color_space	= VK_COLOR_SPACE_MAX_ENUM_KHR;

			swapchain.reset( new VulkanSwapchain{ vulkan } );

			CHECK_ERR( swapchain->ChooseColorFormat( INOUT color_fmt, INOUT color_space ));

			CHECK_ERR( swapchain->Create( window->GetSize(), color_fmt, color_space ));
		}


		// initialize vulkan objects
		VkQueue				cmd_queue		= vulkan.GetVkQueues().front().handle;
		VkCommandBuffer		cmd_buffers[2]	= {};
		VkFence				fences[2]		= {};
		VkSemaphore			semaphores[2]	= {};
		{
			VkCommandPoolCreateInfo		pool_info = {};
			pool_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			pool_info.queueFamilyIndex	= vulkan.GetVkQueues().front().familyIndex;
			pool_info.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			VK_CHECK( vkCreateCommandPool( vulkan.GetVkDevice(), &pool_info, null, OUT &cmdPool ));

			VkCommandBufferAllocateInfo	info = {};
			info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.pNext				= null;
			info.commandPool		= cmdPool;
			info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			info.commandBufferCount	= 2;
			VK_CHECK( vkAllocateCommandBuffers( vulkan.GetVkDevice(), &info, OUT cmd_buffers ));
		
			VkFenceCreateInfo	fence_info	= {};
			fence_info.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags	= VK_FENCE_CREATE_SIGNALED_BIT;
			VK_CHECK( vkCreateFence( vulkan.GetVkDevice(), &fence_info, null, OUT &fences[0] ));
			VK_CHECK( vkCreateFence( vulkan.GetVkDevice(), &fence_info, null, OUT &fences[1] ));
			
			VkSemaphoreCreateInfo	sem_info = {};
			sem_info.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			sem_info.flags		= 0;
			VK_CALL( vkCreateSemaphore( vulkan.GetVkDevice(), &sem_info, null, OUT &semaphores[0] ) );
			VK_CALL( vkCreateSemaphore( vulkan.GetVkDevice(), &sem_info, null, OUT &semaphores[1] ) );
		}
	
		// main loop
		for (uint i = 0; i < 60; ++i)
		{
			if ( not window->Update() )
				break;

			if ( Any( window->GetSize() == uint2(0) )) {
				std::this_thread::sleep_for( std::chrono::milliseconds(16) );
				continue;
			}
			
			window->SetTitle( title + ("[FPS: "s << ToString(uint(swapchain->GetFramesPerSecond())) << ']') );

			// wait and acquire next image
			{
				VK_CHECK( vkWaitForFences( vulkan.GetVkDevice(), 1, &fences[i&1], true, UMax ));
				VK_CHECK( vkResetFences( vulkan.GetVkDevice(), 1, &fences[i&1] ));

				VK_CALL( swapchain->AcquireNextImage( semaphores[0] ));
			}

			// build command buffer
			{
				VkCommandBufferBeginInfo	begin_info = {};
				begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				VK_CALL( vkBeginCommandBuffer( cmd_buffers[i&1], &begin_info ));


				// image layout undefined to transfer optimal
				VkImageMemoryBarrier	image_barrier1;
				image_barrier1.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				image_barrier1.pNext				= null;
				image_barrier1.image				= swapchain->GetCurrentImage();
				image_barrier1.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
				image_barrier1.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				image_barrier1.srcAccessMask		= 0;
				image_barrier1.dstAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
				image_barrier1.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				image_barrier1.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				image_barrier1.subresourceRange		= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

				vkCmdPipelineBarrier( cmd_buffers[i&1], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
									  0, null, 0, null, 1, &image_barrier1 );
		

				// clear image
				RGBA32f				color		{ HSVColor{Fract( float(i) / 60.0f )} };
				VkClearColorValue	clear_value {{ color.r, color.g, color.b, color.a }};

				VkImageSubresourceRange	range;
				range.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
				range.baseArrayLayer	= 0;
				range.layerCount		= 1;
				range.baseMipLevel		= 0;
				range.levelCount		= 1;

				vkCmdClearColorImage( cmd_buffers[i&1], swapchain->GetCurrentImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_value, 1, &range );
			

				// image layout transfer optimal to present source
				VkImageMemoryBarrier	image_barrier2;
				image_barrier2.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				image_barrier2.pNext				= null;
				image_barrier2.image				= swapchain->GetCurrentImage();
				image_barrier2.oldLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				image_barrier2.newLayout			= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				image_barrier2.srcAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
				image_barrier2.dstAccessMask		= 0;
				image_barrier2.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				image_barrier2.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				image_barrier2.subresourceRange		= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

				vkCmdPipelineBarrier( cmd_buffers[i&1], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
									  0, null, 0, null, 1, &image_barrier2 );


				VK_CALL( vkEndCommandBuffer( cmd_buffers[i&1] ));
			}


			// submit commands
			{
				VkSemaphore				signal_semaphores[] = { semaphores[1] };
				VkSemaphore				wait_semaphores[]	= { semaphores[0] };
				VkPipelineStageFlags	wait_dst_mask[]		= { VK_PIPELINE_STAGE_TRANSFER_BIT };
				STATIC_ASSERT( CountOf(wait_semaphores) == CountOf(wait_dst_mask) );

				VkSubmitInfo				submit_info = {};
				submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit_info.commandBufferCount		= 1;
				submit_info.pCommandBuffers			= &cmd_buffers[i&1];
				submit_info.waitSemaphoreCount		= uint(CountOf(wait_semaphores));
				submit_info.pWaitSemaphores			= wait_semaphores;
				submit_info.pWaitDstStageMask		= wait_dst_mask;
				submit_info.signalSemaphoreCount	= uint(CountOf(signal_semaphores));
				submit_info.pSignalSemaphores		= signal_semaphores;

				VK_CHECK( vkQueueSubmit( cmd_queue, 1, &submit_info, fences[i&1] ));
			}

			// present
			VkResult	err = swapchain->Present( cmd_queue, {semaphores[1]} );
			switch ( err ) {
				case VK_SUCCESS :
					break;

				case VK_SUBOPTIMAL_KHR :
				case VK_ERROR_SURFACE_LOST_KHR :
				case VK_ERROR_OUT_OF_DATE_KHR :
					VK_CALL( vkQueueWaitIdle( cmd_queue ));
					VK_CALL( vkResetCommandPool( vulkan.GetVkDevice(), cmdPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT ));
					CHECK( swapchain->Recreate( window->GetSize() ));
					break;

				default :
					RETURN_ERR( "Present failed" );
			}
		}


		// destroy all
		{
			VK_CALL( vkDeviceWaitIdle( vulkan.GetVkDevice() ));

			vkDestroySemaphore( vulkan.GetVkDevice(), semaphores[0], null );
			vkDestroySemaphore( vulkan.GetVkDevice(), semaphores[1], null );
			vkDestroyFence( vulkan.GetVkDevice(), fences[0], null );
			vkDestroyFence( vulkan.GetVkDevice(), fences[1], null );
			vkDestroyCommandPool( vulkan.GetVkDevice(), cmdPool, null );

			cmdPool = VK_NULL_HANDLE;

			swapchain->Destroy();
			swapchain.reset();

			vulkan.Destroy();

			window->Destroy();
			window.reset();
		}
		return true;
	}
};

#ifdef PLATFORM_ANDROID
extern "C" void android_main (android_app* androidApp)
{
	FWApp1	app;
	app.Run( androidApp );
}
#else

extern void FW_Test1 ()
{
	FWApp1	app;

	CHECK_FATAL( app.Run() );
}
#endif

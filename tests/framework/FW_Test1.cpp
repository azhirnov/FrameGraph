// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "framework/Vulkan/VulkanDevice.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"

using namespace FG;


class FWApp1 final : public IWindowEventListener, public VulkanDeviceFn
{
private:
	VulkanDevice		vulkan;
	VulkanSwapchainPtr	swapchain;
	WindowPtr			window;


public:
	FWApp1 ()
	{
		VulkanDeviceFn_Init( vulkan );
	}

	void OnResize (const uint2 &) override
	{
		ASSERT( !"not supported" );
	}
	
	void OnRefrash () override {}
	void OnDestroy () override {}
	void OnUpdate () override {}


	bool Run ()
	{
#	 if defined(FG_ENABLE_GLFW)
		window.reset( new WindowGLFW() );

#	 elif defined(FG_ENABLE_SDL2)
		window.reset( new WindowSDL2() );

#	 else
#		error unknown window library!
#	 endif
	

		// create window and vulkan device
		const uint2		wnd_size{ 800, 600 };
		{
			CHECK_ERR( window->Create( wnd_size, "Test", null ));

			CHECK_ERR( vulkan.Create( window->GetVulkanSurface(), "Test", VK_API_VERSION_1_0 ));
		
			// it is the test, so test must fail on any error
			vulkan.CreateDebugCallback( VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT,
										[] (const VulkanDevice::DebugReport &rep) { CHECK_FATAL(rep.flags != VK_DEBUG_REPORT_ERROR_BIT_EXT); });
		}


		// initialize swapchain
		{
			VkFormat		color_fmt	= VK_FORMAT_UNDEFINED;
			VkColorSpaceKHR	color_space	= VK_COLOR_SPACE_MAX_ENUM_KHR;

			swapchain.reset( new VulkanSwapchain{ vulkan } );

			CHECK_ERR( swapchain->ChooseColorFormat( INOUT color_fmt, INOUT color_space ));

			CHECK_ERR( swapchain->Create( wnd_size, color_fmt, color_space ));
		}


		// initialize vulkan objects
		VkQueue				cmd_queue		= vulkan.GetVkQuues().front().id;
		VkCommandPool		cmd_pool;
		VkCommandBuffer		cmd_buffers[2]	= {};
		VkFence				fences[2]		= {};
		{
			VkCommandPoolCreateInfo		pool_info = {};
			pool_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			pool_info.queueFamilyIndex	= vulkan.GetVkQuues().front().familyIndex;
			pool_info.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			VK_CHECK( vkCreateCommandPool( vulkan.GetVkDevice(), &pool_info, null, OUT &cmd_pool ));

			VkCommandBufferAllocateInfo	info = {};
			info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.pNext				= null;
			info.commandPool		= cmd_pool;
			info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			info.commandBufferCount	= 2;
			VK_CHECK( vkAllocateCommandBuffers( vulkan.GetVkDevice(), &info, OUT cmd_buffers ));
		
			VkFenceCreateInfo	fence_info	= {};
			fence_info.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags	= VK_FENCE_CREATE_SIGNALED_BIT;
			VK_CHECK( vkCreateFence( vulkan.GetVkDevice(), &fence_info, null, OUT &fences[0] ));
			VK_CHECK( vkCreateFence( vulkan.GetVkDevice(), &fence_info, null, OUT &fences[1] ));
		}
	
		// main loop
		for (uint i = 0; i < 60*2; ++i)
		{
			if ( not window->Update() )
				break;

			// wait and acquire next image
			{
				VK_CHECK( vkWaitForFences( vulkan.GetVkDevice(), 1, &fences[i&1], true, ~0ull ));
				VK_CHECK( vkResetFences( vulkan.GetVkDevice(), 1, &fences[i&1] ));

				VK_CALL( swapchain->AcquireNextImage() );
			}

			// build command buffer
			{
				VkCommandBufferBeginInfo	begin_info = {};
				begin_info.sType			= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				begin_info.pNext			= null;
				begin_info.flags			= 0;
				begin_info.pInheritanceInfo	= null;
				VK_CALL( vkBeginCommandBuffer( cmd_buffers[i&1], &begin_info ));


				// image layout undefined to transfer optimal
				VkImageMemoryBarrier	image_barrier1;
				image_barrier1.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				image_barrier1.pNext					= null;
				image_barrier1.image					= swapchain->GetCurrentImage();
				image_barrier1.oldLayout				= VK_IMAGE_LAYOUT_UNDEFINED;
				image_barrier1.newLayout				= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				image_barrier1.srcAccessMask			= VK_ACCESS_MEMORY_READ_BIT;
				image_barrier1.dstAccessMask			= VK_ACCESS_TRANSFER_WRITE_BIT;
				image_barrier1.srcQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
				image_barrier1.dstQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
				image_barrier1.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
				image_barrier1.subresourceRange.baseMipLevel	= 0;
				image_barrier1.subresourceRange.levelCount		= 1;
				image_barrier1.subresourceRange.baseArrayLayer	= 0;
				image_barrier1.subresourceRange.layerCount		= 1;

				vkCmdPipelineBarrier( cmd_buffers[i&1], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
									  0, null, 0, null, 1, &image_barrier1 );
		

				// generate random color
				float	factor	= Fract( float(i) / 60.0f );
				float3	color	= Clamp( float3{ Abs(factor * 6.0f - 3.0f) - 1.0f, 2.0f - Abs(factor * 6.0f - 2.0f), 2.0f - Abs(factor * 6.0f - 4.0f) }, 0.0f, 1.0f );


				// clear image
				VkClearColorValue	clear_value = { color.x, color.y, color.z, 1.0f };

				VkImageSubresourceRange	range;
				range.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
				range.baseArrayLayer	= 0;
				range.layerCount		= 1;
				range.baseMipLevel		= 0;
				range.levelCount		= 1;

				vkCmdClearColorImage( cmd_buffers[i&1], swapchain->GetCurrentImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_value, 1, &range );
			

				// image layout transfer optimal to present source
				VkImageMemoryBarrier	image_barrier2;
				image_barrier2.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				image_barrier2.pNext					= null;
				image_barrier2.image					= swapchain->GetCurrentImage();
				image_barrier2.oldLayout				= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				image_barrier2.newLayout				= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				image_barrier2.srcAccessMask			= VK_ACCESS_TRANSFER_WRITE_BIT;
				image_barrier2.dstAccessMask			= VK_ACCESS_MEMORY_READ_BIT;
				image_barrier2.srcQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
				image_barrier2.dstQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
				image_barrier2.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
				image_barrier2.subresourceRange.baseMipLevel	= 0;
				image_barrier2.subresourceRange.levelCount		= 1;
				image_barrier2.subresourceRange.baseArrayLayer	= 0;
				image_barrier2.subresourceRange.layerCount		= 1;

				vkCmdPipelineBarrier( cmd_buffers[i&1], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
									  0, null, 0, null, 1, &image_barrier2 );


				VK_CALL( vkEndCommandBuffer( cmd_buffers[i&1] ));
			}


			// submit commands
			{
				VkSemaphore				signal_semaphores[] = { swapchain->GetRenderFinishedSemaphore() };
				VkSemaphore				wait_semaphores[]	= { swapchain->GetImageAvailableSemaphore() };
				VkPipelineStageFlags	wait_dst_mask[]		= { VK_PIPELINE_STAGE_TRANSFER_BIT };
				STATIC_ASSERT( std::size(wait_semaphores) == std::size(wait_dst_mask) );

				VkSubmitInfo				submit_info = {};
				submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit_info.commandBufferCount		= 1;
				submit_info.pCommandBuffers			= &cmd_buffers[i&1];
				submit_info.waitSemaphoreCount		= uint(std::size(wait_semaphores));
				submit_info.pWaitSemaphores			= wait_semaphores;
				submit_info.pWaitDstStageMask		= wait_dst_mask;
				submit_info.signalSemaphoreCount	= uint(std::size(signal_semaphores));
				submit_info.pSignalSemaphores		= signal_semaphores;

				VK_CHECK( vkQueueSubmit( cmd_queue, 1, &submit_info, fences[i&1] ));
			}

			// present
			CHECK( swapchain->Present( cmd_queue ));
		}


		// destroy all
		{
			VK_CALL( vkDeviceWaitIdle( vulkan.GetVkDevice() ));

			vkDestroyFence( vulkan.GetVkDevice(), fences[0], null );
			vkDestroyFence( vulkan.GetVkDevice(), fences[1], null );
			vkDestroyCommandPool( vulkan.GetVkDevice(), cmd_pool, null );

			swapchain->Destroy();
			swapchain.reset();

			vulkan.Destroy();

			window->Destroy();
			window.reset();
		}
		return true;
	}
};


extern void FW_Test1 ()
{
	FWApp1	app;

	CHECK_FATAL( app.Run() );
}

// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/VR/OpenVRDevice.h"
#include "framework/VR/VRDeviceEmulator.h"
#include "stl/Math/Color.h"
#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Algorithms/StringUtils.h"
#include <thread>

using namespace FGC;


class FWApp3 final : public IWindowEventListener, public VulkanDeviceFn
{
private:
	VulkanDeviceExt		vulkan;
	VulkanSwapchainPtr	swapchain;
	WindowPtr			window;
	VRDevicePtr			vrDevice;
	
	VkCommandPool		cmdPool	= VK_NULL_HANDLE;


public:
	FWApp3 ()
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


	bool Run ()
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

#	if defined(FG_ENABLE_OPENVR)
		vrDevice.reset( new OpenVRDevice{});
#	elif 1
		{
			WindowPtr	wnd{ new WindowGLFW{}};
			CHECK_ERR( wnd->Create( { 1024, 512 }, "emulator" ));

			vrDevice.reset( new VRDeviceEmulator{ std::move(wnd) });
		}
#	endif

		const String	title = "Test";

		// create window and vulkan device
		{
			if ( vrDevice )
				CHECK_ERR( vrDevice->Create() );

			CHECK_ERR( window->Create( { 800, 600 }, title ));
			window->AddListener( this );

			Array<const char*>	inst_ext	{ VulkanDevice::GetRecomendedInstanceExtensions() };
			Array<const char*>	dev_ext		{ VulkanDevice::GetRecomendedDeviceExtensions() };
			Array<String>		vr_inst_ext	= vrDevice ? vrDevice->GetRequiredInstanceExtensions() : Default;
			Array<String>		vr_dev_ext	= vrDevice ? vrDevice->GetRequiredDeviceExtensions() : Default;

			for (auto& ext : vr_inst_ext) {
				inst_ext.push_back( ext.data() );
			}
			for (auto& ext : vr_dev_ext) {
				dev_ext.push_back( ext.data() );
			}

			CHECK_ERR( vulkan.Create( window->GetVulkanSurface(), title, "Engine", VK_API_VERSION_1_2, {}, {},
									  VulkanDevice::GetRecomendedInstanceLayers(), inst_ext, dev_ext ));
		
			// this is a test and the test should fail for any validation error
			vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All,
											[] (const VulkanDeviceExt::DebugReport &rep) { CHECK_FATAL(not rep.isError); });

			if ( vrDevice )
				CHECK_ERR( vrDevice->SetVKDevice( vulkan.GetVkInstance(), vulkan.GetVkPhysicalDevice(), vulkan.GetVkDevice() ));
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

		// create render targets
		VkImage			vr_image[2]		= {};
		VkDeviceMemory	img_memory[2]	= {};
		uint2			vr_dim;
		VkFormat		vr_img_format	= VK_FORMAT_R8G8B8A8_UNORM;

		if ( vrDevice )
		{
			vr_dim = vrDevice->GetRenderTargetDimension();

			VkImageCreateInfo	info = {};
			info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			info.flags			= 0;
			info.imageType		= VK_IMAGE_TYPE_2D;
			info.format			= vr_img_format;
			info.extent			= { vr_dim.x, vr_dim.y, 1 };
			info.mipLevels		= 1;
			info.arrayLayers	= 1;
			info.samples		= VK_SAMPLE_COUNT_1_BIT;
			info.tiling			= VK_IMAGE_TILING_OPTIMAL;
			info.usage			= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			info.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
			VK_CHECK( vkCreateImage( vulkan.GetVkDevice(), &info, null, OUT &vr_image[0] ));
			VK_CHECK( vkCreateImage( vulkan.GetVkDevice(), &info, null, OUT &vr_image[1] ));

			VkMemoryRequirements	mem_req = {};
			vkGetImageMemoryRequirements( vulkan.GetVkDevice(), vr_image[0], OUT &mem_req );
			vkGetImageMemoryRequirements( vulkan.GetVkDevice(), vr_image[1], OUT &mem_req );

			VkMemoryAllocateInfo	alloc = {};
			alloc.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc.allocationSize	= mem_req.size;;
			CHECK_ERR( vulkan.GetMemoryTypeIndex( mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, OUT alloc.memoryTypeIndex ));
			VK_CHECK( vkAllocateMemory( vulkan.GetVkDevice(), &alloc, null, OUT &img_memory[0] ));
			VK_CHECK( vkAllocateMemory( vulkan.GetVkDevice(), &alloc, null, OUT &img_memory[1] ));

			VK_CHECK( vkBindImageMemory( vulkan.GetVkDevice(), vr_image[0], img_memory[0], 0 ));
			VK_CHECK( vkBindImageMemory( vulkan.GetVkDevice(), vr_image[1], img_memory[1], 0 ));

			vrDevice->SetupCamera( float2(0.1f, 100.0f) );
		}
	
		// main loop
		for (uint i = 0; i < 60*1000; ++i)
		{
			if ( not window->Update() )
				break;

			if ( Any( window->GetSize() == uint2(0) )) {
				std::this_thread::sleep_for( std::chrono::milliseconds(16) );
				continue;
			}

			IVRDevice::VRCamera	camera;
			if ( vrDevice )
			{
				if ( not vrDevice->Update() )
					break;

				camera = vrDevice->GetCamera();
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
				VkImageMemoryBarrier	image_barrier1 = {};
				image_barrier1.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
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
				
				if ( vrDevice )
				{
					image_barrier1.image = vr_image[0];
					vkCmdPipelineBarrier( cmd_buffers[i&1], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
										  0, null, 0, null, 1, &image_barrier1 );

					image_barrier1.image = vr_image[1];
					vkCmdPipelineBarrier( cmd_buffers[i&1], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
										  0, null, 0, null, 1, &image_barrier1 );
				}		

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

				if ( vrDevice )
				{
					vkCmdClearColorImage( cmd_buffers[i&1], vr_image[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_value, 1, &range );
					vkCmdClearColorImage( cmd_buffers[i&1], vr_image[1], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_value, 1, &range );
				}			

				// image layout transfer optimal to present source
				VkImageMemoryBarrier	image_barrier2 = {};
				image_barrier2.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
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
				
				if ( vrDevice )
				{
					image_barrier2.image		= vr_image[0];
					image_barrier2.newLayout	= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					vkCmdPipelineBarrier( cmd_buffers[i&1], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
										  0, null, 0, null, 1, &image_barrier2 );

					image_barrier2.image		= vr_image[1];
					vkCmdPipelineBarrier( cmd_buffers[i&1], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
										  0, null, 0, null, 1, &image_barrier2 );
				}

				VK_CALL( vkEndCommandBuffer( cmd_buffers[i&1] ));
			}


			// submit commands
			{
				VkSemaphore				signal_semaphores[] = { semaphores[1] };
				VkSemaphore				wait_semaphores[]	= { semaphores[0] };
				VkPipelineStageFlags	wait_dst_mask[]		= { VK_PIPELINE_STAGE_TRANSFER_BIT };
				STATIC_ASSERT( CountOf(wait_semaphores) == CountOf(wait_dst_mask) );

				VkSubmitInfo			submit_info = {};
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

			// present VR images
			if ( vrDevice )
			{
				IVRDevice::VRImage	image;
				image.bounds			= RectF{ 0.0f, 0.0f, 1.0f, 1.0f };
				image.currQueue			= vulkan.GetVkQueues()[0].handle;
				image.queueFamilyIndex	= vulkan.GetVkQueues()[0].familyIndex;
				image.dimension			= vr_dim;
				image.format			= vr_img_format;
				image.sampleCount		= 1;

				image.handle = vr_image[0];
				CHECK_ERR( vrDevice->Submit( image, IVRDevice::Eye::Left ));

				image.handle = vr_image[1];
				CHECK_ERR( vrDevice->Submit( image, IVRDevice::Eye::Right ));
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
			vkDestroyImage( vulkan.GetVkDevice(), vr_image[0], null );
			vkDestroyImage( vulkan.GetVkDevice(), vr_image[1], null );
			vkFreeMemory( vulkan.GetVkDevice(), img_memory[0], null );
			vkFreeMemory( vulkan.GetVkDevice(), img_memory[1], null );

			cmdPool = VK_NULL_HANDLE;

			if ( vrDevice )
			{
				vrDevice->Destroy();
				vrDevice.reset();
			}

			swapchain->Destroy();
			swapchain.reset();

			vulkan.Destroy();

			window->Destroy();
			window.reset();
		}
		return true;
	}
};


extern void FW_Test3 ()
{
	FWApp3	app;

	CHECK_FATAL( app.Run() );
}

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	references:
	https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present
	https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#synchronization-queue-transfers-release
	https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#synchronization-queue-transfers-acquire
*/

#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "compiler/SpvCompiler.h"
#include "stl/Math/Color.h"

namespace {

class AsyncComputeApp final : public IWindowEventListener, public VulkanDeviceFn
{
private:
	VulkanDeviceExt			vulkan;
	VulkanSwapchainPtr		swapchain;
	WindowPtr				window;
	SpvCompiler				spvCompiler;
	bool					presentInComputeQueueSupported = false;
	
	VkCommandPool			cmdPoolGraphics		= VK_NULL_HANDLE;
	VkCommandPool			cmdPoolCompute		= VK_NULL_HANDLE;
	VkCommandBuffer			cmdBufGraphics[4]	= {};
	VkCommandBuffer			cmdBufCompute[2]	= {};
	VkFence					fences[2]			= {};
	VkSemaphore				semaphores[4]		= {};

	VkPipelineLayout		graphicsPplnLayout	= VK_NULL_HANDLE;
	VkPipeline				graphicsPpln		= VK_NULL_HANDLE;
	VkShaderModule			vertShader			= VK_NULL_HANDLE;
	VkShaderModule			fragShader			= VK_NULL_HANDLE;

	VkPipeline				computePpln			= VK_NULL_HANDLE;
	VkPipelineLayout		computePplnLayout	= VK_NULL_HANDLE;
	VkShaderModule			computeShader		= VK_NULL_HANDLE;
	const uint2				computeGroupSize	{ 16, 16 };

	VkDescriptorPool		descriptorPool		= VK_NULL_HANDLE;
	VkDescriptorSetLayout	dsLayoutCompute		= VK_NULL_HANDLE;
	VkDescriptorSet			dsCompute[2]		= {};

	const uint2				renderTargetSize	{ 2048, 2048 };
	VkFormat				colorFormat			= VK_FORMAT_R8G8B8A8_UNORM;
	VkRenderPass			renderPass			= VK_NULL_HANDLE;
	VkFramebuffer			framebuffers[2]		= {};
	VkImage					renderTarget[2]		= {};
	VkImageView				renderTargetView[2]	= {};
	VkSampler				sampler				= VK_NULL_HANDLE;
	VkDeviceMemory			sharedMemory		= VK_NULL_HANDLE;

	bool					looping				= true;


public:
	AsyncComputeApp ()
	{
		VulkanDeviceFn_Init( vulkan );
	}
	
	void OnKey (StringView key, EKeyAction action) override;
	void OnResize (const uint2 &size) override;
	
	void OnRefresh () override {}
	void OnDestroy () override {}
	void OnUpdate () override {}
	void OnMouseMove (const float2 &) override {}

	bool Initialize ();
	void Destroy ();
	bool Run ();

	// version for graphics & present + async compute without present
	void GenGraphicsCommands1 (uint frameId);
	void GenComputeCommands1 (uint frameId);
	void Present1 (uint frameId);
	
	// version for graphics & present + compute & present
	void GenGraphicsCommands2 (uint frameId);
	void GenComputeCommands2 (uint frameId);
	void Present2 (uint frameId);

	bool CreateCommandBuffers ();
	bool CreateSyncObjects ();
	bool CreateRenderPass ();
	bool CreateSampler ();
	bool CreateResources ();
	bool CreateDescriptorSet ();
	bool CreateGraphicsPipeline ();
	bool CreateComputePipeline ();
};



/*
=================================================
	OnKey
=================================================
*/
void AsyncComputeApp::OnKey (StringView key, EKeyAction action)
{
	if ( action != EKeyAction::Down )
		return;

	if ( key == "escape" )
		looping = false;
}

/*
=================================================
	OnResize
=================================================
*/
void AsyncComputeApp::OnResize (const uint2 &size)
{
	VK_CALL( vkDeviceWaitIdle( vulkan.GetVkDevice() ));

	VK_CALL( vkResetCommandPool( vulkan.GetVkDevice(), cmdPoolGraphics, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT ));
	VK_CALL( vkResetCommandPool( vulkan.GetVkDevice(), cmdPoolCompute, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT ));

	CHECK( swapchain->Recreate( size ));
}

/*
=================================================
	Initialize
=================================================
*/
bool AsyncComputeApp::Initialize ()
{
# if defined(FG_ENABLE_GLFW)
	window.reset( new WindowGLFW() );

# elif defined(FG_ENABLE_SDL2)
	window.reset( new WindowSDL2() );

# else
#	error unknown window library!
# endif
	

	// create window and vulkan device
	{
		const char	title[] = "Graphics + Async compute sample";

		CHECK_ERR( window->Create( { 800, 600 }, title ));
		window->AddListener( this );

		CHECK_ERR( vulkan.Create( window->GetVulkanSurface(),
								  title, "Engine",
								  VK_API_VERSION_1_1,
								  "",
								  {{ VK_QUEUE_PRESENT_BIT | VK_QUEUE_GRAPHICS_BIT, 0.0f },
								   { VK_QUEUE_COMPUTE_BIT, 0.0f }},
								  VulkanDevice::GetRecomendedInstanceLayers(),
								  VulkanDevice::GetRecomendedInstanceExtensions(),
								  {}
			));

		CHECK_ERR( vulkan.GetVkQueues().size() == 2 );
		CHECK_ERR( vulkan.GetVkQueues()[0].familyIndex != vulkan.GetVkQueues()[1].familyIndex );
		
		presentInComputeQueueSupported = EnumEq( vulkan.GetVkQueues()[1].flags, VK_QUEUE_PRESENT_BIT );
		
		vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All );
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
	CHECK_ERR( CreateCommandBuffers() );
	CHECK_ERR( CreateSyncObjects() );
	CHECK_ERR( CreateRenderPass() );
	CHECK_ERR( CreateSampler() );
	CHECK_ERR( CreateResources() );
	CHECK_ERR( CreateDescriptorSet() );
	CHECK_ERR( CreateGraphicsPipeline() );
	CHECK_ERR( CreateComputePipeline() );
	return true;
}

/*
=================================================
	Destroy
=================================================
*/
void AsyncComputeApp::Destroy ()
{
	VkDevice	dev = vulkan.GetVkDevice();

	VK_CALL( vkDeviceWaitIdle( dev ));
	
	for (auto& sem : semaphores) {
		vkDestroySemaphore( dev, sem, null );
		sem = VK_NULL_HANDLE;
	}
	for (auto& fen : fences) {
		vkDestroyFence( dev, fen, null );
		fen = VK_NULL_HANDLE;
	}
	for (auto& fb : framebuffers) {
		vkDestroyFramebuffer( dev, fb, null );
		fb = VK_NULL_HANDLE;
	}
	for (auto& rt : renderTarget) {
		vkDestroyImage( dev, rt, null );
		rt = VK_NULL_HANDLE;
	}
	for (auto& view : renderTargetView) {
		vkDestroyImageView( dev, view, null );
		view = VK_NULL_HANDLE;
	}
	vkDestroyCommandPool( dev, cmdPoolGraphics, null );
	vkDestroyCommandPool( dev, cmdPoolCompute, null );
	vkDestroyRenderPass( dev, renderPass, null );
	vkDestroyPipeline( dev, graphicsPpln, null );
	vkDestroyPipeline( dev, computePpln, null );
	vkDestroyShaderModule( dev, vertShader, null );
	vkDestroyShaderModule( dev, fragShader, null );
	vkDestroyShaderModule( dev, computeShader, null );
	vkDestroyPipelineLayout( dev, graphicsPplnLayout, null );
	vkDestroyPipelineLayout( dev, computePplnLayout, null );
	vkDestroyDescriptorSetLayout( dev, dsLayoutCompute, null );
	vkDestroyDescriptorPool( dev, descriptorPool, null );
	vkDestroySampler( dev, sampler, null );
	vkFreeMemory( dev, sharedMemory, null );

	cmdPoolGraphics		= VK_NULL_HANDLE;
	cmdPoolCompute		= VK_NULL_HANDLE;
	graphicsPpln		= VK_NULL_HANDLE;
	computePpln			= VK_NULL_HANDLE;
	vertShader			= VK_NULL_HANDLE;
	fragShader			= VK_NULL_HANDLE;
	computeShader		= VK_NULL_HANDLE;
	graphicsPplnLayout	= VK_NULL_HANDLE;
	computePplnLayout	= VK_NULL_HANDLE;
	dsLayoutCompute		= VK_NULL_HANDLE;
	descriptorPool		= VK_NULL_HANDLE;
	sharedMemory		= VK_NULL_HANDLE;
	sampler				= VK_NULL_HANDLE;

	swapchain->Destroy();
	swapchain.reset();

	vulkan.Destroy();

	window->Destroy();
	window.reset();
}

/*
=================================================
	GenGraphicsCommands1
----
	draws to 'renderTarget' and transfer 'renderTarget' to compute queue
=================================================
*/
void AsyncComputeApp::GenGraphicsCommands1 (uint frameId)
{
	VkCommandBuffer		g_cmd = cmdBufGraphics[frameId];

	// build graphics command buffer
	{
		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vkBeginCommandBuffer( g_cmd, &begin_info ));
			
		// shader_read_only_optimal -> color_attachment_optimal
		// queue acquire operation
		{
			VkImageMemoryBarrier	barrier = {};
			barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image				= renderTarget[frameId];
			barrier.oldLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.newLayout			= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.srcAccessMask		= 0;
			barrier.dstAccessMask		= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.srcQueueFamilyIndex	= vulkan.GetVkQueues()[1].familyIndex;
			barrier.dstQueueFamilyIndex	= vulkan.GetVkQueues()[0].familyIndex;
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier( g_cmd,
								  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}

		// begin render pass
		{
			VkClearValue			clear_value = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
			VkRenderPassBeginInfo	begin		= {};
			begin.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			begin.framebuffer		= framebuffers[frameId];
			begin.renderPass		= renderPass;
			begin.renderArea		= { {0,0}, {renderTargetSize.x, renderTargetSize.y} };
			begin.clearValueCount	= 1;
			begin.pClearValues		= &clear_value;

			vkCmdBeginRenderPass( g_cmd, &begin, VK_SUBPASS_CONTENTS_INLINE );
		}
			
		vkCmdBindPipeline( g_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPpln );

		// set dynamic states
		{
			VkViewport	viewport = {};
			viewport.x			= 0.0f;
			viewport.y			= 0.0f;
			viewport.width		= float(renderTargetSize.x);
			viewport.height		= float(renderTargetSize.y);
			viewport.minDepth	= 0.0f;
			viewport.maxDepth	= 1.0f;

			vkCmdSetViewport( g_cmd, 0, 1, &viewport );

			VkRect2D	scissor_rect = { {0,0}, {renderTargetSize.x, renderTargetSize.y} };

			vkCmdSetScissor( g_cmd, 0, 1, &scissor_rect );
		}
			
		vkCmdDraw( g_cmd, 4, 1, 0, 0 );

		vkCmdEndRenderPass( g_cmd );
			
		// color_attachment_optimal -> shader_read_only_optimal
		// queue release operation
		{
			VkImageMemoryBarrier	barrier = {};
			barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image				= renderTarget[frameId];
			barrier.oldLayout			= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.newLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask		= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask		= 0;
			barrier.srcQueueFamilyIndex	= vulkan.GetVkQueues()[0].familyIndex;
			barrier.dstQueueFamilyIndex	= vulkan.GetVkQueues()[1].familyIndex;
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier( g_cmd,
								  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}

		VK_CALL( vkEndCommandBuffer( g_cmd ));
	}

	// submit graphics commands
	{
		VkSubmitInfo				submit_info = {};
		submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount		= 1;
		submit_info.pCommandBuffers			= &g_cmd;
		submit_info.waitSemaphoreCount		= 0;
		submit_info.pWaitSemaphores			= null;
		submit_info.pWaitDstStageMask		= null;
		submit_info.signalSemaphoreCount	= 1;
		submit_info.pSignalSemaphores		= &semaphores[0];

		VK_CALL( vkQueueSubmit( vulkan.GetVkQueues()[0].handle, 1, &submit_info, VK_NULL_HANDLE ));
	}
}

/*
=================================================
	GenComputeCommands1
----
	wait for graphics queue
	wait for swapchain image acquiring
	in compute shader copy 'renderTarget' into swapchain image
	transfer swapchain image into present (graphics) queue
=================================================
*/
void AsyncComputeApp::GenComputeCommands1 (uint frameId)
{
	VkCommandBuffer		c_cmd = cmdBufCompute[frameId];
	
	VK_CALL( swapchain->AcquireNextImage( semaphores[1] ));	

	// update descriptor set
	{
		VkDescriptorImageInfo	images[1] = {};
		images[0].imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		images[0].imageView		= swapchain->GetCurrentImageView();
		images[0].sampler		= VK_NULL_HANDLE;

		VkWriteDescriptorSet	writes[1] = {};
		writes[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet			= dsCompute[frameId];
		writes[0].dstBinding		= 1;
		writes[0].descriptorCount	= 1;
		writes[0].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[0].pImageInfo		= &images[0];

		vkUpdateDescriptorSets( vulkan.GetVkDevice(), uint(CountOf( writes )), writes, 0, null );
	}

	// build compute command buffer
	{
		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vkBeginCommandBuffer( c_cmd, &begin_info ));
			
		// color_attachment_optimal -> shader_read_only_optimal
		// queue acquire operation
		{
			VkImageMemoryBarrier	barrier = {};
			barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image				= renderTarget[frameId];
			barrier.oldLayout			= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.newLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask		= 0;
			barrier.dstAccessMask		= VK_ACCESS_SHADER_READ_BIT;
			barrier.srcQueueFamilyIndex	= vulkan.GetVkQueues()[0].familyIndex;
			barrier.dstQueueFamilyIndex	= vulkan.GetVkQueues()[1].familyIndex;
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier( c_cmd,
								  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}
			
		// undefined -> general
		{
			VkImageMemoryBarrier	barrier = {};
			barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image				= swapchain->GetCurrentImage();
			barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout			= VK_IMAGE_LAYOUT_GENERAL;
			barrier.srcAccessMask		= VK_ACCESS_MEMORY_READ_BIT;
			barrier.dstAccessMask		= VK_ACCESS_SHADER_WRITE_BIT;
			barrier.srcQueueFamilyIndex	= vulkan.GetVkQueues()[1].familyIndex;
			barrier.dstQueueFamilyIndex	= vulkan.GetVkQueues()[1].familyIndex;
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier( c_cmd,
								  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}
		
		// dispatch
		{
			vkCmdBindPipeline( c_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePpln );
			vkCmdBindDescriptorSets( c_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePplnLayout, 0, 1, &dsCompute[frameId], 0, null );
			
			const uint2  count = (swapchain->GetSurfaceSize() + computeGroupSize-1) / computeGroupSize;

			vkCmdDispatch( c_cmd, count.x, count.y, 1 );
		}

		// shader_read_only_optimal -> color_attachment_optimal
		// queue release operation
		{
			VkImageMemoryBarrier	barrier = {};
			barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image				= renderTarget[frameId];
			barrier.oldLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.newLayout			= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.srcAccessMask		= VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask		= 0;
			barrier.srcQueueFamilyIndex	= vulkan.GetVkQueues()[1].familyIndex;
			barrier.dstQueueFamilyIndex	= vulkan.GetVkQueues()[0].familyIndex;
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier( c_cmd,
								  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}
			
		// general -> present_src
		// queue release operation
		{
			VkImageMemoryBarrier	barrier = {};
			barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image				= swapchain->GetCurrentImage();
			barrier.oldLayout			= VK_IMAGE_LAYOUT_GENERAL;
			barrier.newLayout			= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier.srcAccessMask		= VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask		= 0;
			barrier.srcQueueFamilyIndex	= vulkan.GetVkQueues()[1].familyIndex;
			barrier.dstQueueFamilyIndex	= vulkan.GetVkQueues()[0].familyIndex;
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier( c_cmd,
								  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}

		VK_CALL( vkEndCommandBuffer( c_cmd ));
	}

	// submit compute commands
	{
		const VkPipelineStageFlags	wait_dst_mask[] = {
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,		// from graphics queue
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT	// from acquire swapchain image
		};

		VkSubmitInfo				submit_info = {};
		submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount		= 1;
		submit_info.pCommandBuffers			= &c_cmd;
		submit_info.waitSemaphoreCount		= 2;
		submit_info.pWaitSemaphores			= &semaphores[0];	// 0, 1
		submit_info.pWaitDstStageMask		= wait_dst_mask;
		submit_info.signalSemaphoreCount	= 1;
		submit_info.pSignalSemaphores		= &semaphores[2];

		VK_CALL( vkQueueSubmit( vulkan.GetVkQueues()[1].handle, 1, &submit_info, VK_NULL_HANDLE ));
	}
}

/*
=================================================
	Present1
=================================================
*/
void AsyncComputeApp::Present1 (uint frameId)
{
	VkCommandBuffer		g_cmd = cmdBufGraphics[frameId + 2];

	// build graphics command buffer
	{
		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vkBeginCommandBuffer( g_cmd, &begin_info ));
			
		// general -> present_src
		// queue acquire operation
		{
			VkImageMemoryBarrier	barrier = {};
			barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image				= swapchain->GetCurrentImage();
			barrier.oldLayout			= VK_IMAGE_LAYOUT_GENERAL;
			barrier.newLayout			= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier.srcAccessMask		= 0;
			barrier.dstAccessMask		= VK_ACCESS_MEMORY_READ_BIT;
			barrier.srcQueueFamilyIndex	= vulkan.GetVkQueues()[1].familyIndex;
			barrier.dstQueueFamilyIndex	= vulkan.GetVkQueues()[0].familyIndex;
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier( g_cmd,
								  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}

		VK_CALL( vkEndCommandBuffer( g_cmd ));
	}

	// submit graphics commands
	{
		const VkPipelineStageFlags	wait_dst_mask[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };

		VkSubmitInfo				submit_info = {};
		submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount		= 1;
		submit_info.pCommandBuffers			= &g_cmd;
		submit_info.waitSemaphoreCount		= 1;
		submit_info.pWaitSemaphores			= &semaphores[2];
		submit_info.pWaitDstStageMask		= wait_dst_mask;
		submit_info.signalSemaphoreCount	= 1;
		submit_info.pSignalSemaphores		= &semaphores[3];

		VK_CALL( vkQueueSubmit( vulkan.GetVkQueues()[0].handle, 1, &submit_info, fences[frameId] ));
	}

	// present
	VkResult	err = swapchain->Present( vulkan.GetVkQueues()[0].handle, {semaphores[3]} );
	switch ( err ) {
		case VK_SUCCESS :
			break;

		case VK_SUBOPTIMAL_KHR :
		case VK_ERROR_SURFACE_LOST_KHR :
		case VK_ERROR_OUT_OF_DATE_KHR :
			OnResize( swapchain->GetSurfaceSize() );
			break;

		default :
			CHECK_FATAL( !"Present failed" );
	}
}

/*
=================================================
	Run
=================================================
*/
bool AsyncComputeApp::Run ()
{
	for (uint frameId = 0; looping; frameId = ((frameId + 1) & 1))
	{
		if ( not window->Update() )
			break;

		VK_CHECK( vkWaitForFences( vulkan.GetVkDevice(), 1, &fences[frameId], true, UMax ));
		VK_CHECK( vkResetFences( vulkan.GetVkDevice(), 1, &fences[frameId] ));

		/*if ( presentInComputeQueueSupported )
		{
			GenGraphicsCommands2( frameId );
			GenComputeCommands2( frameId );
			Present2( frameId );
		}
		else*/
		{
			GenGraphicsCommands1( frameId );
			GenComputeCommands1( frameId );
			Present1( frameId );
		}
	}
	return true;
}

/*
=================================================
	CreateCommandBuffers
=================================================
*/
bool AsyncComputeApp::CreateCommandBuffers ()
{
	VkCommandPoolCreateInfo		pool_info = {};
	pool_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex	= vulkan.GetVkQueues()[0].familyIndex;
	pool_info.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK( vkCreateCommandPool( vulkan.GetVkDevice(), &pool_info, null, OUT &cmdPoolGraphics ));
	
	pool_info.queueFamilyIndex	= vulkan.GetVkQueues()[1].familyIndex;
	VK_CHECK( vkCreateCommandPool( vulkan.GetVkDevice(), &pool_info, null, OUT &cmdPoolCompute ));

	VkCommandBufferAllocateInfo	info = {};
	info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.pNext				= null;
	info.commandPool		= cmdPoolGraphics;
	info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount	= uint(CountOf( cmdBufGraphics ));
	VK_CHECK( vkAllocateCommandBuffers( vulkan.GetVkDevice(), &info, OUT cmdBufGraphics ));
	
	info.commandBufferCount	= uint(CountOf( cmdBufCompute ));
	info.commandPool		= cmdPoolCompute;
	VK_CHECK( vkAllocateCommandBuffers( vulkan.GetVkDevice(), &info, OUT cmdBufCompute ));

	return true;
}

/*
=================================================
	CreateSyncObjects
=================================================
*/
bool AsyncComputeApp::CreateSyncObjects ()
{
	VkDevice	dev = vulkan.GetVkDevice();

	VkFenceCreateInfo	fence_info	= {};
	fence_info.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags	= VK_FENCE_CREATE_SIGNALED_BIT;

	for (auto& fence : fences) {
		VK_CHECK( vkCreateFence( dev, &fence_info, null, OUT &fence ));
	}
			
	VkSemaphoreCreateInfo	sem_info = {};
	sem_info.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	sem_info.flags		= 0;

	for (auto& sem : semaphores) {
		VK_CALL( vkCreateSemaphore( dev, &sem_info, null, OUT &sem ) );
	}

	return true;
}

/*
=================================================
	CreateRenderPass
=================================================
*/
bool AsyncComputeApp::CreateRenderPass ()
{
	// setup attachment
	VkAttachmentDescription		attachments[1] = {};

	attachments[0].format			= colorFormat;
	attachments[0].samples			= VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	// setup subpasses
	VkSubpassDescription	subpasses[1]		= {};
	VkAttachmentReference	attachment_ref[1]	= {};

	attachment_ref[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	subpasses[0].pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].colorAttachmentCount	= 1;
	subpasses[0].pColorAttachments		= &attachment_ref[0];


	// setup dependencies
	VkSubpassDependency		dependencies[2] = {};

	dependencies[0].srcSubpass		= VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass		= 0;
	dependencies[0].srcStageMask	= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask	= 0;
	dependencies[0].dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass		= 0;
	dependencies[1].dstSubpass		= VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask	= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask	= 0;
	dependencies[1].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;


	// setup create info
	VkRenderPassCreateInfo	info = {};
	info.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.flags				= 0;
	info.attachmentCount	= uint(CountOf( attachments ));
	info.pAttachments		= attachments;
	info.subpassCount		= uint(CountOf( subpasses ));
	info.pSubpasses			= subpasses;
	info.dependencyCount	= uint(CountOf( dependencies ));
	info.pDependencies		= dependencies;

	VK_CHECK( vkCreateRenderPass( vulkan.GetVkDevice(), &info, null, OUT &renderPass ));
	return true;
}

/*
=================================================
	CreateSampler
=================================================
*/
bool AsyncComputeApp::CreateSampler ()
{
	VkSamplerCreateInfo		info = {};
	info.sType				= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.magFilter			= VK_FILTER_LINEAR;
	info.minFilter			= VK_FILTER_LINEAR;
	info.mipmapMode			= VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info.addressModeU		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.addressModeV		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.addressModeW		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.mipLodBias			= 0.0f;
	info.anisotropyEnable	= VK_FALSE;
	//info.maxAnisotropy		= 0.0f;
	info.compareEnable		= VK_FALSE;
	//info.compareOp			= 
	info.minLod				= -1000.0f;
	info.maxLod				= +1000.0f;
	//info.borderColor		= 
	info.unnormalizedCoordinates = VK_FALSE;

	VK_CHECK( vkCreateSampler( vulkan.GetVkDevice(), &info, null, OUT &sampler ));
	return true;
}

/*
=================================================
	CreateResources
=================================================
*/
bool AsyncComputeApp::CreateResources ()
{
	VkDeviceSize					total_size		= 0;
	uint							mem_type_bits	= 0;
	VkMemoryPropertyFlags			mem_property	= 0;
	Array<std::function<void ()>>	bind_mem;
	const uint2						image_size		= renderTargetSize;


	// create texture
	for (size_t i = 0; i < CountOf(renderTarget); ++i)
	{
		auto&	rt = renderTarget[i];

		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.flags			= 0;
		info.imageType		= VK_IMAGE_TYPE_2D;
		info.format			= VK_FORMAT_R8G8B8A8_UNORM;
		info.extent			= { image_size.x, image_size.y, 1 };
		info.mipLevels		= 1;
		info.arrayLayers	= 1;
		info.samples		= VK_SAMPLE_COUNT_1_BIT;
		info.tiling			= VK_IMAGE_TILING_OPTIMAL;
		info.usage			= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		info.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vkCreateImage( vulkan.GetVkDevice(), &info, null, OUT &rt ));
		
		VkMemoryRequirements	mem_req;
		vkGetImageMemoryRequirements( vulkan.GetVkDevice(), rt, OUT &mem_req );
		
		VkDeviceSize	offset = AlignToLarger( total_size, mem_req.alignment );
		total_size		 = offset + mem_req.size;
		mem_type_bits	|= mem_req.memoryTypeBits;
		mem_property	|= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		bind_mem.push_back( [this, rt, offset, rt_view = &renderTargetView[i]] () {
			VK_CALL( vkBindImageMemory( vulkan.GetVkDevice(), rt, sharedMemory, offset ));

			VkImageViewCreateInfo	view = {};
			view.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view.image				= rt;
			view.viewType			= VK_IMAGE_VIEW_TYPE_2D;
			view.format				= VK_FORMAT_R8G8B8A8_UNORM;
			view.components			= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
			view.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			VK_CALL( vkCreateImageView( vulkan.GetVkDevice(), &view, null, OUT rt_view ));
		});
	}

	// allocate memory
	{
		VkMemoryAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize		= total_size;

		CHECK_ERR( vulkan.GetMemoryTypeIndex( mem_type_bits, mem_property, OUT info.memoryTypeIndex ));

		VK_CHECK( vkAllocateMemory( vulkan.GetVkDevice(), &info, null, OUT &sharedMemory ));
	}

	// bind resources
	for (auto& bind : bind_mem) {
		bind();
	}

	// update resources
	{
		VkCommandBuffer		cmd = cmdBufCompute[0];

		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vkBeginCommandBuffer( cmd, &begin_info ));

		for (auto& rt : renderTarget)
		{
			// undefined -> shader_read_only_optimal
			{
				VkImageMemoryBarrier	barrier = {};
				barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image				= rt;
				barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask		= 0;
				barrier.dstAccessMask		= 0;
				barrier.srcQueueFamilyIndex	= vulkan.GetVkQueues()[1].familyIndex;
				barrier.dstQueueFamilyIndex	= vulkan.GetVkQueues()[1].familyIndex;
				barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

				vkCmdPipelineBarrier( cmd,
									  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
									  0, null, 0, null, 1, &barrier );
			}

			// shader_read_only_optimal -> color_attachment_optimal
			// queue release operation
			{
				VkImageMemoryBarrier	barrier = {};
				barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image				= rt;
				barrier.oldLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.newLayout			= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				barrier.srcAccessMask		= 0;
				barrier.dstAccessMask		= 0;
				barrier.srcQueueFamilyIndex	= vulkan.GetVkQueues()[1].familyIndex;
				barrier.dstQueueFamilyIndex	= vulkan.GetVkQueues()[0].familyIndex;
				barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

				vkCmdPipelineBarrier( cmd,
									  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
									  0, null, 0, null, 1, &barrier );
			}
		}

		VK_CALL( vkEndCommandBuffer( cmd ));

		VkSubmitInfo		submit_info = {};
		submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount	= 1;
		submit_info.pCommandBuffers		= &cmd;

		VK_CHECK( vkQueueSubmit( vulkan.GetVkQueues()[1].handle, 1, &submit_info, VK_NULL_HANDLE ));
	}

	VK_CALL( vkQueueWaitIdle( vulkan.GetVkQueues()[1].handle ));
	
	// create framebuffers
	{
		VkFramebufferCreateInfo	fb_info = {};
		fb_info.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass		= renderPass;
		fb_info.attachmentCount	= 1;
		fb_info.width			= image_size.x;
		fb_info.height			= image_size.y;
		fb_info.layers			= 1;

		for (size_t i = 0; i < CountOf(framebuffers); ++i)
		{
			fb_info.pAttachments = &renderTargetView[i];
			VK_CALL( vkCreateFramebuffer( vulkan.GetVkDevice(), &fb_info, null, OUT &framebuffers[i] ));
		}
	}
	return true;
}

/*
=================================================
	CreateDescriptorSet
=================================================
*/
bool AsyncComputeApp::CreateDescriptorSet ()
{
	// create layout for compute pipeline
	{
		VkDescriptorSetLayoutBinding	binding[2] = {};
		binding[0].binding			= 0;
		binding[0].descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding[0].descriptorCount	= 1;
		binding[0].stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;

		binding[1].binding			= 1;
		binding[1].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding[1].descriptorCount	= 1;
		binding[1].stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= uint(CountOf( binding ));
		info.pBindings		= binding;

		VK_CHECK( vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &dsLayoutCompute ));
	}

	// create pool
	{
		const VkDescriptorPoolSize		sizes[] = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,			 100 }
		};

		VkDescriptorPoolCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.maxSets		= 100;
		info.poolSizeCount	= uint(CountOf( sizes ));
		info.pPoolSizes		= sizes;

		VK_CHECK( vkCreateDescriptorPool( vulkan.GetVkDevice(), &info, null, OUT &descriptorPool ));
	}

	// allocate descriptor set
	{
		VkDescriptorSetAllocateInfo		info = {};
		info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool		= descriptorPool;
		info.descriptorSetCount	= 1;
		info.pSetLayouts		= &dsLayoutCompute;

		for (size_t i = 0; i < CountOf(dsCompute); ++i) {
			VK_CHECK( vkAllocateDescriptorSets( vulkan.GetVkDevice(), &info, OUT &dsCompute[i] ));
		}
	}

	// update descriptor set
	for (size_t i = 0; i < CountOf(renderTargetView); ++i)
	{
		VkDescriptorImageInfo	images[1] = {};
		images[0].imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		images[0].imageView		= renderTargetView[i];
		images[0].sampler		= sampler;

		VkWriteDescriptorSet	writes[1] = {};
		writes[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet			= dsCompute[i];
		writes[0].dstBinding		= 0;
		writes[0].descriptorCount	= 1;
		writes[0].descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo		= &images[0];

		vkUpdateDescriptorSets( vulkan.GetVkDevice(), uint(CountOf( writes )), writes, 0, null );
	}
	return true;
}

/*
=================================================
	CreateGraphicsPipeline
=================================================
*/
bool AsyncComputeApp::CreateGraphicsPipeline ()
{
	// create vertex shader
	{
		static const char	vert_shader_source[] = R"#(
const vec2	g_Positions[4] = {
	vec2( -1.0,  1.0 ),
	vec2( -1.0, -1.0 ),
	vec2(  1.0,  1.0 ),
	vec2(  1.0, -1.0 )
};

void main()
{
	gl_Position = vec4( g_Positions[gl_VertexIndex], 0.0, 1.0 );
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT vertShader, vulkan, {vert_shader_source}, "main", EShLangVertex ));
	}

	// create fragment shader
	{
		static const char	frag_shader_source[] = R"#(
layout(location = 0) out vec4  out_Color;

void main ()
{
	// from https://www.shadertoy.com/view/4tcBDj

	vec2 u = floor(gl_FragCoord.xy);
	bvec2 v = not(equal(u * u.yx / u, u.yx));
	out_Color = vec4(v, v.x||v.y, 1);
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT fragShader, vulkan, {frag_shader_source}, "main", EShLangFragment ));
	}

	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 0;
		info.pSetLayouts			= null;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &graphicsPplnLayout ));
	}

	VkPipelineShaderStageCreateInfo			stages[2] = {};
	stages[0].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage		= VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module	= vertShader;
	stages[0].pName		= "main";
	stages[1].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage		= VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module	= fragShader;
	stages[1].pName		= "main";

	VkPipelineVertexInputStateCreateInfo	vertex_input = {};
	vertex_input.sType		= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	VkPipelineInputAssemblyStateCreateInfo	input_assembly = {};
	input_assembly.sType	= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology	= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	VkPipelineViewportStateCreateInfo		viewport = {};
	viewport.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport.viewportCount	= 1;
	viewport.scissorCount	= 1;

	VkPipelineRasterizationStateCreateInfo	rasterization = {};
	rasterization.sType			= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode	= VK_POLYGON_MODE_FILL;
	rasterization.cullMode		= VK_CULL_MODE_NONE;
	rasterization.frontFace		= VK_FRONT_FACE_COUNTER_CLOCKWISE;

	VkPipelineMultisampleStateCreateInfo	multisample = {};
	multisample.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo	depth_stencil = {};
	depth_stencil.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable		= VK_FALSE;
	depth_stencil.depthWriteEnable		= VK_FALSE;
	depth_stencil.depthCompareOp		= VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil.depthBoundsTestEnable	= VK_FALSE;
	depth_stencil.stencilTestEnable		= VK_FALSE;

	VkPipelineColorBlendAttachmentState		blend_attachment = {};
	blend_attachment.blendEnable		= VK_FALSE;
	blend_attachment.colorWriteMask		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo		blend_state = {};
	blend_state.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend_state.logicOpEnable		= VK_FALSE;
	blend_state.attachmentCount		= 1;
	blend_state.pAttachments		= &blend_attachment;

	const VkDynamicState					dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo		dynamic_state	 = {};
	dynamic_state.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount	= uint(CountOf( dynamic_states ));
	dynamic_state.pDynamicStates	= dynamic_states;

	// create pipeline
	VkGraphicsPipelineCreateInfo	info = {};
	info.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.stageCount				= uint(CountOf( stages ));
	info.pStages				= stages;
	info.pViewportState			= &viewport;
	info.pVertexInputState		= &vertex_input;
	info.pInputAssemblyState	= &input_assembly;
	info.pRasterizationState	= &rasterization;
	info.pMultisampleState		= &multisample;
	info.pDepthStencilState		= &depth_stencil;
	info.pColorBlendState		= &blend_state;
	info.pDynamicState			= &dynamic_state;
	info.layout					= graphicsPplnLayout;
	info.renderPass				= renderPass;
	info.subpass				= 0;

	VK_CHECK( vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &graphicsPpln ));
	return true;
}

/*
=================================================
	CreateComputePipeline
=================================================
*/
bool AsyncComputeApp::CreateComputePipeline ()
{
	// create compute shader
	{
		static const char	comp_shader_source[] = R"#(
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;	// == computeGroupSize

layout(binding = 0)				uniform sampler2D  un_Texture;
layout(binding = 1) writeonly	uniform image2D	un_SwapchainImage;

void main()
{
	vec2	point = (vec3(gl_GlobalInvocationID) / vec3(gl_NumWorkGroups * gl_WorkGroupSize - 1)).xy;

	vec4	color = texture( un_Texture, point ).brga;

	imageStore( un_SwapchainImage, ivec2(gl_GlobalInvocationID.xy), color );
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT computeShader, vulkan, {comp_shader_source}, "main", EShLangCompute ));
	}

	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &dsLayoutCompute;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &computePplnLayout ));
	}

	VkComputePipelineCreateInfo		info = {};
	info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
	info.stage.module	= computeShader;
	info.stage.pName	= "main";
	info.layout			= computePplnLayout;

	VK_CHECK( vkCreateComputePipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &computePpln ));
	return true;
}
}	// anonymous namespace

/*
=================================================
	AsyncCompute_Sample1
=================================================
*/
extern void AsyncCompute_Sample1 ()
{
	AsyncComputeApp	app;
	
	if ( app.Initialize() )
	{
		app.Run();
		app.Destroy();
	}
}

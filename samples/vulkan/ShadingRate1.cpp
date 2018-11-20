// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "compiler/SpvCompiler.h"
#include "stl/Algorithms/StringUtils.h"

using namespace FG;
namespace {


class ShadingRateApp final : public IWindowEventListener, public VulkanDeviceFn
{
private:
	VulkanDeviceExt			vulkan;
	VulkanSwapchainPtr		swapchain;
	WindowPtr				window;
	SpvCompiler				spvCompiler;
	
	VkCommandPool			cmdPool			= VK_NULL_HANDLE;
	VkQueue					cmdQueue		= VK_NULL_HANDLE;
	VkCommandBuffer			cmdBuffers[2]	= {};
	VkFence					fences[2]		= {};
	VkSemaphore				semaphores[2]	= {};

	VkRenderPass			renderPass		= VK_NULL_HANDLE;
	VkFramebuffer			framebuffers[8]	= {};

	VkPipeline				pipelineFSQ		= VK_NULL_HANDLE;	// draw full screen quad
	VkPipelineLayout		pplnLayoutFSQ	= VK_NULL_HANDLE;
	VkShaderModule			vertShader		= VK_NULL_HANDLE;
	VkShaderModule			fragShader		= VK_NULL_HANDLE;

	VkPipeline				pipelineGenSRI	= VK_NULL_HANDLE;	// generate shading rate image
	VkPipelineLayout		pplnLayoutSRI	= VK_NULL_HANDLE;
	VkShaderModule			compShader		= VK_NULL_HANDLE;
	
	VkDescriptorSetLayout	dsLayout		= VK_NULL_HANDLE;
	VkDescriptorPool		descriptorPool	= VK_NULL_HANDLE;
	VkDescriptorSet			descriptorSet	= VK_NULL_HANDLE;
	
	VkImage					shadingRateImg	= VK_NULL_HANDLE;
	VkImageView				shadingRateView	= VK_NULL_HANDLE;
	VkDeviceMemory			sharedMemory	= VK_NULL_HANDLE;

	bool					looping			= true;

	static constexpr char	title[]			= "Shading rate image sample";


public:
	ShadingRateApp ()
	{
		VulkanDeviceFn_Init( vulkan );
	}
	
	void OnKey (StringView key, EKeyAction action) override;
	void OnResize (const uint2 &size) override;
	
	void OnRefresh () override {}
	void OnDestroy () override {}
	void OnUpdate () override {}

	bool Initialize ();
	void Destroy ();
	bool Run ();

	bool CreateCommandBuffers ();
	bool CreateSyncObjects ();
	bool CreateRenderPass ();
	bool CreateFramebuffers ();
	void DestroyFramebuffers ();
	bool CreateResources ();
	void DestroyResources ();
	bool CreateDescriptorSet ();
	bool CreatePipelineFSQ ();
	bool CreatePipelineGenSRI ();

	ND_ bool IsShadingRateImageSupported () const		{ return vulkan.GetDeviceShadingRateImageFeatures().shadingRateImage; }
};



/*
=================================================
	OnKey
=================================================
*/
void ShadingRateApp::OnKey (StringView key, EKeyAction action)
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
void ShadingRateApp::OnResize (const uint2 &size)
{
	VK_CALL( vkDeviceWaitIdle( vulkan.GetVkDevice() ));

	VK_CALL( vkResetCommandPool( vulkan.GetVkDevice(), cmdPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT ));
	DestroyFramebuffers();
	DestroyResources();

	CHECK( swapchain->Recreate( size ));

	CHECK( CreateFramebuffers() );
	CHECK( CreateResources() );
}

/*
=================================================
	Initialize
=================================================
*/
bool ShadingRateApp::Initialize ()
{
# if defined(FG_ENABLE_GLFW)
	window.reset( new WindowGLFW() );

# elif defined(FG_ENABLE_SDL2)
	window.reset( new WindowSDL2() );

# elif defined(FG_ENABLE_SFML)
	window.reset( new WindowSFML() );

# else
#	error unknown window library!
# endif
	

	// create window and vulkan device
	{
		CHECK_ERR( window->Create( { 800, 600 }, title ));
		window->AddListener( this );

		CHECK_ERR( vulkan.Create( window->GetVulkanSurface(),
								  "sample", "Engine",
								  VK_API_VERSION_1_1,
								  "",
								  {{ VK_QUEUE_PRESENT_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0.0f }},
								  VulkanDevice::GetRecomendedInstanceLayers(),
								  { VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME },
								  { VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME }
			));
		
		//vulkan.CreateDebugReportCallback( DebugReportFlags_All );
		vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All );

		CHECK_ERR( IsShadingRateImageSupported() );
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
	cmdQueue = vulkan.GetVkQuues().front().id;

	CHECK_ERR( CreateCommandBuffers() );
	CHECK_ERR( CreateSyncObjects() );
	CHECK_ERR( CreateRenderPass() );
	CHECK_ERR( CreateFramebuffers() );
	CHECK_ERR( CreateDescriptorSet() );
	CHECK_ERR( CreatePipelineFSQ() );
	CHECK_ERR( CreatePipelineGenSRI() );
	CHECK_ERR( CreateResources() );
	return true;
}

/*
=================================================
	Destroy
=================================================
*/
void ShadingRateApp::Destroy ()
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
	vkDestroyCommandPool( dev, cmdPool, null );
	vkDestroyRenderPass( dev, renderPass, null );
	vkDestroyPipeline( dev, pipelineFSQ, null );
	vkDestroyPipeline( dev, pipelineGenSRI, null );
	vkDestroyPipelineLayout( dev, pplnLayoutFSQ, null );
	vkDestroyPipelineLayout( dev, pplnLayoutSRI, null );
	vkDestroyShaderModule( dev, vertShader, null );
	vkDestroyShaderModule( dev, fragShader, null );
	vkDestroyShaderModule( dev, compShader, null );
	vkDestroyDescriptorSetLayout( vulkan.GetVkDevice(), dsLayout, null );
	vkDestroyDescriptorPool( vulkan.GetVkDevice(), descriptorPool, null );

	cmdPool			= VK_NULL_HANDLE;
	cmdQueue		= VK_NULL_HANDLE;
	pipelineFSQ		= VK_NULL_HANDLE;
	pplnLayoutFSQ	= VK_NULL_HANDLE;
	pipelineGenSRI	= VK_NULL_HANDLE;
	pplnLayoutSRI	= VK_NULL_HANDLE;
	vertShader		= VK_NULL_HANDLE;
	fragShader		= VK_NULL_HANDLE;
	compShader		= VK_NULL_HANDLE;
	dsLayout		= VK_NULL_HANDLE;
	descriptorPool	= VK_NULL_HANDLE;
	descriptorSet	= VK_NULL_HANDLE;

	DestroyResources();
	DestroyFramebuffers();
	swapchain->Destroy();
	swapchain.reset();

	vulkan.Destroy();

	window->Destroy();
	window.reset();
}

/*
=================================================
	Run
=================================================
*/
bool ShadingRateApp::Run ()
{
	for (uint frameId = 0; looping; frameId = ((frameId + 1) & 1))
	{
		if ( not window->Update() )
			break;

		// wait and acquire next image
		{
			VK_CHECK( vkWaitForFences( vulkan.GetVkDevice(), 1, &fences[frameId], true, ~0ull ));
			VK_CHECK( vkResetFences( vulkan.GetVkDevice(), 1, &fences[frameId] ));

			VK_CALL( swapchain->AcquireNextImage( semaphores[0] ));
		}

		// build command buffer
		{
			VkCommandBufferBeginInfo	begin_info = {};
			begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			VK_CALL( vkBeginCommandBuffer( cmdBuffers[frameId], &begin_info ));
			
			// begin render pass
			{
                VkClearValue			clear_value = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
				VkRenderPassBeginInfo	begin		= {};
				begin.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				begin.framebuffer		= framebuffers[ swapchain->GetCurretImageIndex() ];
				begin.renderPass		= renderPass;
				begin.renderArea		= { {0,0}, {swapchain->GetSurfaceSize().x, swapchain->GetSurfaceSize().y} };
				begin.clearValueCount	= 1;
				begin.pClearValues		= &clear_value;

				vkCmdBeginRenderPass( cmdBuffers[frameId], &begin, VK_SUBPASS_CONTENTS_INLINE );
			}
			
			vkCmdBindPipeline( cmdBuffers[frameId], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineFSQ );
			
			// set dynamic states
			{
				VkViewport	viewport = {};
				viewport.x			= 0.0f;
				viewport.y			= 0.0f;
				viewport.width		= float(swapchain->GetSurfaceSize().x);
				viewport.height		= float(swapchain->GetSurfaceSize().y);
				viewport.minDepth	= 0.0f;
				viewport.maxDepth	= 1.0f;

				vkCmdSetViewport( cmdBuffers[frameId], 0, 1, &viewport );

				VkRect2D	scissor_rect = { {0,0}, {swapchain->GetSurfaceSize().x, swapchain->GetSurfaceSize().y} };

				vkCmdSetScissor( cmdBuffers[frameId], 0, 1, &scissor_rect );
			}
			
			// set dynamic shading rate pallete
			{
				const VkShadingRatePaletteEntryNV	palette_entries[] = {
					VK_SHADING_RATE_PALETTE_ENTRY_8_INVOCATIONS_PER_PIXEL_NV,			// 1x1		8 invocations
					VK_SHADING_RATE_PALETTE_ENTRY_4_INVOCATIONS_PER_PIXEL_NV,			// 1x1		4
					VK_SHADING_RATE_PALETTE_ENTRY_2_INVOCATIONS_PER_PIXEL_NV,			// 1x1		2
					//VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,					// none
					VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_PIXEL_NV,			// 1x1		1
					VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X1_PIXELS_NV,		// 2x1		1
					VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X2_PIXELS_NV,		// 2x2		1
					VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X4_PIXELS_NV,		// 2x4		1
					VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X4_PIXELS_NV,		// 4x4		1
				};
				const VkShadingRatePaletteNV	palette = { uint(CountOf( palette_entries )), &palette_entries[0] };

				vkCmdSetViewportShadingRatePaletteNV( cmdBuffers[frameId], 0, 1, &palette );
			}

			// draw
			{
				vkCmdBindShadingRateImageNV( cmdBuffers[frameId], shadingRateView, VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV );
			
				vkCmdDraw( cmdBuffers[frameId], 4, 1, 0, 0 );
			}

			vkCmdEndRenderPass( cmdBuffers[frameId] );

			VK_CALL( vkEndCommandBuffer( cmdBuffers[frameId] ));
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
			submit_info.pCommandBuffers			= &cmdBuffers[frameId];
			submit_info.waitSemaphoreCount		= uint(CountOf(wait_semaphores));
			submit_info.pWaitSemaphores			= wait_semaphores;
			submit_info.pWaitDstStageMask		= wait_dst_mask;
			submit_info.signalSemaphoreCount	= uint(CountOf(signal_semaphores));
			submit_info.pSignalSemaphores		= signal_semaphores;

			VK_CHECK( vkQueueSubmit( cmdQueue, 1, &submit_info, fences[frameId] ));
		}

		// present
		VkResult	err = swapchain->Present( cmdQueue, {semaphores[1]} );
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
	return true;
}

/*
=================================================
	CreateCommandBuffers
=================================================
*/
bool ShadingRateApp::CreateCommandBuffers ()
{
	VkCommandPoolCreateInfo		pool_info = {};
	pool_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex	= vulkan.GetVkQuues().front().familyIndex;
	pool_info.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK( vkCreateCommandPool( vulkan.GetVkDevice(), &pool_info, null, OUT &cmdPool ));

	VkCommandBufferAllocateInfo	info = {};
	info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.pNext				= null;
	info.commandPool		= cmdPool;
	info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount	= uint(CountOf( cmdBuffers ));
	VK_CHECK( vkAllocateCommandBuffers( vulkan.GetVkDevice(), &info, OUT cmdBuffers ));

	return true;
}

/*
=================================================
	CreateSyncObjects
=================================================
*/
bool ShadingRateApp::CreateSyncObjects ()
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
bool ShadingRateApp::CreateRenderPass ()
{
	// setup attachment
	VkAttachmentDescription		attachments[1] = {};

	attachments[0].format			= swapchain->GetColorFormat();
	attachments[0].samples			= VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


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
	dependencies[0].srcAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass		= 0;
	dependencies[1].dstSubpass		= VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask	= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
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
	CreateFramebuffers
=================================================
*/
bool ShadingRateApp::CreateFramebuffers ()
{
	VkFramebufferCreateInfo	fb_info = {};
	fb_info.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.renderPass		= renderPass;
	fb_info.attachmentCount	= 1;
	fb_info.width			= swapchain->GetSurfaceSize().x;
	fb_info.height			= swapchain->GetSurfaceSize().y;
	fb_info.layers			= 1;

	for (uint i = 0; i < swapchain->GetSwapchainLength(); ++i)
	{
		VkImageView	view = swapchain->GetImageView( i );
		fb_info.pAttachments = &view;

		VK_CALL( vkCreateFramebuffer( vulkan.GetVkDevice(), &fb_info, null, OUT &framebuffers[i] ));
	}
	return true;
}

/*
=================================================
	DestroyFramebuffers
=================================================
*/
void ShadingRateApp::DestroyFramebuffers ()
{
	for (uint i = 0; i < swapchain->GetSwapchainLength(); ++i)
	{
		vkDestroyFramebuffer( vulkan.GetVkDevice(), framebuffers[i], null );
	}
}

/*
=================================================
	CreateResources
=================================================
*/
bool ShadingRateApp::CreateResources ()
{
	VkDeviceSize					total_size		= 0;
	uint							mem_type_bits	= 0;
	VkMemoryPropertyFlags			mem_property	= 0;
	Array<std::function<void ()>>	bind_mem;
	uint2							sr_image_size;

	// create shading rate image
	{
		const auto&		sri_size	= vulkan.GetDeviceShadingRateImageProperties().shadingRateTexelSize;
		const auto&		surf_size	= swapchain->GetSurfaceSize();
		sr_image_size = { surf_size.x / sri_size.width + 1, surf_size.y / sri_size.height + 1 };

		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.flags			= 0;
		info.imageType		= VK_IMAGE_TYPE_2D;
		info.format			= VK_FORMAT_R8_UINT;
		info.extent			= { sr_image_size.x, sr_image_size.y, 1 };
		info.mipLevels		= 1;
		info.arrayLayers	= 1;
		info.samples		= VK_SAMPLE_COUNT_1_BIT;
		info.tiling			= VK_IMAGE_TILING_OPTIMAL;
		info.usage			= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;

		VK_CHECK( vkCreateImage( vulkan.GetVkDevice(), &info, null, OUT &shadingRateImg ));
		
		VkMemoryRequirements	mem_req;
		vkGetImageMemoryRequirements( vulkan.GetVkDevice(), shadingRateImg, OUT &mem_req );
		
		VkDeviceSize	offset = AlignToLarger( total_size, mem_req.alignment );
		total_size		 = offset + mem_req.size;
		mem_type_bits	|= mem_req.memoryTypeBits;
		mem_property	|= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		bind_mem.push_back( [this, offset] () {
			VK_CALL( vkBindImageMemory( vulkan.GetVkDevice(), shadingRateImg, sharedMemory, offset ));

            VkImageViewCreateInfo	view = {};
            view.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view.image				= shadingRateImg;
            view.viewType			= VK_IMAGE_VIEW_TYPE_2D;
            view.format				= VK_FORMAT_R8_UINT;
            view.components			= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
            view.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            VK_CALL( vkCreateImageView( vulkan.GetVkDevice(), &view, null, OUT &shadingRateView ));
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

	// update descriptor set
	{
		VkDescriptorImageInfo	images[1] = {};
		images[0].imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		images[0].imageView		= shadingRateView;
		images[0].sampler		= VK_NULL_HANDLE;

		VkWriteDescriptorSet	writes[1] = {};
		writes[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet			= descriptorSet;
		writes[0].dstBinding		= 0;
		writes[0].descriptorCount	= uint(CountOf( images ));
		writes[0].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[0].pImageInfo		= images;

		vkUpdateDescriptorSets( vulkan.GetVkDevice(), uint(CountOf( writes )), writes, 0, null );
	}

	// update resources
	{
		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vkBeginCommandBuffer( cmdBuffers[0], &begin_info ));

		#if 1
		// generate shading rate image
		{
			// undefined -> general
			VkImageMemoryBarrier	barrier = {};
			barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image				= shadingRateImg;
			barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout			= VK_IMAGE_LAYOUT_GENERAL;
			barrier.srcAccessMask		= 0;
			barrier.dstAccessMask		= VK_ACCESS_SHADER_WRITE_BIT;
			barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			
			vkCmdPipelineBarrier( cmdBuffers[0],
								  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
								  0, null, 0, null, 1, &barrier );

			// generate image
			vkCmdBindDescriptorSets( cmdBuffers[0], VK_PIPELINE_BIND_POINT_COMPUTE, pplnLayoutSRI, 0, 1, &descriptorSet, 0, null );
			vkCmdBindPipeline( cmdBuffers[0], VK_PIPELINE_BIND_POINT_COMPUTE, pipelineGenSRI );

			vkCmdDispatch( cmdBuffers[0], sr_image_size.x, sr_image_size.y, 1 );
			
			// general -> shading_rate
			barrier.oldLayout			= VK_IMAGE_LAYOUT_GENERAL;
			barrier.newLayout			= VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV;
			barrier.srcAccessMask		= VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask		= 0;

			vkCmdPipelineBarrier( cmdBuffers[0],
								  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}
		#else
		// clear shading rate image
		{
			// undefined -> transfer_dst
			VkImageMemoryBarrier	barrier = {};
			barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image				= shadingRateImg;
			barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask		= 0;
			barrier.dstAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier( cmdBuffers[0],
								  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
								  0, null, 0, null, 1, &barrier );

			// clear image
			VkClearColorValue		clear_value	= {};
			VkImageSubresourceRange	range		= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			clear_value.uint32[0] = 6;

			vkCmdClearColorImage( cmdBuffers[0], shadingRateImg, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_value, 1, &range );
			
			// transfer_dst -> shading_rate
			barrier.oldLayout		= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout		= VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV;
			barrier.srcAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask	= 0;

			vkCmdPipelineBarrier( cmdBuffers[0],
								  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}
		#endif

		VK_CALL( vkEndCommandBuffer( cmdBuffers[0] ));

		VkSubmitInfo		submit_info = {};
		submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount	= 1;
		submit_info.pCommandBuffers		= &cmdBuffers[0];

		VK_CALL( vkQueueSubmit( cmdQueue, 1, &submit_info, VK_NULL_HANDLE ));
		VK_CALL( vkQueueWaitIdle( cmdQueue ));
	}
	return true;
}

/*
=================================================
	DestroyResources
=================================================
*/
void ShadingRateApp::DestroyResources ()
{
	VkDevice	dev = vulkan.GetVkDevice();

	vkDestroyImageView( dev, shadingRateView, null );
	vkDestroyImage( dev, shadingRateImg, null );
	vkFreeMemory( dev, sharedMemory, null );

	shadingRateImg	= VK_NULL_HANDLE;
	shadingRateView	= VK_NULL_HANDLE;
	sharedMemory	= VK_NULL_HANDLE;
}

/*
=================================================
	CreateDescriptorSet
=================================================
*/
bool ShadingRateApp::CreateDescriptorSet ()
{
	// create layout
	{
		VkDescriptorSetLayoutBinding		binding[1] = {};
		binding[0].binding			= 0;
		binding[0].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding[0].descriptorCount	= 1;
		binding[0].stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= uint(CountOf( binding ));
		info.pBindings		= binding;

		VK_CHECK( vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &dsLayout ));
	}

	// create pool
	{
		const VkDescriptorPoolSize		sizes[] = {
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 }
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
		info.pSetLayouts		= &dsLayout;

		VK_CHECK( vkAllocateDescriptorSets( vulkan.GetVkDevice(), &info, OUT &descriptorSet ));
	}
	return true;
}

/*
=================================================
	CreatePipelineFSQ
=================================================
*/
bool ShadingRateApp::CreatePipelineFSQ ()
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
	// see https://github.com/KhronosGroup/GLSL/blob/master/extensions/nv/GLSL_NV_shading_rate_image.txt
	{
		static const char	frag_shader_source[] = R"#(
#extension GL_NV_shading_rate_image : require

layout(location = 0) out vec4  out_Color;

//in ivec2 gl_FragmentSizeNV;
//in int   gl_InvocationsPerPixelNV;

const vec3 g_Colors[] = {
	vec3( 1.0, 0.0, 0.0 ),	// 0
	vec3( 0.5, 0.0, 0.0 ),	// 1
	vec3( 0.0 ),
	vec3( 0.0, 1.0, 0.0 ),	// 3
	vec3( 0.0, 0.5, 0.0 ),	// 4
	vec3( 0.0, 0.0, 1.0 ),	// 5
	vec3( 0.0 ),
	vec3( 0.0, 0.0, 0.5 ),	// 7
	vec3( 0.0 ),
	vec3( 0.0 ),
	vec3( 0.0 ),
	vec3( 0.0 ),
	vec3( 1.0, 1.0, 0.0 ),	// 12
	vec3( 0.5, 1.0, 0.0 ),	// 13
	vec3( 0.0 ),
	vec3( 0.0, 1.0, 1.0 )	// 15
};

void main ()
{
	if ( gl_InvocationsPerPixelNV > 1 )
	{
		out_Color = vec4(1.0);	// why this doesn't work?
		//out_Color = vec4( g_Colors[ clamp(gl_InvocationsPerPixelNV, 0, 16) ], 1.0 );	// 1, 2, 4, 8, 16
	}
	else
	{
		int	index = (gl_FragmentSizeNV.x-1) + (gl_FragmentSizeNV.y-1) * 4;	// 0, 1, 3, 4, 5, 7, 12, 13, 15

		out_Color = vec4( g_Colors[ clamp(index, 0, 16) ], 1.0 );
	}
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

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &pplnLayoutFSQ ));
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

	VkDynamicState							dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT_SHADING_RATE_PALETTE_NV };
	VkPipelineDynamicStateCreateInfo		dynamic_state = {};
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
	info.layout					= pplnLayoutFSQ;
	info.renderPass				= renderPass;
	info.subpass				= 0;

	VK_CHECK( vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &pipelineFSQ ));
	return true;
}

/*
=================================================
	CreatePipelineGenSRI
=================================================
*/
bool ShadingRateApp::CreatePipelineGenSRI ()
{
	// create vertex shader
	{
		static const char	comp_shader_source[] = R"#(
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) writeonly uniform uimage2D  out_ShadingRateImage;

void main()
{
	vec2	point = (vec3(gl_GlobalInvocationID) / vec3(gl_NumWorkGroups * gl_WorkGroupSize - 1)).xy * 2.0 - 1.0;

	uint	index = clamp( uint(length( point ) * 7.0 + 0.5), 0, 7 );

	imageStore( out_ShadingRateImage, ivec2(gl_GlobalInvocationID.xy), uvec4(index) );
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT compShader, vulkan, {comp_shader_source}, "main", EShLangCompute ));
	}

	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &dsLayout;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &pplnLayoutSRI ));
	}

	VkComputePipelineCreateInfo		info = {};
	info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
	info.stage.module	= compShader;
	info.stage.pName	= "main";
	info.layout			= pplnLayoutSRI;

	VK_CHECK( vkCreateComputePipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &pipelineGenSRI ));
	return true;
}
}	// anonymous namespace

/*
=================================================
	ShadingRateImage_Sample1
=================================================
*/
extern void ShadingRateImage_Sample1 ()
{
	ShadingRateApp	app;
	
	if ( app.Initialize() )
	{
		app.Run();
		app.Destroy();
	}
}

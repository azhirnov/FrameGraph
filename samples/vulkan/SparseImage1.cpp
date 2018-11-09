// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "compiler/SpvCompiler.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Math/Color.h"

using namespace FG;
namespace {


class SparseImageApp final : public IWindowEventListener, public VulkanDeviceFn
{
	using TimePoint_t	= std::chrono::time_point< std::chrono::high_resolution_clock >;

private:
	VulkanDeviceExt			vulkan;
	VulkanSwapchainPtr		swapchain;
	WindowPtr				window;
	SpvCompiler				spvCompiler;
	
	VkCommandPool			cmdPool			= VK_NULL_HANDLE;
	VkQueue					cmdQueue		= VK_NULL_HANDLE;
	VkCommandBuffer			cmdBuffers[2]	= {};
	VkFence					fences[2]		= {};
	VkSemaphore				semaphores[4]	= {};

	VkRenderPass			renderPass		= VK_NULL_HANDLE;
	VkFramebuffer			framebuffers[8]	= {};

	VkPipeline				pipeline		= VK_NULL_HANDLE;
	VkPipelineLayout		pplnLayout		= VK_NULL_HANDLE;
	VkShaderModule			vertShader		= VK_NULL_HANDLE;
	VkShaderModule			fragShader		= VK_NULL_HANDLE;
	
	VkDescriptorSetLayout	dsLayout		= VK_NULL_HANDLE;
	VkDescriptorPool		descriptorPool	= VK_NULL_HANDLE;
	VkDescriptorSet			descriptorSet	= VK_NULL_HANDLE;
	
	VkImage					sparseImage		= VK_NULL_HANDLE;
	VkImageView				sparseImageView	= VK_NULL_HANDLE;
	VkDeviceMemory			sharedMemory	= VK_NULL_HANDLE;
	VkSampler				sampler			= VK_NULL_HANDLE;

	Array<VkSparseImageMemoryBind>	imageBlocks;

	bool					looping			= true;

	static constexpr char	title[]			= "Sparse image sample";


public:
	SparseImageApp ()
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
	bool CreateSampler ();
	bool CreateDescriptorSet ();
	bool CreatePipeline ();

	ND_ bool IsSparseImageSupported () const
	{
		return	vulkan.GetDeviceFeatures().sparseBinding			and
				vulkan.GetDeviceFeatures().sparseResidencyAliased	and
				vulkan.GetDeviceFeatures().sparseResidencyBuffer	and
				vulkan.GetDeviceFeatures().sparseResidencyImage2D;
	}
};



/*
=================================================
	OnKey
=================================================
*/
void SparseImageApp::OnKey (StringView key, EKeyAction action)
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
void SparseImageApp::OnResize (const uint2 &size)
{
	VK_CALL( vkDeviceWaitIdle( vulkan.GetVkDevice() ));

	VK_CALL( vkResetCommandPool( vulkan.GetVkDevice(), cmdPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT ));
	DestroyFramebuffers();

	CHECK( swapchain->Recreate( size ));
	CHECK( CreateFramebuffers() );
}

/*
=================================================
	Initialize
=================================================
*/
bool SparseImageApp::Initialize ()
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
								  {{ VK_QUEUE_PRESENT_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_SPARSE_BINDING_BIT, 0.0f }},
								  VulkanDevice::GetRecomendedInstanceLayers(),
								  { VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME },
								  {}
			));
		
		vulkan.CreateDebugCallback( VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT );

		CHECK_ERR( IsSparseImageSupported() );
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
	CHECK_ERR( CreateResources() );
	CHECK_ERR( CreateSampler() );
	CHECK_ERR( CreateDescriptorSet() );
	CHECK_ERR( CreatePipeline() );
	return true;
}

/*
=================================================
	Destroy
=================================================
*/
void SparseImageApp::Destroy ()
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
	vkDestroyPipeline( dev, pipeline, null );
	vkDestroyPipelineLayout( dev, pplnLayout, null );
	vkDestroyShaderModule( dev, vertShader, null );
	vkDestroyShaderModule( dev, fragShader, null );
	vkDestroyDescriptorSetLayout( dev, dsLayout, null );
	vkDestroyDescriptorPool( dev, descriptorPool, null );
	vkDestroyImage( dev, sparseImage, null );
	vkDestroyImageView( dev, sparseImageView, null );
	vkDestroySampler( dev, sampler, null );
	vkFreeMemory( dev, sharedMemory, null );

	cmdPool			= VK_NULL_HANDLE;
	cmdQueue		= VK_NULL_HANDLE;
	pipeline		= VK_NULL_HANDLE;
	pplnLayout		= VK_NULL_HANDLE;
	vertShader		= VK_NULL_HANDLE;
	fragShader		= VK_NULL_HANDLE;
	dsLayout		= VK_NULL_HANDLE;
	descriptorPool	= VK_NULL_HANDLE;
	descriptorSet	= VK_NULL_HANDLE;
	sampler			= VK_NULL_HANDLE;
	sparseImage		= VK_NULL_HANDLE;
	sparseImageView	= VK_NULL_HANDLE;
	sharedMemory	= VK_NULL_HANDLE;

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
bool SparseImageApp::Run ()
{	
	TimePoint_t		last_sparse_rebind = TimePoint_t::clock::now();

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
		
		// bind sparse memory to image
		VkSemaphore		sparse_binding_sem = VK_NULL_HANDLE;
		{
			const TimePoint_t	time = TimePoint_t::clock::now();

			if ( time - last_sparse_rebind > std::chrono::seconds(2) )
			{
				last_sparse_rebind	= time;
				sparse_binding_sem	= semaphores[frameId + 2];

                const uint	src_idx = uint(rand() % imageBlocks.size());
				uint		dst_idx = src_idx;

				while ( src_idx == dst_idx ) {
                    dst_idx = uint(rand() % imageBlocks.size());
				}
				
				std::swap( imageBlocks[src_idx].memoryOffset, imageBlocks[dst_idx].memoryOffset );

				const VkSparseImageMemoryBind	img_binds[] = {
					imageBlocks[ src_idx ],
					imageBlocks[ dst_idx ]
				};

				VkSparseImageMemoryBindInfo		img_info = {};
				img_info.image		= sparseImage;
				img_info.bindCount	= uint(CountOf( img_binds ));
				img_info.pBinds		= img_binds;

				VkBindSparseInfo				binding = {};
				binding.sType				= VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
				binding.imageBindCount		= 1;
				binding.pImageBinds			= &img_info;
				binding.signalSemaphoreCount= 1;
				binding.pSignalSemaphores	= &sparse_binding_sem;

				VK_CALL( vkQueueBindSparse( cmdQueue, 1, &binding, VK_NULL_HANDLE ));
			}
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
			
			vkCmdBindDescriptorSets( cmdBuffers[frameId], VK_PIPELINE_BIND_POINT_GRAPHICS, pplnLayout, 0, 1, &descriptorSet, 0, null );
			vkCmdBindPipeline( cmdBuffers[frameId], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
			
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

			// draw
			{
				vkCmdDraw( cmdBuffers[frameId], 4, 1, 0, 0 );
			}

			vkCmdEndRenderPass( cmdBuffers[frameId] );

			VK_CALL( vkEndCommandBuffer( cmdBuffers[frameId] ));
		}


		// submit commands
		{
			VkSemaphore				signal_semaphores[] = { semaphores[1] };
			VkSemaphore				wait_semaphores[]	= { semaphores[0], sparse_binding_sem };
			VkPipelineStageFlags	wait_dst_mask[]		= { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
			STATIC_ASSERT( CountOf(wait_semaphores) == CountOf(wait_dst_mask) );

			VkSubmitInfo				submit_info = {};
			submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.commandBufferCount		= 1;
			submit_info.pCommandBuffers			= &cmdBuffers[frameId];
			submit_info.waitSemaphoreCount		= uint(CountOf( wait_semaphores )) - uint(sparse_binding_sem == VK_NULL_HANDLE);
			submit_info.pWaitSemaphores			= wait_semaphores;
			submit_info.pWaitDstStageMask		= wait_dst_mask;
			submit_info.signalSemaphoreCount	= uint(CountOf( signal_semaphores ));
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
bool SparseImageApp::CreateCommandBuffers ()
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
bool SparseImageApp::CreateSyncObjects ()
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
bool SparseImageApp::CreateRenderPass ()
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
	dependencies[1].dstAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
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
bool SparseImageApp::CreateFramebuffers ()
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
void SparseImageApp::DestroyFramebuffers ()
{
	for (uint i = 0; i < swapchain->GetSwapchainLength(); ++i)
	{
		vkDestroyFramebuffer( vulkan.GetVkDevice(), framebuffers[i], null );
	}
}

/*
=================================================
	CreateSampler
=================================================
*/
bool SparseImageApp::CreateSampler ()
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
bool SparseImageApp::CreateResources ()
{
	VkDeviceSize					total_size		= 0;
	uint							mem_type_bits	= 0;
	VkMemoryPropertyFlags			mem_property	= 0;
	uint2							image_size;
	const uint2						num_tiles		{ 4, 4 };
	uint2							tile_size;
	VkMemoryRequirements			img_mem_req		= {};
	VkMemoryRequirements			buf_mem_req		= {};

	// create sparse image
	{
		VkFormat	img_format = VK_FORMAT_R8G8B8A8_UNORM;

		// find supported format
		{
			VkPhysicalDeviceSparseImageFormatInfo2	fmt_info = {};
			fmt_info.sType		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SPARSE_IMAGE_FORMAT_INFO_2;
			fmt_info.format		= img_format;
			fmt_info.type		= VK_IMAGE_TYPE_2D;
			fmt_info.samples	= VK_SAMPLE_COUNT_1_BIT;
			fmt_info.usage		= VK_IMAGE_USAGE_SAMPLED_BIT;
			fmt_info.tiling		= VK_IMAGE_TILING_OPTIMAL;

			uint	count = 0;
			vkGetPhysicalDeviceSparseImageFormatProperties2( vulkan.GetVkPhysicalDevice(), &fmt_info, OUT &count, null );
			CHECK_ERR( count > 0 );

			Array<VkSparseImageFormatProperties2>	fmt_properties{ count };
			for (auto& prop : fmt_properties) {
				prop.sType = VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2;
				prop.pNext = null;
			}
			vkGetPhysicalDeviceSparseImageFormatProperties2( vulkan.GetVkPhysicalDevice(), &fmt_info, OUT &count, fmt_properties.data() );

			for (auto& prop : fmt_properties) {
				if ( prop.properties.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT ) {
					tile_size.x = prop.properties.imageGranularity.width;
					tile_size.y = prop.properties.imageGranularity.height;
					image_size  = tile_size * num_tiles;
					break;
				}
			}
			CHECK_ERR(All( tile_size > 0 ));
			CHECK_ERR(All( image_size > 0 ));
		}

		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.flags			= VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT | VK_IMAGE_CREATE_SPARSE_ALIASED_BIT;
		info.imageType		= VK_IMAGE_TYPE_2D;
		info.format			= img_format;
		info.extent			= { image_size.x, image_size.y, 1 };
		info.mipLevels		= 1;
		info.arrayLayers	= 1;
		info.samples		= VK_SAMPLE_COUNT_1_BIT;
		info.tiling			= VK_IMAGE_TILING_OPTIMAL;
		info.usage			= VK_IMAGE_USAGE_SAMPLED_BIT;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;

		VK_CHECK( vkCreateImage( vulkan.GetVkDevice(), &info, null, OUT &sparseImage ));
		
		vkGetImageMemoryRequirements( vulkan.GetVkDevice(), sparseImage, OUT &img_mem_req );

		uint	count = 0;
		vkGetImageSparseMemoryRequirements( vulkan.GetVkDevice(), sparseImage, OUT &count, null );

		Array<VkSparseImageMemoryRequirements>	sparse_req{ count };
		vkGetImageSparseMemoryRequirements( vulkan.GetVkDevice(), sparseImage, OUT &count, sparse_req.data() );

		VkDeviceSize	offset = AlignToLarger( total_size, img_mem_req.alignment );
		total_size		 = offset + img_mem_req.size;
		mem_type_bits	|= img_mem_req.memoryTypeBits;
		mem_property	|= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		VkImageViewCreateInfo	view = {};
		view.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.image				= sparseImage;
		view.viewType			= VK_IMAGE_VIEW_TYPE_2D;
		view.format				= img_format;
		view.components			= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		view.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		VK_CALL( vkCreateImageView( vulkan.GetVkDevice(), &view, null, OUT &sparseImageView ));
	}

	// create sparse buffer for one image tile
	VkBuffer		temp_buffer			= VK_NULL_HANDLE;
	VkDeviceSize	temp_buffer_size	= sizeof(float) * tile_size.x * tile_size.y;
	{
		VkBufferCreateInfo	info = {};
		info.sType		= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags		= VK_BUFFER_CREATE_SPARSE_BINDING_BIT | VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT | VK_BUFFER_CREATE_SPARSE_ALIASED_BIT;
		info.size		= temp_buffer_size;
		info.usage		= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		info.sharingMode= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vkCreateBuffer( vulkan.GetVkDevice(), &info, null, OUT &temp_buffer ));
		
		vkGetBufferMemoryRequirements( vulkan.GetVkDevice(), temp_buffer, OUT &buf_mem_req );

		ASSERT( total_size > buf_mem_req.size );

		mem_type_bits	|= buf_mem_req.memoryTypeBits;
		mem_property	|= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}

	// allocate memory
	{
		VkMemoryAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize		= (total_size *= 2);

		CHECK_ERR( vulkan.GetMemoryTypeIndex( mem_type_bits, mem_property, OUT info.memoryTypeIndex ));

		VK_CHECK( vkAllocateMemory( vulkan.GetVkDevice(), &info, null, OUT &sharedMemory ));
	}

	// initialize image data
	VkDeviceSize	mem_offset	= 0;

	for (uint y = 0; y < num_tiles.y; ++y)
	{
		for (uint x = 0; x < num_tiles.x; ++x)
		{
			// bind sparse memory to buffer
			{
				VkSparseMemoryBind			buf_bind = {};
				buf_bind.resourceOffset		= 0;
				buf_bind.size				= buf_mem_req.size;
				buf_bind.memory				= sharedMemory;
				buf_bind.memoryOffset		= mem_offset;
				buf_bind.flags				= 0;
	
				VkSparseImageMemoryBind		img_bind = {};
				img_bind.subresource		= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
				img_bind.offset				= { int(tile_size.x * x), int(tile_size.y * y), 0 };
				img_bind.extent				= { tile_size.x, tile_size.y, 1 };
				img_bind.memory				= sharedMemory;
				img_bind.memoryOffset		= buf_bind.memoryOffset;
				
				ASSERT( img_bind.memoryOffset % img_mem_req.alignment == 0 );
				imageBlocks.push_back( img_bind );

				VkSparseBufferMemoryBindInfo	buf_info = {};
				buf_info.buffer				= temp_buffer;
				buf_info.bindCount			= 1;
				buf_info.pBinds				= &buf_bind;
				
				VkBindSparseInfo			binding = {};
				binding.sType				= VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
				binding.bufferBindCount		= 1;
				binding.pBufferBinds		= &buf_info;
				binding.signalSemaphoreCount= 1;
				binding.pSignalSemaphores	= &semaphores[0];

				VK_CALL( vkQueueBindSparse( cmdQueue, 1, &binding, VK_NULL_HANDLE ));
			}

			// clear buffer
			{
				VkCommandBufferBeginInfo	begin_info = {};
				begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				VK_CALL( vkBeginCommandBuffer( cmdBuffers[0], &begin_info ));
				
				RGBA32f		color		{ HSVColor{float(x + y * num_tiles.x) / float(num_tiles.x * num_tiles.y)} };
				RGBA8u		rgba		{ color };
				uint		pattern;	MemCopy( pattern, rgba );

				vkCmdFillBuffer( cmdBuffers[0], temp_buffer, 0, temp_buffer_size, pattern );

				VK_CALL( vkEndCommandBuffer( cmdBuffers[0] ));

				VkSubmitInfo			submit_info = {};
				VkPipelineStageFlags	stage_mask	= VK_PIPELINE_STAGE_TRANSFER_BIT;
				submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit_info.commandBufferCount	= 1;
				submit_info.pCommandBuffers		= &cmdBuffers[0];
				submit_info.waitSemaphoreCount	= 1;
				submit_info.pWaitSemaphores		= &semaphores[0];
				submit_info.pWaitDstStageMask	= &stage_mask;

				VK_CALL( vkQueueSubmit( cmdQueue, 1, &submit_info, VK_NULL_HANDLE ));
				VK_CALL( vkQueueWaitIdle( cmdQueue ));
			}

			mem_offset = AlignToLarger( mem_offset + buf_mem_req.size, Max(img_mem_req.alignment, buf_mem_req.alignment) );
			ASSERT( mem_offset <= total_size );
		}
	}
	vkDestroyBuffer( vulkan.GetVkDevice(), temp_buffer, null );

	// bind sparse memory to image
	{
		VkSparseImageMemoryBindInfo		img_info = {};
		img_info.image			= sparseImage;
		img_info.bindCount		= uint(imageBlocks.size());
		img_info.pBinds			= imageBlocks.data();

		VkBindSparseInfo		binding = {};
		binding.sType			= VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
		binding.imageBindCount	= 1;
		binding.pImageBinds		= &img_info;

		VK_CALL( vkQueueBindSparse( cmdQueue, 1, &binding, VK_NULL_HANDLE ));
		VK_CALL( vkQueueWaitIdle( cmdQueue ));
	}
	
	// update image
	{
		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vkBeginCommandBuffer( cmdBuffers[0], &begin_info ));
		
		// undefined -> shader_read_only_optimal
		VkImageMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image				= sparseImage;
		barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask		= 0;
		barrier.dstAccessMask		= VK_ACCESS_SHADER_READ_BIT;
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier( cmdBuffers[0],
								VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
								0, null, 0, null, 1, &barrier );

		VK_CALL( vkEndCommandBuffer( cmdBuffers[0] ));

		VkSubmitInfo		submit_info = {};
		submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount	= 1;
		submit_info.pCommandBuffers		= &cmdBuffers[0];

		VK_CHECK( vkQueueSubmit( cmdQueue, 1, &submit_info, VK_NULL_HANDLE ));
		VK_CALL( vkQueueWaitIdle( cmdQueue ));
	}
	return true;
}

/*
=================================================
	CreateDescriptorSet
=================================================
*/
bool SparseImageApp::CreateDescriptorSet ()
{
	// create layout
	{
		VkDescriptorSetLayoutBinding		binding[1] = {};
		binding[0].binding			= 0;
		binding[0].descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding[0].descriptorCount	= 1;
		binding[0].stageFlags		= VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= uint(CountOf( binding ));
		info.pBindings		= binding;

		VK_CHECK( vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &dsLayout ));
	}

	// create pool
	{
		const VkDescriptorPoolSize		sizes[] = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }
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
	
	// update descriptor set
	{
		VkDescriptorImageInfo	textures[1] = {};
		textures[0].imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		textures[0].imageView	= sparseImageView;
		textures[0].sampler		= sampler;

		VkWriteDescriptorSet	writes[1] = {};
		writes[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet			= descriptorSet;
		writes[0].dstBinding		= 0;
		writes[0].descriptorCount	= uint(CountOf( textures ));
		writes[0].descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo		= textures;

		vkUpdateDescriptorSets( vulkan.GetVkDevice(), uint(CountOf( writes )), writes, 0, null );
	}
	return true;
}

/*
=================================================
	CreatePipeline
=================================================
*/
bool SparseImageApp::CreatePipeline ()
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
layout(location = 0) out vec2  v_TexCoord;

void main()
{
	gl_Position = vec4( g_Positions[gl_VertexIndex], 0.0, 1.0 );
	v_TexCoord  = g_Positions[gl_VertexIndex] * 0.5 + 0.5;
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT vertShader, vulkan, {vert_shader_source}, "main", EShLangVertex ));
	}

	// create fragment shader
	{
		static const char	frag_shader_source[] = R"#(
layout(location = 0) out vec4  out_Color;
layout(location = 0) in  vec2  v_TexCoord;
layout(binding = 0) uniform sampler2D un_Texture;

void main ()
{
	out_Color = texture( un_Texture, v_TexCoord );
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT fragShader, vulkan, {frag_shader_source}, "main", EShLangFragment ));
	}

	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &dsLayout;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &pplnLayout ));
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

	VkDynamicState							dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
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
	info.layout					= pplnLayout;
	info.renderPass				= renderPass;
	info.subpass				= 0;

	VK_CHECK( vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &pipeline ));
	return true;
}
}	// anonymous namespace

/*
=================================================
	SparseImage_Sample1
=================================================
*/
extern void SparseImage_Sample1 ()
{
	SparseImageApp	app;
	
	if ( app.Initialize() )
	{
		app.Run();
		app.Destroy();
	}
}

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "compiler/SpvCompiler.h"
#include "stl/Math/Color.h"
#include "stl/CompileTime/Math.h"
#include "stl/Algorithms/StringUtils.h"

namespace {

class ClearImageApp final : public IWindowEventListener, public VulkanDeviceFn
{
	using Nanoseconds	= std::chrono::nanoseconds;
	
	static constexpr uint2		imageSize		= uint2{ 1u << 12 };
	static constexpr RGBA32f	clearColor		{ 1.0f, 1.0f, 1.0f, 1.0f };
	//static constexpr RGBA32f	clearColor		{ 0.123f, 0.456f, 0.789f, 0.158f };
	static constexpr bool		genInComputeShader		 = false;
	static constexpr bool		invalidateBeforeClearing = false;

private:
	VulkanDeviceExt			vulkan;
	VulkanSwapchainPtr		swapchain;
	WindowPtr				window;
	SpvCompiler				spvCompiler;

	bool					shaderSubgroupSupported	= false;
	
	VkQueue					cmdQueue				= VK_NULL_HANDLE;
	VkCommandPool			cmdPool					= VK_NULL_HANDLE;
	VkCommandBuffer			cmdBuffers[3]			= {};
	VkFence					fences[2]				= {};
	VkSemaphore				semaphores[2]			= {};
	VkQueryPool				queryPool				= VK_NULL_HANDLE;
	
	struct {
		VkDescriptorSetLayout	dsLayout			= VK_NULL_HANDLE;
		VkDescriptorSet			descSet				= VK_NULL_HANDLE;
		VkPipelineLayout		pipelineLayout		= VK_NULL_HANDLE;
		VkPipeline				pipeline			= VK_NULL_HANDLE;
		VkShaderModule			vertShader			= VK_NULL_HANDLE;
		VkShaderModule			fragShader			= VK_NULL_HANDLE;
	}						draw;

	struct {
		VkDescriptorSetLayout	dsLayout			= VK_NULL_HANDLE;
		VkDescriptorSet			descSet				= VK_NULL_HANDLE;
		VkPipelineLayout		pipelineLayout		= VK_NULL_HANDLE;
		VkPipeline				pipeline			= VK_NULL_HANDLE;
		VkShaderModule			shader				= VK_NULL_HANDLE;
		uint2					groupCount;
	}						clearCompute;
	
	struct {
		VkDescriptorSetLayout	dsLayout			= VK_NULL_HANDLE;
		VkDescriptorSet			descSet				= VK_NULL_HANDLE;
		VkPipelineLayout		pipelineLayout		= VK_NULL_HANDLE;
		VkPipeline				pipeline			= VK_NULL_HANDLE;
		VkShaderModule			shader				= VK_NULL_HANDLE;
		const uint2				localGroupSize		{ 16, 16 };
	}						proceduralImage;

	struct {
		VkPipelineLayout		pipelineLayout		= VK_NULL_HANDLE;
		VkPipeline				pipeline			= VK_NULL_HANDLE;
		VkShaderModule			vertShader			= VK_NULL_HANDLE;
		VkShaderModule			fragShader			= VK_NULL_HANDLE;
	}						proceduralImage2;

	VkDescriptorPool		descriptorPool			= VK_NULL_HANDLE;
	
	const VkFormat			imageFormat				= VK_FORMAT_R8G8B8A8_UNORM;
	VkFormat				colorFormat				= VK_FORMAT_R8G8B8A8_UNORM;
	VkRenderPass			renderPass				= VK_NULL_HANDLE;
	VkFramebuffer			framebuffers[2]			= {};

	VkImage					image					= VK_NULL_HANDLE;
	VkImageView				textureView				= VK_NULL_HANDLE;
	VkImageView				attachmentView			= VK_NULL_HANDLE;
	VkFramebuffer			imageFramebuffer		= VK_NULL_HANDLE;
	VkRenderPass			clearingRenderpass		= VK_NULL_HANDLE;
	VkRenderPass			proceduralRenderpass	= VK_NULL_HANDLE;
	VkSampler				sampler					= VK_NULL_HANDLE;
	VkDeviceMemory			sharedMemory			= VK_NULL_HANDLE;

	float2					imageScale				= { 1.0f, 1.0f };
	uint					imageClearMode			= UMax;
	bool					looping					= true;


public:
	ClearImageApp ()
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

	void ClearWithRenderPass (VkCommandBuffer cmd, VkPipelineStageFlags stage, INOUT VkImageMemoryBarrier &barrier);
	void ClearWithTransferOp (VkCommandBuffer cmd, VkPipelineStageFlags stage, INOUT VkImageMemoryBarrier &barrier);
	void ClearWithTransferOpOptimal (VkCommandBuffer cmd, VkPipelineStageFlags stage, INOUT VkImageMemoryBarrier &barrier);
	void ClearWithComputeShader (VkCommandBuffer cmd, VkPipelineStageFlags stage, INOUT VkImageMemoryBarrier &barrier);
	void ClearImage (uint mode);

	bool CreateCommandBuffers ();
	bool CreateQueryPool ();
	bool CreateSyncObjects ();
	bool CreateRenderPass ();
	bool CreateFramebuffers ();
	void DestroyFramebuffers ();
	bool CreateSampler ();
	bool CreateImage ();
	bool CreateDescriptorSet ();
	bool CreateGraphicsPipeline ();
	bool CreateProceduralImagePipeline ();
	bool CreateProceduralImagePipeline2 ();
	bool CreateImageClearingComputePipeline ();
};



/*
=================================================
	OnKey
=================================================
*/
void ClearImageApp::OnKey (StringView key, EKeyAction action)
{
	if ( action != EKeyAction::Down )
		return;

	if ( key == "escape" )
		looping = false;

	if ( key == "arrow right" )	imageScale.x = Min( imageScale.x + 0.1f, 10.0f );
	if ( key == "arrow left" )	imageScale.x = Max( imageScale.x - 0.1f,  0.1f );
	
	if ( key == "arrow up" )	imageScale.y = Min( imageScale.y + 0.1f, 10.0f );
	if ( key == "arrow down" )	imageScale.y = Max( imageScale.y - 0.1f,  0.1f );

	if ( key == "1" )	imageClearMode = 0;
	if ( key == "2" )	imageClearMode = 1;
	if ( key == "3" )	imageClearMode = 2;
	if ( key == "4" )	imageClearMode = 3;
}

/*
=================================================
	OnResize
=================================================
*/
void ClearImageApp::OnResize (const uint2 &size)
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
bool ClearImageApp::Initialize ()
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
		const char	title[] = "Mipmap generation sample";

		CHECK_ERR( window->Create( { 800, 600 }, title ));
		window->AddListener( this );

		CHECK_ERR( vulkan.Create( window->GetVulkanSurface(),
								  title, "Engine",
								  VK_API_VERSION_1_1,
								  "",
								  {},
								  VulkanDevice::GetRecomendedInstanceLayers(),
								  VulkanDevice::GetRecomendedInstanceExtensions(),
								  VulkanDevice::GetAllDeviceExtensions()
			));

		shaderSubgroupSupported = EnumEq( vulkan.GetDeviceSubgroupProperties().supportedStages, VK_SHADER_STAGE_COMPUTE_BIT );

		cmdQueue = vulkan.GetVkQueues()[0].handle;

		vulkan.CreateDebugUtilsCallback( VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT );
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
	CHECK_ERR( CreateQueryPool() );
	CHECK_ERR( CreateSyncObjects() );
	CHECK_ERR( CreateRenderPass() );
	CHECK_ERR( CreateFramebuffers() );
	CHECK_ERR( CreateSampler() );
	CHECK_ERR( CreateImage() );
	CHECK_ERR( CreateDescriptorSet() );
	CHECK_ERR( CreateGraphicsPipeline() );
	CHECK_ERR( CreateProceduralImagePipeline() );
	CHECK_ERR( CreateProceduralImagePipeline2() );
	CHECK_ERR( CreateImageClearingComputePipeline() );
	
	ClearImage( 0 );
	ClearImage( 1 );
	ClearImage( 2 );
	ClearImage( 3 );
	return true;
}

/*
=================================================
	Destroy
=================================================
*/
void ClearImageApp::Destroy ()
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

	vkDestroyFramebuffer( dev, imageFramebuffer, null );
	imageFramebuffer = VK_NULL_HANDLE;
	
	vkDestroyImage( dev, image, null );
	image = VK_NULL_HANDLE;
	
	vkDestroyImageView( dev, attachmentView, null );
	attachmentView = VK_NULL_HANDLE;
	
	vkDestroyImageView( dev, textureView, null );
	textureView = VK_NULL_HANDLE;
	
	vkDestroyDescriptorSetLayout( dev, draw.dsLayout, null );
	vkDestroyPipelineLayout( dev, draw.pipelineLayout, null );
	vkDestroyPipeline( dev, draw.pipeline, null );
	vkDestroyShaderModule( dev, draw.vertShader, null );
	vkDestroyShaderModule( dev, draw.fragShader, null );
	
	vkDestroyDescriptorSetLayout( dev, clearCompute.dsLayout, null );
	vkDestroyPipelineLayout( dev, clearCompute.pipelineLayout, null );
	vkDestroyPipeline( dev, clearCompute.pipeline, null );
	vkDestroyShaderModule( dev, clearCompute.shader, null );
	
	vkDestroyDescriptorSetLayout( dev, proceduralImage.dsLayout, null );
	vkDestroyPipelineLayout( dev, proceduralImage.pipelineLayout, null );
	vkDestroyPipeline( dev, proceduralImage.pipeline, null );
	vkDestroyShaderModule( dev, proceduralImage.shader, null );
	
	vkDestroyPipelineLayout( dev, proceduralImage2.pipelineLayout, null );
	vkDestroyPipeline( dev, proceduralImage2.pipeline, null );
	vkDestroyShaderModule( dev, proceduralImage2.vertShader, null );
	vkDestroyShaderModule( dev, proceduralImage2.fragShader, null );

	vkDestroyCommandPool( dev, cmdPool, null );
	vkDestroyRenderPass( dev, renderPass, null );
	vkDestroyRenderPass( dev, clearingRenderpass, null );
	vkDestroyRenderPass( dev, proceduralRenderpass, null );
	vkDestroyDescriptorPool( dev, descriptorPool, null );
	vkDestroySampler( dev, sampler, null );
	vkFreeMemory( dev, sharedMemory, null );
	vkDestroyQueryPool( dev, queryPool, null );

	cmdPool				= VK_NULL_HANDLE;
	descriptorPool		= VK_NULL_HANDLE;
	sharedMemory		= VK_NULL_HANDLE;
	sampler				= VK_NULL_HANDLE;
	queryPool			= VK_NULL_HANDLE;
	renderPass			= VK_NULL_HANDLE;
	clearingRenderpass	= VK_NULL_HANDLE;
	proceduralRenderpass = VK_NULL_HANDLE;

	swapchain->Destroy();
	swapchain.reset();

	vulkan.Destroy();

	window->Destroy();
	window.reset();
}

/*
=================================================
	ClearWithRenderPass
=================================================
*/
void ClearImageApp::ClearWithRenderPass (VkCommandBuffer cmd, VkPipelineStageFlags, INOUT VkImageMemoryBarrier &barrier)
{
	VkClearValue			clear_value = {{{ clearColor.r, clearColor.g, clearColor.b, clearColor.a }}};
	VkRenderPassBeginInfo	begin		= {};
	begin.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	begin.framebuffer		= imageFramebuffer;
	begin.renderPass		= clearingRenderpass;
	begin.renderArea		= { {0,0}, {imageSize.x, imageSize.y} };
	begin.clearValueCount	= 1;
	begin.pClearValues		= &clear_value;
	vkCmdBeginRenderPass( cmd, &begin, VK_SUBPASS_CONTENTS_INLINE );
	
	vkCmdEndRenderPass( cmd );

	barrier.dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.newLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

/*
=================================================
	ClearWithTransferOp
=================================================
*/
void ClearImageApp::ClearWithTransferOp (VkCommandBuffer cmd, VkPipelineStageFlags srcStage, INOUT VkImageMemoryBarrier &barrier)
{
	// write after write
	barrier.dstAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.newLayout		= VK_IMAGE_LAYOUT_GENERAL;
	vkCmdPipelineBarrier( cmd, srcStage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, null, 0, null, 1, &barrier );


	VkClearColorValue		clear_value = {{ clearColor.r, clearColor.g, clearColor.b, clearColor.a }};
	VkImageSubresourceRange	range		= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdClearColorImage( cmd, image, barrier.newLayout, &clear_value, 1, &range );
}

/*
=================================================
	ClearWithTransferOpOptimal
=================================================
*/
void ClearImageApp::ClearWithTransferOpOptimal (VkCommandBuffer cmd, VkPipelineStageFlags srcStage, INOUT VkImageMemoryBarrier &barrier)
{
	// .. -> transfer_dst
	barrier.dstAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.newLayout		= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	vkCmdPipelineBarrier( cmd, srcStage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, null, 0, null, 1, &barrier );


	VkClearColorValue		clear_value = {{ clearColor.r, clearColor.g, clearColor.b, clearColor.a }};
	VkImageSubresourceRange	range		= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdClearColorImage( cmd, image, barrier.newLayout, &clear_value, 1, &range );
}

/*
=================================================
	ClearWithComputeShader
=================================================
*/
void ClearImageApp::ClearWithComputeShader (VkCommandBuffer cmd, VkPipelineStageFlags srcStage, INOUT VkImageMemoryBarrier &barrier)
{
	// write after write
	barrier.dstAccessMask	= VK_ACCESS_SHADER_WRITE_BIT;
	barrier.newLayout		= VK_IMAGE_LAYOUT_GENERAL;
	vkCmdPipelineBarrier( cmd, srcStage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, null, 0, null, 1, &barrier );


	vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, clearCompute.pipeline );
	vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, clearCompute.pipelineLayout, 0, 1, &clearCompute.descSet, 0, null );
	vkCmdPushConstants( cmd, clearCompute.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(clearColor), clearColor.data() );

	vkCmdDispatch( cmd, clearCompute.groupCount.x, clearCompute.groupCount.y, 1 );
}

/*
=================================================
	ClearImage
=================================================
*/
void ClearImageApp::ClearImage (uint mode)
{
	VkCommandBuffer			cmd = cmdBuffers[2];
	String					name;
	VkPipelineStageFlags	last_stage = 0;

	VkImageMemoryBarrier	barrier = {};
	barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	barrier.image				= image;
	barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	// begin
	{
		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vkBeginCommandBuffer( cmd, &begin_info ));
	}

	// generate procedural image
	{
		// generate
		if ( genInComputeShader )
		{
			// shader_read_only -> general
			barrier.srcAccessMask	= 0;
			barrier.dstAccessMask	= VK_ACCESS_SHADER_WRITE_BIT;
			barrier.oldLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.newLayout		= VK_IMAGE_LAYOUT_GENERAL;

			vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
								  0, null, 0, null, 1, &barrier );

			uint2	count = (imageSize + proceduralImage.localGroupSize - 1) / proceduralImage.localGroupSize;

			vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, proceduralImage.pipeline );
			vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, proceduralImage.pipelineLayout, 0, 1, &proceduralImage.descSet, 0, null );
			vkCmdDispatch( cmd, count.x, count.y, 1 );

			barrier.oldLayout	= VK_IMAGE_LAYOUT_GENERAL;
			last_stage			= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}
		else
		{
			// begin render pass
			VkClearValue			clear_value = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
			VkRenderPassBeginInfo	begin		= {};
			begin.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			begin.framebuffer		= imageFramebuffer;
			begin.renderPass		= proceduralRenderpass;
			begin.renderArea		= { {0,0}, {imageSize.x, imageSize.y} };
			begin.clearValueCount	= 1;
			begin.pClearValues		= &clear_value;
			vkCmdBeginRenderPass( cmd, &begin, VK_SUBPASS_CONTENTS_INLINE );
			
			vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, proceduralImage2.pipeline );

			// set dynamic states
			VkViewport	viewport = {};
			viewport.x			= 0.0f;
			viewport.y			= 0.0f;
			viewport.width		= float(imageSize.x);
			viewport.height		= float(imageSize.y);
			viewport.minDepth	= 0.0f;
			viewport.maxDepth	= 1.0f;
			vkCmdSetViewport( cmd, 0, 1, &viewport );

			VkRect2D	scissor_rect = { {0,0}, {imageSize.x, imageSize.y} };
			vkCmdSetScissor( cmd, 0, 1, &scissor_rect );
			
			vkCmdDraw( cmd, 4, 1, 0, 0 );

			vkCmdEndRenderPass( cmd );
			
			barrier.oldLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			last_stage			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}

		barrier.oldLayout		= invalidateBeforeClearing ? VK_IMAGE_LAYOUT_UNDEFINED : barrier.oldLayout;
		barrier.srcAccessMask	= barrier.dstAccessMask;
	}

	vkCmdWriteTimestamp( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPool, 0 );
	
	// generate mipmaps
	switch ( mode )
	{
		case 0 :	ClearWithRenderPass( cmd, last_stage, INOUT barrier );			name = "using render pass";			break;
		case 1 :	ClearWithTransferOp( cmd, last_stage, INOUT barrier );			name = "using transfer op";			break;
		case 2 :	ClearWithTransferOpOptimal( cmd, last_stage, INOUT barrier );	name = "using optimal transfer op";	break;
		case 3 :	ClearWithComputeShader( cmd, last_stage, INOUT barrier );		name = "using compute shader";		break;
		default :	ASSERT(false);	break;
	}

	vkCmdWriteTimestamp( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPool, 1 );
	
	// read after write
	barrier.srcAccessMask	= barrier.dstAccessMask;
	barrier.dstAccessMask	= 0;
	barrier.oldLayout		= barrier.newLayout;
	barrier.newLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
							0, null, 0, null, 1, &barrier );

	VK_CALL( vkEndCommandBuffer( cmd ));
	
	// wait until GPU finished using 'image'
	VK_CALL( vkQueueWaitIdle( cmdQueue ));

	// submit and wait
	{
		VkSubmitInfo		submit_info = {};
		submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount	= 1;
		submit_info.pCommandBuffers		= &cmd;

		VK_CALL( vkQueueSubmit( cmdQueue, 1, &submit_info, VK_NULL_HANDLE ));
		VK_CALL( vkQueueWaitIdle( cmdQueue ));
	}

	uint64_t	results[2] = {};
	VK_CALL( vkGetQueryPoolResults( vulkan.GetVkDevice(), queryPool, 0, uint(CountOf(results)), sizeof(results), OUT results,
									sizeof(results[0]), VK_QUERY_RESULT_64_BIT ));

	FG_LOGI( "clear image " + name + ": " + ToString(Nanoseconds{results[1] - results[0]}) );
}

/*
=================================================
	Run
=================================================
*/
bool ClearImageApp::Run ()
{
	for (uint frameId = 0; looping; frameId = ((frameId + 1) & 1))
	{
		if ( not window->Update() )
			break;
		
		if ( imageClearMode != UMax )
		{
			ClearImage( imageClearMode );
			imageClearMode = UMax;
		}

		// wait and acquire next image
		{
			VK_CHECK( vkWaitForFences( vulkan.GetVkDevice(), 1, &fences[frameId], true, UMax ));
			VK_CHECK( vkResetFences( vulkan.GetVkDevice(), 1, &fences[frameId] ));

			VK_CALL( swapchain->AcquireNextImage( semaphores[0] ));
		}

		VkCommandBuffer	cmd = cmdBuffers[frameId];

		// build command buffer
		{
			VkCommandBufferBeginInfo	begin_info = {};
			begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			VK_CALL( vkBeginCommandBuffer( cmd, &begin_info ));
			
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

				vkCmdBeginRenderPass( cmd, &begin, VK_SUBPASS_CONTENTS_INLINE );
			}
			
			vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.pipeline );
			vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.pipelineLayout, 0, 1, &draw.descSet, 0, null );

			vkCmdPushConstants( cmd, draw.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float2), &imageScale );

			// set dynamic states
			{
				VkViewport	viewport = {};
				viewport.x			= 0.0f;
				viewport.y			= 0.0f;
				viewport.width		= float(swapchain->GetSurfaceSize().x);
				viewport.height		= float(swapchain->GetSurfaceSize().y);
				viewport.minDepth	= 0.0f;
				viewport.maxDepth	= 1.0f;

				vkCmdSetViewport( cmd, 0, 1, &viewport );

				VkRect2D	scissor_rect = { {0,0}, {swapchain->GetSurfaceSize().x, swapchain->GetSurfaceSize().y} };

				vkCmdSetScissor( cmd, 0, 1, &scissor_rect );
			}
			
			vkCmdDraw( cmd, 4, 1, 0, 0 );

			vkCmdEndRenderPass( cmd );

			VK_CALL( vkEndCommandBuffer( cmd ));
		}


		// submit commands
		{
			VkSemaphore				signal_semaphores[] = { semaphores[1] };
			VkSemaphore				wait_semaphores[]	= { semaphores[0] };
			VkPipelineStageFlags	wait_dst_mask[]		= { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
			STATIC_ASSERT( CountOf(wait_semaphores) == CountOf(wait_dst_mask) );

			VkSubmitInfo			submit_info = {};
			submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.commandBufferCount		= 1;
			submit_info.pCommandBuffers			= &cmd;
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
bool ClearImageApp::CreateCommandBuffers ()
{
	VkCommandPoolCreateInfo		pool_info = {};
	pool_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex	= vulkan.GetVkQueues()[0].familyIndex;
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
	CreateQueryPool
=================================================
*/
bool ClearImageApp::CreateQueryPool ()
{
	VkQueryPoolCreateInfo	info = {};
	info.sType		= VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	info.queryType	= VK_QUERY_TYPE_TIMESTAMP;
	info.queryCount	= 2;
	
	VK_CHECK( vkCreateQueryPool( vulkan.GetVkDevice(), &info, null, OUT &queryPool ));
	return true;
}

/*
=================================================
	CreateSyncObjects
=================================================
*/
bool ClearImageApp::CreateSyncObjects ()
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
bool ClearImageApp::CreateRenderPass ()
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


	// setup render pass for clearing
	attachments[0].format			= imageFormat;
	attachments[0].loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].initialLayout	= invalidateBeforeClearing ? VK_IMAGE_LAYOUT_UNDEFINED :
										(genInComputeShader ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	attachments[0].finalLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	dependencies[0].srcStageMask	= genInComputeShader ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask	= 0;
	dependencies[0].dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask	= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask	= 0;
	dependencies[1].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

	VK_CHECK( vkCreateRenderPass( vulkan.GetVkDevice(), &info, null, OUT &clearingRenderpass ));
	

	// setup render pass for image generation
	attachments[0].format			= imageFormat;
	attachments[0].loadOp			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	dependencies[0].srcStageMask	= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	dependencies[0].dstStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask	= 0;
	dependencies[0].dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask	= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask	= 0;
	dependencies[1].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

	VK_CHECK( vkCreateRenderPass( vulkan.GetVkDevice(), &info, null, OUT &proceduralRenderpass ));
	return true;
}

/*
=================================================
	CreateFramebuffers
=================================================
*/
bool ClearImageApp::CreateFramebuffers ()
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
void ClearImageApp::DestroyFramebuffers ()
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
bool ClearImageApp::CreateSampler ()
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
	CreateImage
=================================================
*/
bool ClearImageApp::CreateImage ()
{
	VkMemoryRequirements	mem_req;

	// create image
	{
		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.flags			= 0;
		info.imageType		= VK_IMAGE_TYPE_2D;
		info.format			= imageFormat;
		info.extent			= { imageSize.x, imageSize.y, 1 };
		info.mipLevels		= 1;
		info.arrayLayers	= 1;
		info.samples		= VK_SAMPLE_COUNT_1_BIT;
		info.tiling			= VK_IMAGE_TILING_OPTIMAL;
		info.usage			= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vkCreateImage( vulkan.GetVkDevice(), &info, null, OUT &image ));
		
		vkGetImageMemoryRequirements( vulkan.GetVkDevice(), image, OUT &mem_req );
	}

	// allocate memory
	{
		VkMemoryAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize		= mem_req.size;

		CHECK_ERR( vulkan.GetMemoryTypeIndex( mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, OUT info.memoryTypeIndex ));

		VK_CHECK( vkAllocateMemory( vulkan.GetVkDevice(), &info, null, OUT &sharedMemory ));
	}

	// bind resources
	VK_CALL( vkBindImageMemory( vulkan.GetVkDevice(), image, sharedMemory, 0 ));
	
	// create image view for clearing
	{
		VkImageViewCreateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image				= image;
		info.viewType			= VK_IMAGE_VIEW_TYPE_2D;
		info.format				= imageFormat;
		info.components			= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		info.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		VK_CALL( vkCreateImageView( vulkan.GetVkDevice(), &info, null, OUT &attachmentView ));
		
		VkFramebufferCreateInfo	fb_info = {};
		fb_info.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass		= clearingRenderpass;
		fb_info.attachmentCount	= 1;
		fb_info.width			= Max( imageSize.x, 1u );
		fb_info.height			= Max( imageSize.y, 1u );
		fb_info.layers			= 1;
		fb_info.pAttachments	= &attachmentView;
		
		VK_CALL( vkCreateFramebuffer( vulkan.GetVkDevice(), &fb_info, null, OUT &imageFramebuffer ));
	}
	
	// create image view for sampling
	{
		VkImageViewCreateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image				= image;
		info.viewType			= VK_IMAGE_VIEW_TYPE_2D;
		info.format				= imageFormat;
		info.components			= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		info.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		VK_CALL( vkCreateImageView( vulkan.GetVkDevice(), &info, null, OUT &textureView ));
	}

	// update resources
	{
		VkCommandBuffer		cmd = cmdBuffers[2];

		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vkBeginCommandBuffer( cmd, &begin_info ));

		// undefined -> shader_read_only
		{
			VkImageMemoryBarrier	barrier = {};
			barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image				= image;
			barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask		= 0;
			barrier.dstAccessMask		= 0;
			barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}

		VK_CALL( vkEndCommandBuffer( cmd ));

		VkSubmitInfo		submit_info = {};
		submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount	= 1;
		submit_info.pCommandBuffers		= &cmd;

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
bool ClearImageApp::CreateDescriptorSet ()
{
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
	return true;
}

/*
=================================================
	CreateGraphicsPipeline
=================================================
*/
bool ClearImageApp::CreateGraphicsPipeline ()
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

layout(push_constant, std140) uniform PushConst {
	vec2	scale;
} pc;

layout(location=0) out vec2  v_Texcoord;

void main()
{
	gl_Position = vec4( g_Positions[gl_VertexIndex] * pc.scale, 0.0, 1.0 );
	v_Texcoord  = g_Positions[gl_VertexIndex] * 0.5f + 0.5f;
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT draw.vertShader, vulkan, {vert_shader_source}, "main", EShLangVertex ));
	}

	// create fragment shader
	{
		static const char	frag_shader_source[] = R"#(
layout(location = 0) out vec4  out_Color;

layout(binding=0) uniform sampler2D  un_Texture;
layout(location=0) in vec2  v_Texcoord;

void main ()
{
	out_Color = texture( un_Texture, v_Texcoord );
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT draw.fragShader, vulkan, {frag_shader_source}, "main", EShLangFragment ));
	}
	
	// create descriptor set layout
	{
		VkDescriptorSetLayoutBinding	binding[1] = {};
		binding[0].binding			= 0;
		binding[0].descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding[0].descriptorCount	= 1;
		binding[0].stageFlags		= VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= uint(CountOf( binding ));
		info.pBindings		= binding;

		VK_CHECK( vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &draw.dsLayout ));
	}

	// create pipeline layout
	{
		VkPushConstantRange		range = {};
		range.offset		= 0;
		range.size			= sizeof(float2);
		range.stageFlags	= VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &draw.dsLayout;
		info.pushConstantRangeCount	= 1;
		info.pPushConstantRanges	= &range;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &draw.pipelineLayout ));
	}

	VkPipelineShaderStageCreateInfo			stages[2] = {};
	stages[0].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage		= VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module	= draw.vertShader;
	stages[0].pName		= "main";
	stages[1].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage		= VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module	= draw.fragShader;
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
	info.layout					= draw.pipelineLayout;
	info.renderPass				= renderPass;
	info.subpass				= 0;

	VK_CHECK( vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &draw.pipeline ));
	
	// allocate descriptor set
	{
		VkDescriptorSetAllocateInfo		ds_info = {};
		ds_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		ds_info.descriptorPool		= descriptorPool;
		ds_info.descriptorSetCount	= 1;
		ds_info.pSetLayouts			= &draw.dsLayout;

		VK_CHECK( vkAllocateDescriptorSets( vulkan.GetVkDevice(), &ds_info, OUT &draw.descSet ));
	}

	// update descriptor set
	{
		VkDescriptorImageInfo	images[1] = {};
		images[0].imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		images[0].imageView		= textureView;
		images[0].sampler		= sampler;

		VkWriteDescriptorSet	writes[1] = {};
		writes[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet			= draw.descSet;
		writes[0].dstBinding		= 0;
		writes[0].descriptorCount	= 1;
		writes[0].descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo		= &images[0];

		vkUpdateDescriptorSets( vulkan.GetVkDevice(), uint(CountOf(writes)), writes, 0, null );
	}
	return true;
}

/*
=================================================
	CreateProceduralImagePipeline
=================================================
*/
bool ClearImageApp::CreateProceduralImagePipeline ()
{
	// create compute shader
	{
		const String	comp_shader_source =
"layout (local_size_x = "s << ToString(proceduralImage.localGroupSize.x) << ", local_size_y = " << ToString(proceduralImage.localGroupSize.y) << ", local_size_z = 1) in;\n"
R"#(
layout(binding = 0) writeonly uniform image2D	un_outImage;

void main()
{
	vec4	color = all(equal( gl_WorkGroupID.xy & 1, uvec2(1) )) ?
					vec4(1.0f, 0.0f, 0.0f, 1.0f) :
					vec4(0.0f, 1.0f, 0.0f, 1.0f);

	imageStore( un_outImage, ivec2(gl_GlobalInvocationID.xy), color );
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT proceduralImage.shader, vulkan, {comp_shader_source.data()}, "main", EShLangCompute ));
	}
	
	// create descriptor set layout
	{
		VkDescriptorSetLayoutBinding	binding[1] = {};
		binding[0].binding			= 0;
		binding[0].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding[0].descriptorCount	= 1;
		binding[0].stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= uint(CountOf( binding ));
		info.pBindings		= binding;

		VK_CHECK( vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &proceduralImage.dsLayout ));
	}

	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &proceduralImage.dsLayout;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &proceduralImage.pipelineLayout ));
	}

	VkComputePipelineCreateInfo		info = {};
	info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
	info.stage.module	= proceduralImage.shader;
	info.stage.pName	= "main";
	info.layout			= proceduralImage.pipelineLayout;

	VK_CHECK( vkCreateComputePipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &proceduralImage.pipeline ));
	
	// allocate descriptor set
	{
		VkDescriptorSetAllocateInfo		ds_info = {};
		ds_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		ds_info.descriptorPool		= descriptorPool;
		ds_info.descriptorSetCount	= 1;
		ds_info.pSetLayouts			= &proceduralImage.dsLayout;

		VK_CHECK( vkAllocateDescriptorSets( vulkan.GetVkDevice(), &ds_info, OUT &proceduralImage.descSet ));
	}

	// update descriptor set
	{
		VkDescriptorImageInfo	images[1] = {};
		images[0].imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		images[0].imageView		= attachmentView;

		VkWriteDescriptorSet	writes[1] = {};
		writes[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet			= proceduralImage.descSet;
		writes[0].dstBinding		= 0;
		writes[0].descriptorCount	= 1;
		writes[0].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[0].pImageInfo		= &images[0];

		vkUpdateDescriptorSets( vulkan.GetVkDevice(), uint(CountOf(writes)), writes, 0, null );
	}
	return true;
}

/*
=================================================
	CreateProceduralImagePipeline2
=================================================
*/
bool ClearImageApp::CreateProceduralImagePipeline2 ()
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

layout(location=0) out vec2  v_Texcoord;

void main()
{
	gl_Position = vec4( g_Positions[gl_VertexIndex], 0.0, 1.0 );
	v_Texcoord  = g_Positions[gl_VertexIndex] * 0.5f + 0.5f;
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT proceduralImage2.vertShader, vulkan, {vert_shader_source}, "main", EShLangVertex ));
	}

	// create fragment shader
	{
		static const char	frag_shader_source[] = R"#(
layout(location = 0) out vec4  out_Color;

layout(location=0) in vec2  v_Texcoord;

void main ()
{
	uvec2	group = uvec2(gl_FragCoord.xy / 16);
	out_Color     = all(equal( group & 1, uvec2(1) )) ?
					vec4(1.0f, 0.0f, 0.0f, 1.0f) :
					vec4(0.0f, 1.0f, 0.0f, 1.0f);
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT proceduralImage2.fragShader, vulkan, {frag_shader_source}, "main", EShLangFragment ));
	}

	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &proceduralImage2.pipelineLayout ));
	}

	VkPipelineShaderStageCreateInfo			stages[2] = {};
	stages[0].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage		= VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module	= proceduralImage2.vertShader;
	stages[0].pName		= "main";
	stages[1].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage		= VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module	= proceduralImage2.fragShader;
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
	info.layout					= proceduralImage2.pipelineLayout;
	info.renderPass				= proceduralRenderpass;
	info.subpass				= 0;

	VK_CHECK( vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &proceduralImage2.pipeline ));
	return true;
}

/*
=================================================
	CreateImageClearingComputePipeline
=================================================
*/
bool ClearImageApp::CreateImageClearingComputePipeline ()
{
	// find the maximum group size
	uint2	local_size;
	{
		const uint	max_invocations = vulkan.GetDeviceProperties().limits.maxComputeWorkGroupInvocations;
		uint2		size {2};

		for (; (size.x * size.y <= max_invocations); size = size << 1) {
			local_size = size;
		}
	}

	// create compute shader
	{
		String	comp_shader_source;
		comp_shader_source <<
"layout (local_size_x = " << ToString(local_size.x) << ", local_size_y = " << ToString(local_size.y) << ", local_size_z = 1) in;\n" R"#(
layout(binding = 0) writeonly uniform image2D  un_OutImage;

layout(push_constant, std140) uniform PushConst {
	vec4	color;
};

void main()
{
	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), color );
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT clearCompute.shader, vulkan, {comp_shader_source.data()}, "main", EShLangCompute ));
	}
	
	// create descriptor set layout
	{
		VkDescriptorSetLayoutBinding	binding = {};

		// un_OutImage
		binding.binding			= 0;
		binding.descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding.descriptorCount	= 1;
		binding.stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= 1;
		info.pBindings		= &binding;

		VK_CHECK( vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &clearCompute.dsLayout ));
	}

	// create pipeline layout
	{
		VkPushConstantRange			range = {};
		range.offset		= 0;
		range.size			= sizeof(clearColor);
		range.stageFlags	= VK_SHADER_STAGE_COMPUTE_BIT;

		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &clearCompute.dsLayout;
		info.pushConstantRangeCount	= 1;
		info.pPushConstantRanges	= &range;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &clearCompute.pipelineLayout ));
	}

	VkComputePipelineCreateInfo		info = {};
	info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
	info.stage.module	= clearCompute.shader;
	info.stage.pName	= "main";
	info.layout			= clearCompute.pipelineLayout;

	VK_CHECK( vkCreateComputePipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &clearCompute.pipeline ));
	
	// allocate and update descriptor set
	{
		VkDescriptorSetAllocateInfo		ds_info = {};
		ds_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		ds_info.descriptorPool		= descriptorPool;
		ds_info.descriptorSetCount	= 1;
		ds_info.pSetLayouts			= &clearCompute.dsLayout;
		VK_CHECK( vkAllocateDescriptorSets( vulkan.GetVkDevice(), &ds_info, OUT &clearCompute.descSet ));

		clearCompute.groupCount = Max( (imageSize + local_size-1) / local_size, 1u );

		VkDescriptorImageInfo	img_info = {};
		img_info.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		img_info.imageView		= attachmentView;

		VkWriteDescriptorSet	write = {};
		write.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet			= clearCompute.descSet;
		write.dstBinding		= 0;
		write.descriptorCount	= 1;
		write.descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		write.pImageInfo		= &img_info;

		vkUpdateDescriptorSets( vulkan.GetVkDevice(), 1, &write, 0, null );
	}
	return true;
}
}	// anonymous namespace

/*
=================================================
	ClearImage_Sample
=================================================
*/
extern void ClearImage_Sample ()
{
	ClearImageApp	app;
	
	if ( app.Initialize() )
	{
		app.Run();
		app.Destroy();
	}
}

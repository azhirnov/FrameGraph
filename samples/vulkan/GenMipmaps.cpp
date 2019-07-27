// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	references:
	https://gpuopen.com/optimize-engine-using-compute-4c-prague-2018/
	http://developer.download.nvidia.com/compute/cuda/1_1/Website/projects/reduction/doc/reduction.pdf

	docs:
	https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_memory_scope_semantics.txt
	https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_shader_subgroup.txt

	delta color compression:
	https://gpuopen.com/dcc-overview/
	https://github.com/philiptaylor/vulkan-sync/blob/master/memory.md#framebuffer-compression
*/

#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "compiler/SpvCompiler.h"
#include "stl/Math/Color.h"
#include "stl/CompileTime/Math.h"
#include "stl/Algorithms/StringUtils.h"

namespace {

class GenMipmapsApp final : public IWindowEventListener, public VulkanDeviceFn
{
	using Nanoseconds	= std::chrono::nanoseconds;
	
	static constexpr uint2	imageSize	= uint2{ 1u << 12 };
	static constexpr uint	numMipmaps	= CT_IntLog2< Max( imageSize.x, imageSize.y )>;
	static constexpr bool	genInComputeShader = true;

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
		VkDescriptorSet			descSet				= {};
		VkPipelineLayout		pipelineLayout		= VK_NULL_HANDLE;
		VkPipeline				pipeline			= VK_NULL_HANDLE;
		VkShaderModule			vertShader			= VK_NULL_HANDLE;
		VkShaderModule			fragShader			= VK_NULL_HANDLE;
	}						draw;

	struct {
		VkDescriptorSetLayout	dsLayout			= VK_NULL_HANDLE;
		VkDescriptorSet			descSet[numMipmaps]	= {};
		VkPipelineLayout		pipelineLayout		= VK_NULL_HANDLE;
		VkPipeline				pipeline			= VK_NULL_HANDLE;
		VkShaderModule			vertShader			= VK_NULL_HANDLE;
		VkShaderModule			fragShader			= VK_NULL_HANDLE;
	}						genGraphics;

	struct {
		VkDescriptorSetLayout	dsLayout			= VK_NULL_HANDLE;
		VkDescriptorSet			descSet[8]			= {};
		uint2					groupCount[8]		= {};
		uint2					mipmapRanges[8]		= {};
		VkPipelineLayout		pipelineLayout		= VK_NULL_HANDLE;
		VkPipeline				pipeline			= VK_NULL_HANDLE;
		VkShaderModule			shader				= VK_NULL_HANDLE;
	}						genCompute;

	struct {
		VkDescriptorSetLayout	dsLayout			= VK_NULL_HANDLE;
		VkDescriptorSet			descSet[8]			= {};
		uint2					groupCount[8]		= {};
		uint2					mipmapRanges[8]		= {};
		VkPipelineLayout		pipelineLayout		= VK_NULL_HANDLE;
		VkPipeline				pipeline			= VK_NULL_HANDLE;
		VkShaderModule			shader				= VK_NULL_HANDLE;
		uint2					subGroupSize		{ 4, 8 };
	}						genSM6;
	
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
	VkImageView				mipmapViews[numMipmaps]	= {};
	VkFramebuffer			mipmapFramebuffers[numMipmaps]	= {};
	VkRenderPass			mipmapRenderpass		= VK_NULL_HANDLE;
	VkSampler				sampler					= VK_NULL_HANDLE;
	VkDeviceMemory			sharedMemory			= VK_NULL_HANDLE;

	float2					imageScale				= { 1.0f, 1.0f };
	uint					mipmapGenMode			= UMax;
	bool					looping					= true;


public:
	GenMipmapsApp ()
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

	void GenMipmapsOnGraphicsShader (VkCommandBuffer cmd, OUT VkImageLayout &layout);
	void GenMipmapsOnTransferCommands (VkCommandBuffer cmd, OUT VkImageLayout &layout);
	void GenMipmapsOnTransferCommands2 (VkCommandBuffer cmd, OUT VkImageLayout &layout);
	void GenMipmapsOnComputeShader (VkCommandBuffer cmd, OUT VkImageLayout &layout);
	void GenMipmapsOnComputeShaderSM6 (VkCommandBuffer cmd, OUT VkImageLayout &layout);
	void GenerateMipmaps (uint mode);

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
	bool CreateDownscaleGraphicsPipeline ();
	bool CreateDownscaleComputePipeline ();
	bool CreateDownscaleComputePipelineSM6 ();
};



/*
=================================================
	OnKey
=================================================
*/
void GenMipmapsApp::OnKey (StringView key, EKeyAction action)
{
	if ( action != EKeyAction::Down )
		return;

	if ( key == "escape" )
		looping = false;

	if ( key == "arrow right" )	imageScale.x = Min( imageScale.x + 0.1f, 10.0f );
	if ( key == "arrow left" )	imageScale.x = Max( imageScale.x - 0.1f,  0.1f );
	
	if ( key == "arrow up" )	imageScale.y = Min( imageScale.y + 0.1f, 10.0f );
	if ( key == "arrow down" )	imageScale.y = Max( imageScale.y - 0.1f,  0.1f );

	if ( key == "1" )	mipmapGenMode = 0;
	if ( key == "2" )	mipmapGenMode = 1;
	if ( key == "3" )	mipmapGenMode = 2;
	if ( key == "4" )	mipmapGenMode = 3;
}

/*
=================================================
	OnResize
=================================================
*/
void GenMipmapsApp::OnResize (const uint2 &size)
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
bool GenMipmapsApp::Initialize ()
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
	CHECK_ERR( CreateDownscaleGraphicsPipeline() );
	CHECK_ERR( CreateDownscaleComputePipeline() );
	//CHECK_ERR( CreateDownscaleComputePipelineSM6() );
	
	GenerateMipmaps( 0 );
	GenerateMipmaps( 1 );
	GenerateMipmaps( 2 );
	GenerateMipmaps( 3 );
	return true;
}

/*
=================================================
	Destroy
=================================================
*/
void GenMipmapsApp::Destroy ()
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
	for (auto& fb : mipmapFramebuffers) {
		vkDestroyFramebuffer( dev, fb, null );
		fb = VK_NULL_HANDLE;
	}
	
	vkDestroyImage( dev, image, null );
	image = VK_NULL_HANDLE;
	
	for (auto& view : mipmapViews) {
		vkDestroyImageView( dev, view, null );
		view = VK_NULL_HANDLE;
	}
	vkDestroyImageView( dev, textureView, null );
	textureView = VK_NULL_HANDLE;
	
	vkDestroyDescriptorSetLayout( dev, draw.dsLayout, null );
	vkDestroyPipelineLayout( dev, draw.pipelineLayout, null );
	vkDestroyPipeline( dev, draw.pipeline, null );
	vkDestroyShaderModule( dev, draw.vertShader, null );
	vkDestroyShaderModule( dev, draw.fragShader, null );

	vkDestroyDescriptorSetLayout( dev, genGraphics.dsLayout, null );
	vkDestroyPipelineLayout( dev, genGraphics.pipelineLayout, null );
	vkDestroyPipeline( dev, genGraphics.pipeline, null );
	vkDestroyShaderModule( dev, genGraphics.vertShader, null );
	vkDestroyShaderModule( dev, genGraphics.fragShader, null );
	
	vkDestroyDescriptorSetLayout( dev, genCompute.dsLayout, null );
	vkDestroyPipelineLayout( dev, genCompute.pipelineLayout, null );
	vkDestroyPipeline( dev, genCompute.pipeline, null );
	vkDestroyShaderModule( dev, genCompute.shader, null );
	
	vkDestroyDescriptorSetLayout( dev, genSM6.dsLayout, null );
	vkDestroyPipelineLayout( dev, genSM6.pipelineLayout, null );
	vkDestroyPipeline( dev, genSM6.pipeline, null );
	vkDestroyShaderModule( dev, genSM6.shader, null );
	
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
	vkDestroyRenderPass( dev, mipmapRenderpass, null );
	vkDestroyDescriptorPool( dev, descriptorPool, null );
	vkDestroySampler( dev, sampler, null );
	vkFreeMemory( dev, sharedMemory, null );
	vkDestroyQueryPool( dev, queryPool, null );

	cmdPool			= VK_NULL_HANDLE;
	descriptorPool	= VK_NULL_HANDLE;
	sharedMemory	= VK_NULL_HANDLE;
	sampler			= VK_NULL_HANDLE;
	queryPool		= VK_NULL_HANDLE;
	renderPass		= VK_NULL_HANDLE;
	mipmapRenderpass= VK_NULL_HANDLE;

	swapchain->Destroy();
	swapchain.reset();

	vulkan.Destroy();

	window->Destroy();
	window.reset();
}

/*
=================================================
	GenMipmapsOnGraphicsShader
=================================================
*/
void GenMipmapsApp::GenMipmapsOnGraphicsShader (VkCommandBuffer cmd, OUT VkImageLayout &layout)
{
	for (size_t i = 1; i < numMipmaps; ++i)
	{
		const uint2	size = Max( imageSize >> i, 1u );

		// begin render pass
		{
			VkRenderPassBeginInfo	begin = {};
			begin.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			begin.framebuffer	= mipmapFramebuffers[i];
			begin.renderPass	= mipmapRenderpass;
			begin.renderArea	= { {0,0}, {size.x, size.y} };
			vkCmdBeginRenderPass( cmd, &begin, VK_SUBPASS_CONTENTS_INLINE );
		}
			
		vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, genGraphics.pipeline );
		vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, genGraphics.pipelineLayout, 0, 1, &genGraphics.descSet[i-1], 0, null );

		// set dynamic states
		{
			VkRect2D	scissor_rect = { {0,0}, {size.x, size.y} };
			vkCmdSetScissor( cmd, 0, 1, &scissor_rect );

			VkViewport	viewport = {};
			viewport.x			= 0.0f;
			viewport.y			= 0.0f;
			viewport.width		= float(size.x);
			viewport.height		= float(size.y);
			viewport.minDepth	= 0.0f;
			viewport.maxDepth	= 1.0f;
			vkCmdSetViewport( cmd, 0, 1, &viewport );
		}
		
		vkCmdDraw( cmd, 4, 1, 0, 0 );

		vkCmdEndRenderPass( cmd );
	}

	layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

/*
=================================================
	GenMipmapsOnTransferCommands
=================================================
*/
void GenMipmapsApp::GenMipmapsOnTransferCommands (VkCommandBuffer cmd, OUT VkImageLayout &layout)
{
	VkImageMemoryBarrier	barrier = {};
	barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask		= 0;
	barrier.dstAccessMask		= VK_ACCESS_TRANSFER_READ_BIT;
	barrier.oldLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout			= VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	barrier.image				= image;
	barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	
	vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						  0, null, 0, null, 1, &barrier );
	
	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;

	for (size_t i = 1; i < numMipmaps; ++i)
	{
		int2	src_size	= Max( uint2(1), imageSize >> (i-1) );
		int2	dst_size	= Max( uint2(1), imageSize >> i );

		VkImageBlit		region	= {};
		region.srcOffsets[0]	= { 0, 0, 0 };
		region.srcOffsets[1]	= { src_size.x, src_size.y, 1 };
		region.srcSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, uint(i-1), 0, 1 };
		region.dstOffsets[0]	= { 0, 0, 0 };
		region.dstOffsets[1]	= { dst_size.x, dst_size.y, 1 };
		region.dstSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, uint(i), 0, 1 };

		vkCmdBlitImage( cmd, image, VK_IMAGE_LAYOUT_GENERAL, image, VK_IMAGE_LAYOUT_GENERAL, 1, &region, VK_FILTER_LINEAR );

		// read after write
		barrier.srcAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask		= VK_ACCESS_TRANSFER_READ_BIT;
		barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, uint(i), 1, 0, 1 };

		vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							  0, null, 0, null, 1, &barrier );
	}

	layout = VK_IMAGE_LAYOUT_GENERAL;
}

/*
=================================================
	GenMipmapsOnTransferCommands2
=================================================
*/
void GenMipmapsApp::GenMipmapsOnTransferCommands2 (VkCommandBuffer cmd, OUT VkImageLayout &layout)
{
	VkImageMemoryBarrier	barrier = {};
	barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask		= 0;
	barrier.dstAccessMask		= VK_ACCESS_TRANSFER_READ_BIT;
	barrier.oldLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	barrier.image				= image;
	barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						  0, null, 0, null, 1, &barrier );

	for (size_t i = 1; i < numMipmaps; ++i)
	{
		int2	src_size	= Max( uint2(1), imageSize >> (i-1) );
		int2	dst_size	= Max( uint2(1), imageSize >> i );
		
		// undefined -> transfer_dst_optimal
		barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask		= 0;
		barrier.dstAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, uint(i), 1, 0, 1 };

		vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							  0, null, 0, null, 1, &barrier );

		VkImageBlit		region	= {};
		region.srcOffsets[0]	= { 0, 0, 0 };
		region.srcOffsets[1]	= { src_size.x, src_size.y, 1 };
		region.srcSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, uint(i-1), 0, 1 };
		region.dstOffsets[0]	= { 0, 0, 0 };
		region.dstOffsets[1]	= { dst_size.x, dst_size.y, 1 };
		region.dstSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, uint(i), 0, 1 };

		vkCmdBlitImage( cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR );

		// read after write
		barrier.oldLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask		= VK_ACCESS_TRANSFER_READ_BIT;
		barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, uint(i), 1, 0, 1 };

		vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							  0, null, 0, null, 1, &barrier );
	}

	layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
}

/*
=================================================
	GenMipmapsOnComputeShader
=================================================
*/
void GenMipmapsApp::GenMipmapsOnComputeShader (VkCommandBuffer cmd, OUT VkImageLayout &layout)
{
	VkImageMemoryBarrier	barrier = {};
	barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask		= 0;
	barrier.dstAccessMask		= VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout			= VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	barrier.image				= image;
	barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	
	vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
						  0, null, 0, null, 1, &barrier );

	vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, genCompute.pipeline );

	for (size_t i = 0; i < CountOf(genCompute.descSet); ++i)
	{
		if ( not genCompute.descSet[i] )
			break;

		const uint2  count = genCompute.groupCount[i];
		const uint2  range = genCompute.mipmapRanges[i];

		vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, genCompute.pipelineLayout, 0, 1, &genCompute.descSet[i], 0, null );
		vkCmdDispatch( cmd, count.x, count.y, 1 );
		
		// read after write
		barrier.oldLayout			= VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcAccessMask		= VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask		= VK_ACCESS_SHADER_READ_BIT;
		barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, range[0], range[1], 0, 1 };

		vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
							  0, null, 0, null, 1, &barrier );
	}

	layout = VK_IMAGE_LAYOUT_GENERAL;
}

/*
=================================================
	GenMipmapsOnComputeShaderSM6
=================================================
*/
void GenMipmapsApp::GenMipmapsOnComputeShaderSM6 (VkCommandBuffer cmd, OUT VkImageLayout &)
{
	VkImageMemoryBarrier	barrier = {};
	barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask		= 0;
	barrier.dstAccessMask		= 0;
	barrier.oldLayout			= VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout			= VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
	barrier.image				= image;

	vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, genSM6.pipeline );

	for (size_t i = 0; i < CountOf(genSM6.descSet); ++i)
	{
		if ( not genSM6.descSet[i] )
			break;

		const uint2  count = genSM6.groupCount[i];
		const uint2  range = genSM6.mipmapRanges[i];

		vkCmdBindDescriptorSets( cmd, VK_PIPELINE_BIND_POINT_COMPUTE, genSM6.pipelineLayout, 0, 1, &genSM6.descSet[i], 0, null );
		vkCmdDispatch( cmd, count.x, count.y, 1 );
		
		// read after write
		barrier.srcAccessMask		= VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask		= VK_ACCESS_SHADER_READ_BIT;
		barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, range[0], range[1], 0, 1 };

		vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
							  0, null, 0, null, 1, &barrier );
	}
}

/*
=================================================
	GenerateMipmaps
=================================================
*/
void GenMipmapsApp::GenerateMipmaps (uint mode)
{
	VkCommandBuffer		cmd = cmdBuffers[2];
	String				name;

	// begin
	{
		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vkBeginCommandBuffer( cmd, &begin_info ));
	}

	// generate procedural image
	{
		VkImageMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.image				= image;
		barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

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
			
			// general -> shader read only
			barrier.srcAccessMask	= VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask	= 0;
			barrier.oldLayout		= VK_IMAGE_LAYOUT_GENERAL;
			barrier.newLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
								  0, null, 0, null, 1, &barrier );
		}
		else
		{
			// begin render pass
			VkClearValue			clear_value = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
			VkRenderPassBeginInfo	begin		= {};
			begin.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			begin.framebuffer		= mipmapFramebuffers[0];
			begin.renderPass		= mipmapRenderpass;
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
		}

		// clear mipmaps
		{
			barrier.srcAccessMask	= 0;
			barrier.dstAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.oldLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.newLayout		= VK_IMAGE_LAYOUT_GENERAL;
			barrier.subresourceRange= { VK_IMAGE_ASPECT_COLOR_BIT, 1, numMipmaps-1, 0, 1 };

			vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
								  0, null, 0, null, 1, &barrier );

			VkImageSubresourceRange	range {};
			range.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel		= 1;
			range.levelCount		= numMipmaps-1;
			range.baseArrayLayer	= 0;
			range.layerCount		= 1;
			
			VkClearColorValue		color {};
			color.float32[0]	= 0.0f;
			color.float32[1]	= 0.0f;
			color.float32[2]	= 1.0f;
			color.float32[3]	= 1.0f;

			vkCmdClearColorImage( cmd, image, VK_IMAGE_LAYOUT_GENERAL, &color, 1, &range );
		}
		
		// flush after write
		barrier.srcAccessMask	= VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask	= 0;
		barrier.oldLayout		= VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout		= VK_IMAGE_LAYOUT_GENERAL;
		barrier.subresourceRange= { VK_IMAGE_ASPECT_COLOR_BIT, 1, numMipmaps-1, 0, 1 };

		vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
							  0, null, 0, null, 1, &barrier );
	}

	vkCmdWriteTimestamp( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPool, 0 );

	VkImageLayout	curr_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	// generate mipmaps
	switch ( mode )
	{
		case 0 :	GenMipmapsOnGraphicsShader( cmd, OUT curr_layout );		name = "using render pass";			break;
		case 1 :	GenMipmapsOnTransferCommands( cmd, OUT curr_layout );	name = "using image blit";			break;
		case 2 :	GenMipmapsOnTransferCommands2( cmd, OUT curr_layout );	name = "using optimal image blit";	break;
		case 3 :	GenMipmapsOnComputeShader( cmd, OUT curr_layout );		name = "using compute shader";		break;
		//case 4 :	GenMipmapsOnComputeShaderSM6( cmd, OUT curr_layout );	name = "using subgroups";			break;
		default :	ASSERT(false);	break;
	}

	vkCmdWriteTimestamp( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPool, 1 );

	// general -> shader_read_only
	{
		VkImageMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask		= 0;
		barrier.dstAccessMask		= VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout			= curr_layout;
		barrier.newLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.image				= image;
		barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, numMipmaps, 0, 1 };

		vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
							  0, null, 0, null, 1, &barrier );
	}

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

	FG_LOGI( "mipmap generation " + name + ": " + ToString(Nanoseconds{results[1] - results[0]}) );
}

/*
=================================================
	Run
=================================================
*/
bool GenMipmapsApp::Run ()
{
	for (uint frameId = 0; looping; frameId = ((frameId + 1) & 1))
	{
		if ( not window->Update() )
			break;
		
		if ( mipmapGenMode != UMax )
		{
			GenerateMipmaps( mipmapGenMode );
			mipmapGenMode = UMax;
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
bool GenMipmapsApp::CreateCommandBuffers ()
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
bool GenMipmapsApp::CreateQueryPool ()
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
bool GenMipmapsApp::CreateSyncObjects ()
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
bool GenMipmapsApp::CreateRenderPass ()
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


	// setup second render pass
	attachments[0].format			= imageFormat;
	attachments[0].loadOp			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	dependencies[0].srcStageMask	= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	dependencies[0].dstStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask	= 0;
	dependencies[0].dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask	= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask	= VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;

	VK_CHECK( vkCreateRenderPass( vulkan.GetVkDevice(), &info, null, OUT &mipmapRenderpass ));
	return true;
}

/*
=================================================
	CreateFramebuffers
=================================================
*/
bool GenMipmapsApp::CreateFramebuffers ()
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
void GenMipmapsApp::DestroyFramebuffers ()
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
bool GenMipmapsApp::CreateSampler ()
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
bool GenMipmapsApp::CreateImage ()
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
		info.mipLevels		= uint(CountOf(mipmapViews));
		info.arrayLayers	= 1;
		info.samples		= VK_SAMPLE_COUNT_1_BIT;
		info.tiling			= VK_IMAGE_TILING_OPTIMAL;
		info.usage			= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
							  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
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
	
	// create image view for mipmaps
	for (size_t i = 0; i < CountOf(mipmapViews); ++i)
	{
		VkImageViewCreateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image				= image;
		info.viewType			= VK_IMAGE_VIEW_TYPE_2D;
		info.format				= imageFormat;
		info.components			= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		info.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, uint(i), 1, 0, 1 };

		VK_CALL( vkCreateImageView( vulkan.GetVkDevice(), &info, null, OUT &mipmapViews[i] ));
		
		VkFramebufferCreateInfo	fb_info = {};
		fb_info.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass		= mipmapRenderpass;
		fb_info.attachmentCount	= 1;
		fb_info.width			= Max( imageSize.x >> i, 1u );
		fb_info.height			= Max( imageSize.y >> i, 1u );
		fb_info.layers			= 1;
		fb_info.pAttachments	= &mipmapViews[i];
		
		VK_CALL( vkCreateFramebuffer( vulkan.GetVkDevice(), &fb_info, null, OUT &mipmapFramebuffers[i] ));
	}
	
	// create image view for sampling
	{
		VkImageViewCreateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image				= image;
		info.viewType			= VK_IMAGE_VIEW_TYPE_2D;
		info.format				= imageFormat;
		info.components			= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		info.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, uint(CountOf(mipmapViews)), 0, 1 };

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
			barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, uint(CountOf(mipmapViews)), 0, 1 };

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
bool GenMipmapsApp::CreateDescriptorSet ()
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
bool GenMipmapsApp::CreateGraphicsPipeline ()
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
bool GenMipmapsApp::CreateProceduralImagePipeline ()
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
		images[0].imageView		= mipmapViews[0];

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
bool GenMipmapsApp::CreateProceduralImagePipeline2 ()
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
	info.renderPass				= mipmapRenderpass;
	info.subpass				= 0;

	VK_CHECK( vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &proceduralImage2.pipeline ));
	return true;
}

/*
=================================================
	CreateDownscaleGraphicsPipeline
=================================================
*/
bool GenMipmapsApp::CreateDownscaleGraphicsPipeline ()
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
		CHECK_ERR( spvCompiler.Compile( OUT genGraphics.vertShader, vulkan, {vert_shader_source}, "main", EShLangVertex ));
	}

	// create fragment shader
	{
		static const char	frag_shader_source[] = R"#(
layout(location = 0) out vec4  out_Color;

layout(binding=0) uniform sampler2D  un_Image;

layout(location=0) in vec2  v_Texcoord;

void main ()
{
	out_Color = texture( un_Image, v_Texcoord );	
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT genGraphics.fragShader, vulkan, {frag_shader_source}, "main", EShLangFragment ));
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

		VK_CHECK( vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &genGraphics.dsLayout ));
	}

	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &genGraphics.dsLayout;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &genGraphics.pipelineLayout ));
	}

	VkPipelineShaderStageCreateInfo			stages[2] = {};
	stages[0].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage		= VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module	= genGraphics.vertShader;
	stages[0].pName		= "main";
	stages[1].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage		= VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module	= genGraphics.fragShader;
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
	info.layout					= genGraphics.pipelineLayout;
	info.renderPass				= mipmapRenderpass;
	info.subpass				= 0;

	VK_CHECK( vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &genGraphics.pipeline ));
	
	// allocate descriptor set
	{
		VkDescriptorSetAllocateInfo		ds_info = {};
		ds_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		ds_info.descriptorPool		= descriptorPool;
		ds_info.descriptorSetCount	= 1;
		ds_info.pSetLayouts			= &genGraphics.dsLayout;

		for (auto& ds : genGraphics.descSet) {
			VK_CHECK( vkAllocateDescriptorSets( vulkan.GetVkDevice(), &ds_info, OUT &ds ));
		}
	}

	// update descriptor set
	for (size_t i = 0; i < CountOf(genGraphics.descSet); ++i)
	{
		VkDescriptorImageInfo	images[1] = {};
		images[0].imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		images[0].imageView		= mipmapViews[i];
		images[0].sampler		= sampler;

		VkWriteDescriptorSet	writes[1] = {};
		writes[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet			= genGraphics.descSet[i];
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
	CreateDownscaleComputePipeline
=================================================
*/
bool GenMipmapsApp::CreateDownscaleComputePipeline ()
{
	// find the maximum group size
	uint2	local_size;
	{
		const uint	max_invocations = vulkan.GetDeviceProperties().limits.maxComputeWorkGroupInvocations;
		uint2		size {2};

		for (; (size.x * size.y <= max_invocations) and (IntLog2(size.x) <= int(numMipmaps)+1); size = size << 1) {
			local_size = size;
		}
	}

	const uint	dst_images_per_pass = IntLog2( local_size.x );


	// create compute shader
	{
		String	comp_shader_source;
		comp_shader_source <<
"#extension GL_KHR_memory_scope_semantics : require\n"
"layout (local_size_x = " << ToString(local_size.x) << ", local_size_y = " << ToString(local_size.y) << ", local_size_z = 1) in;\n"
"layout(binding = 0, rgba8) readonly  uniform image2D  un_InImage;\n"
"layout(binding = 1)        writeonly uniform image2D  un_OutImages[" << ToString(dst_images_per_pass) << "];\n"
R"#(
shared vec4  imageData[ gl_WorkGroupSize.x * gl_WorkGroupSize.y ];

void main()
{
	ivec2	size	= ivec2(gl_WorkGroupSize.xy);
	ivec2	coord	= ivec2(gl_LocalInvocationID.xy);
	ivec2	gsize	= ivec2(gl_GlobalInvocationID.xy)*2;

	imageData[gl_LocalInvocationIndex] = (imageLoad( un_InImage, gsize + ivec2(0,0) ) +
										  imageLoad( un_InImage, gsize + ivec2(0,1) ) +
										  imageLoad( un_InImage, gsize + ivec2(1,0) ) +
										  imageLoad( un_InImage, gsize + ivec2(1,1) )) * 0.25f;

	imageStore( un_OutImages[0], (gsize >> 1), imageData[gl_LocalInvocationIndex] );

)#";
		for (uint i = 1; i < dst_images_per_pass; ++i)
		{
			comp_shader_source <<
				"\tbarrier();\n\n"
				"\tif ( (gl_LocalInvocationIndex & " << ToString( (1u << (i+1)) - 1 ) << ") == 0 )\n\t{\n"
				"\t\tmemoryBarrier( gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquireRelease );\n\n"
				"\t\timageData[gl_LocalInvocationIndex] += (imageData[(coord.x+0) + (coord.y+" << ToString(i) << ") * size.x] +\n"
				"\t\t                                       imageData[(coord.x+" << ToString(i) << ") + (coord.y+0) * size.x] +\n"
				"\t\t                                       imageData[(coord.x+" << ToString(i) << ") + (coord.y+" << ToString(i) << ") * size.x]);\n"
				"\t\timageData[gl_LocalInvocationIndex] *= 0.25f;\n"
				"\t\timageStore( un_OutImages[" << ToString(i) << "], (gsize >> " << ToString(i+1) << "), imageData[gl_LocalInvocationIndex] );\n"
				"\t}\n\n";
		}
		comp_shader_source << "}\n";
		CHECK_ERR( spvCompiler.Compile( OUT genCompute.shader, vulkan, {comp_shader_source.data()}, "main", EShLangCompute ));
	}
	
	// create descriptor set layout
	{
		VkDescriptorSetLayoutBinding	binding[2] = {};

		// un_InImage
		binding[0].binding			= 0;
		binding[0].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding[0].descriptorCount	= 1;
		binding[0].stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;

		// un_OutImages[]
		binding[1].binding			= 1;
		binding[1].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding[1].descriptorCount	= dst_images_per_pass;
		binding[1].stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= uint(CountOf( binding ));
		info.pBindings		= binding;

		VK_CHECK( vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &genCompute.dsLayout ));
	}

	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &genCompute.dsLayout;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &genCompute.pipelineLayout ));
	}

	VkComputePipelineCreateInfo		info = {};
	info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
	info.stage.module	= genCompute.shader;
	info.stage.pName	= "main";
	info.layout			= genCompute.pipelineLayout;

	VK_CHECK( vkCreateComputePipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &genCompute.pipeline ));
	
	// allocate and update descriptor set
	for (uint j = 0, ds = 0;
		 (j+dst_images_per_pass < numMipmaps) and (ds < CountOf(genCompute.descSet));
		 j += dst_images_per_pass, ++ds)
	{
		VkDescriptorSetAllocateInfo		ds_info = {};
		ds_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		ds_info.descriptorPool		= descriptorPool;
		ds_info.descriptorSetCount	= 1;
		ds_info.pSetLayouts			= &genCompute.dsLayout;
		VK_CHECK( vkAllocateDescriptorSets( vulkan.GetVkDevice(), &ds_info, OUT &genCompute.descSet[ds] ));

		genCompute.groupCount[ds] = (((imageSize/2) >> (dst_images_per_pass * ds)) + local_size-1) / local_size;
		genCompute.groupCount[ds] = Max( genCompute.groupCount[ds], 1u );

		genCompute.mipmapRanges[ds] = { j+1, dst_images_per_pass };

		VkDescriptorImageInfo	images[20] = {};
		images[0].imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		images[0].imageView		= mipmapViews[j];

		for (uint i = 0; i < dst_images_per_pass; ++i)
		{
			images[i+1].imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
			images[i+1].imageView	= mipmapViews[j+i+1];
		}

		VkWriteDescriptorSet	writes[2] = {};
		writes[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet			= genCompute.descSet[ds];
		writes[0].dstBinding		= 0;
		writes[0].descriptorCount	= 1;
		writes[0].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[0].pImageInfo		= &images[0];
		
		writes[1].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet			= genCompute.descSet[ds];
		writes[1].dstBinding		= 1;
		writes[1].descriptorCount	= dst_images_per_pass;
		writes[1].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[1].pImageInfo		= &images[1];

		vkUpdateDescriptorSets( vulkan.GetVkDevice(), uint(CountOf(writes)), writes, 0, null );
	}
	return true;
}

/*
=================================================
	CreateDownscaleComputePipelineSM6
=================================================
*/
bool GenMipmapsApp::CreateDownscaleComputePipelineSM6 ()
{
	// create compute shader
	{
		static const char	comp_shader_source[] = R"#(
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0)				uniform sampler2D  un_Texture;
layout(binding = 1) writeonly	uniform image2D	un_SwapchainImage;

void main()
{
	vec2	point = (vec3(gl_GlobalInvocationID) / vec3(gl_NumWorkGroups * gl_WorkGroupSize - 1)).xy;

	vec4	color = texture( un_Texture, point ).brga;

	imageStore( un_SwapchainImage, ivec2(gl_GlobalInvocationID.xy), color );
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT genSM6.shader, vulkan, {comp_shader_source}, "main", EShLangCompute ));
	}
	
	// create descriptor set layout
	{
		VkDescriptorSetLayoutBinding	binding[2] = {};
		binding[0].binding			= 0;
		binding[0].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
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

		VK_CHECK( vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &genSM6.dsLayout ));
	}

	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &genSM6.dsLayout;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &genSM6.pipelineLayout ));
	}

	VkComputePipelineCreateInfo		info = {};
	info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
	info.stage.module	= genSM6.shader;
	info.stage.pName	= "main";
	info.layout			= genSM6.pipelineLayout;

	VK_CHECK( vkCreateComputePipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &genSM6.pipeline ));
	
	// allocate descriptor set
	{
		VkDescriptorSetAllocateInfo		ds_info = {};
		ds_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		ds_info.descriptorPool		= descriptorPool;
		ds_info.descriptorSetCount	= 1;
		ds_info.pSetLayouts			= &genSM6.dsLayout;

		for (auto& ds : genSM6.descSet) {
			VK_CHECK( vkAllocateDescriptorSets( vulkan.GetVkDevice(), &ds_info, OUT &ds ));
		}
	}
	return true;
}
}	// anonymous namespace

/*
=================================================
	GenMipmpas_Sample
=================================================
*/
extern void GenMipmpas_Sample ()
{
	GenMipmapsApp	app;
	
	if ( app.Initialize() )
	{
		app.Run();
		app.Destroy();
	}
}

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "compiler/SpvCompiler.h"
#include "stl/Math/Color.h"

namespace {

class CacheTestApp final : public IWindowEventListener, public VulkanDeviceFn
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
	
	VkDescriptorPool		descriptorPool		= VK_NULL_HANDLE;

	struct {
		VkPipeline				handle		= VK_NULL_HANDLE;
		VkPipelineLayout		layout		= VK_NULL_HANDLE;
		VkShaderModule			shader		= VK_NULL_HANDLE;
		VkDescriptorSet			descSet		= VK_NULL_HANDLE;
		VkDescriptorSetLayout	dsLayout	= VK_NULL_HANDLE;
		const uint2				localSize	{ 32, 32 };
	}						pipeline1;
	
	struct {
		VkPipeline				handle		= VK_NULL_HANDLE;
		VkPipelineLayout		layout		= VK_NULL_HANDLE;
		VkShaderModule			shader		= VK_NULL_HANDLE;
		VkDescriptorSet			descSet		= VK_NULL_HANDLE;
		VkDescriptorSetLayout	dsLayout	= VK_NULL_HANDLE;
	}						pipeline2;

	static constexpr uint	imageSize		= 1024;
	static constexpr uint	bufferSize		= sizeof(float4) * 1024;

	VkBuffer				buffer				= VK_NULL_HANDLE;
	VkImage					renderTarget		= VK_NULL_HANDLE;
	VkImageView				renderTargetView	= VK_NULL_HANDLE;
	VkDeviceMemory			sharedMemory		= VK_NULL_HANDLE;

	bool					looping			= true;


public:
	CacheTestApp ()
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

	bool CreateCommandBuffers ();
	bool CreateSyncObjects ();
	bool CreateResources ();
	bool CreateDescriptorSet ();
	bool CreatePipeline1 ();
	bool CreatePipeline2 ();
};



/*
=================================================
	OnKey
=================================================
*/
void CacheTestApp::OnKey (StringView key, EKeyAction action)
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
void CacheTestApp::OnResize (const uint2 &size)
{
	VK_CALL( vkDeviceWaitIdle( vulkan.GetVkDevice() ));

	VK_CALL( vkResetCommandPool( vulkan.GetVkDevice(), cmdPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT ));

	CHECK( swapchain->Recreate( size ));
}

/*
=================================================
	Initialize
=================================================
*/
bool CacheTestApp::Initialize ()
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
		CHECK_ERR( window->Create( { 800, 600 }, "Cache invalidation test" ));
		window->AddListener( this );

		CHECK_ERR( vulkan.Create( window->GetVulkanSurface(),
								  "Cache invalidation test", "Engine",
								  VK_API_VERSION_1_1,
								  "",
								  {{ VK_QUEUE_PRESENT_BIT | VK_QUEUE_GRAPHICS_BIT, 0.0f }},
								  VulkanDevice::GetRecomendedInstanceLayers(),
								  VulkanDevice::GetRecomendedInstanceExtensions(),
								  {}
			));
		
		vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All );
	}


	// initialize swapchain
	{
		VkFormat		color_fmt	= VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR	color_space	= VK_COLOR_SPACE_MAX_ENUM_KHR;

		swapchain.reset( new VulkanSwapchain{ vulkan } );

		CHECK_ERR( swapchain->ChooseColorFormat( INOUT color_fmt, INOUT color_space ));

		CHECK_ERR( swapchain->Create( window->GetSize(), color_fmt, color_space/*, 2, VK_PRESENT_MODE_MAILBOX_KHR*/ ));
	}


	// initialize vulkan objects
	cmdQueue = vulkan.GetVkQueues().front().handle;

	CHECK_ERR( CreateCommandBuffers() );
	CHECK_ERR( CreateSyncObjects() );
	CHECK_ERR( CreateResources() );
	CHECK_ERR( CreateDescriptorSet() );
	CHECK_ERR( CreatePipeline1() );
	CHECK_ERR( CreatePipeline2() );
	return true;
}

/*
=================================================
	Destroy
=================================================
*/
void CacheTestApp::Destroy ()
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
	vkDestroyDescriptorPool( dev, descriptorPool, null );
	vkDestroyBuffer( dev, buffer, null );
	vkDestroyImage( dev, renderTarget, null );
	vkDestroyImageView( dev, renderTargetView, null );
	vkFreeMemory( dev, sharedMemory, null );

	vkDestroyPipeline( dev, pipeline1.handle, null );
	vkDestroyPipelineLayout( dev, pipeline1.layout, null );
	vkDestroyDescriptorSetLayout( dev, pipeline1.dsLayout, null );
	vkDestroyShaderModule( dev, pipeline1.shader, null );
	
	vkDestroyPipeline( dev, pipeline2.handle, null );
	vkDestroyPipelineLayout( dev, pipeline2.layout, null );
	vkDestroyDescriptorSetLayout( dev, pipeline2.dsLayout, null );
	vkDestroyShaderModule( dev, pipeline2.shader, null );

	cmdPool			= VK_NULL_HANDLE;
	cmdQueue		= VK_NULL_HANDLE;
	descriptorPool	= VK_NULL_HANDLE;
	buffer			= VK_NULL_HANDLE;
	sharedMemory	= VK_NULL_HANDLE;

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
bool CacheTestApp::Run ()
{
	for (uint frameId = 0; looping; ++frameId)
	{
		if ( not window->Update() )
			break;

		const uint	i = frameId & 1;

		// wait and acquire next image
		{
			VK_CHECK( vkWaitForFences( vulkan.GetVkDevice(), 1, &fences[i], true, UMax ));
			VK_CHECK( vkResetFences( vulkan.GetVkDevice(), 1, &fences[i] ));

			VK_CALL( swapchain->AcquireNextImage( semaphores[0] ));
		}

		// build command buffer
		{
			VkCommandBufferBeginInfo	begin_info = {};
			begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			VK_CALL( vkBeginCommandBuffer( cmdBuffers[i], &begin_info ));
			
			if ( frameId % 60 == 0 )
			{
				// undefined -> general
				{
					VkImageMemoryBarrier	barrier = {};
					barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier.image				= renderTarget;
					barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
					barrier.newLayout			= VK_IMAGE_LAYOUT_GENERAL;
					barrier.srcAccessMask		= 0;
					barrier.dstAccessMask		= VK_ACCESS_SHADER_WRITE_BIT;
					barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
					barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

					vkCmdPipelineBarrier( cmdBuffers[i],
										  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
										  0, null, 0, null, 1, &barrier );
				}

				// clear buffer
				{
					vkCmdFillBuffer( cmdBuffers[i], buffer, 0, bufferSize, 0 );
					
					VkBufferMemoryBarrier	barrier = {};
					barrier.sType			= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
					barrier.srcAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask	= VK_ACCESS_SHADER_WRITE_BIT;
					barrier.buffer			= buffer;
					barrier.offset			= 0;
					barrier.size			= bufferSize;
					barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;

					vkCmdPipelineBarrier( cmdBuffers[i],
										  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
										  0, null, 1, &barrier, 0, null );
				}

				// write to buffer
				{
					vkCmdBindPipeline( cmdBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline2.handle );
					vkCmdBindDescriptorSets( cmdBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline2.layout, 0, 1, &pipeline2.descSet, 0, null );

					uint4	swizzle{ 0, 1, 2, frameId };
					vkCmdPushConstants( cmdBuffers[i], pipeline2.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(swizzle), &swizzle );
				
					vkCmdDispatch( cmdBuffers[i], 1, 1, 1 );
				}

				// read after write
				{
					/*VkBufferMemoryBarrier	barrier = {};
					barrier.sType			= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
					barrier.srcAccessMask	= VK_ACCESS_SHADER_WRITE_BIT;
					barrier.dstAccessMask	= VK_ACCESS_UNIFORM_READ_BIT;
					barrier.buffer			= buffer;
					barrier.offset			= 0;
					barrier.size			= bufferSize;
					barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;

					vkCmdPipelineBarrier( cmdBuffers[i],
										  VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
										  //VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
										  0, null,
										  0, null,
										  //1, &barrier,
										  0, null );*/
				}

				// compute pass
				{
					vkCmdBindPipeline( cmdBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline1.handle );
					vkCmdBindDescriptorSets( cmdBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline1.layout, 0, 1, &pipeline1.descSet, 0, null );

					const uint2	count = (uint2{imageSize} + pipeline1.localSize - 1) / pipeline1.localSize;

					vkCmdDispatch( cmdBuffers[i], count.x, count.y, 1 );
				}
			
				// general -> transfer_src
				{
					VkImageMemoryBarrier	barrier = {};
					barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier.image				= renderTarget;
					barrier.oldLayout			= VK_IMAGE_LAYOUT_GENERAL;
					barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					barrier.srcAccessMask		= VK_ACCESS_SHADER_WRITE_BIT;
					barrier.dstAccessMask		= VK_ACCESS_TRANSFER_READ_BIT;
					barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
					barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

					vkCmdPipelineBarrier( cmdBuffers[i],
										  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
										  0, null, 0, null, 1, &barrier );
				}
			}
			
			// undefined -> transfer_dst
			{
				VkImageMemoryBarrier	barrier = {};
				barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image				= swapchain->GetCurrentImage();
				barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcAccessMask		= 0;
				barrier.dstAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

				vkCmdPipelineBarrier( cmdBuffers[i],
									  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
									  0, null, 0, null, 1, &barrier );
			}

			// blit image
			{
				VkImageBlit	region = {};
				region.srcOffsets[1]	= { imageSize, imageSize, 1 };
				region.srcSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
				region.dstOffsets[1]	= { int(swapchain->GetSurfaceSize().x), int(swapchain->GetSurfaceSize().y), 1 };
				region.dstSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };

				vkCmdBlitImage( cmdBuffers[i], renderTarget, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							    swapchain->GetCurrentImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							    1, &region, VK_FILTER_LINEAR );
			}

			// transfer_dst -> present_src
			{
				VkImageMemoryBarrier	barrier = {};
				barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image				= swapchain->GetCurrentImage();
				barrier.oldLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout			= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				barrier.srcAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask		= 0;
				barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

				vkCmdPipelineBarrier( cmdBuffers[i],
									  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
									  0, null, 0, null, 1, &barrier );
			}

			VK_CALL( vkEndCommandBuffer( cmdBuffers[i] ));
		}

		// submit commands
		{
			VkSemaphore				signal_semaphores[] = { semaphores[1] };
			VkSemaphore				wait_semaphores[]	= { semaphores[0] };
			VkPipelineStageFlags	wait_dst_mask[]		= { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
			STATIC_ASSERT( CountOf(wait_semaphores) == CountOf(wait_dst_mask) );

			VkSubmitInfo				submit_info = {};
			submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.commandBufferCount		= 1;
			submit_info.pCommandBuffers			= &cmdBuffers[i];
			submit_info.waitSemaphoreCount		= uint(CountOf(wait_semaphores));
			submit_info.pWaitSemaphores			= wait_semaphores;
			submit_info.pWaitDstStageMask		= wait_dst_mask;
			submit_info.signalSemaphoreCount	= uint(CountOf(signal_semaphores));
			submit_info.pSignalSemaphores		= signal_semaphores;

			VK_CHECK( vkQueueSubmit( cmdQueue, 1, &submit_info, fences[i] ));
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
bool CacheTestApp::CreateCommandBuffers ()
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
	info.commandBufferCount	= uint(CountOf( cmdBuffers ));
	VK_CHECK( vkAllocateCommandBuffers( vulkan.GetVkDevice(), &info, OUT cmdBuffers ));

	return true;
}

/*
=================================================
	CreateSyncObjects
=================================================
*/
bool CacheTestApp::CreateSyncObjects ()
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
	CreateResources
=================================================
*/
bool CacheTestApp::CreateResources ()
{
	VkDeviceSize					total_size		= 0;
	uint							mem_type_bits	= 0;
	VkMemoryPropertyFlags			mem_property	= 0;
	Array<std::function<void ()>>	bind_mem;

	// create uniform/storage buffer
	{
		VkBufferCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags			= 0;
		info.size			= bufferSize;
		info.usage			= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vkCreateBuffer( vulkan.GetVkDevice(), &info, null, OUT &buffer ));

		VkMemoryRequirements	mem_req;
		vkGetBufferMemoryRequirements( vulkan.GetVkDevice(), buffer, OUT &mem_req );

		VkDeviceSize	offset = AlignToLarger( total_size, mem_req.alignment );
		total_size		 = offset + mem_req.size;
		mem_type_bits	|= mem_req.memoryTypeBits;
		mem_property	|= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		bind_mem.push_back( [this, offset] () {
			VK_CALL( vkBindBufferMemory( vulkan.GetVkDevice(), buffer, sharedMemory, offset ));
		});
	}

	// create render target
	{
		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.flags			= 0;
		info.imageType		= VK_IMAGE_TYPE_2D;
		info.format			= VK_FORMAT_R8G8B8A8_UNORM;
		info.extent			= { imageSize, imageSize, 1 };
		info.mipLevels		= 1;
		info.arrayLayers	= 1;
		info.samples		= VK_SAMPLE_COUNT_1_BIT;
		info.tiling			= VK_IMAGE_TILING_OPTIMAL;
		info.usage			= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		info.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vkCreateImage( vulkan.GetVkDevice(), &info, null, OUT &renderTarget ));
		
		VkMemoryRequirements	mem_req;
		vkGetImageMemoryRequirements( vulkan.GetVkDevice(), renderTarget, OUT &mem_req );
		
		VkDeviceSize	offset = AlignToLarger( total_size, mem_req.alignment );
		total_size		 = offset + mem_req.size;
		mem_type_bits	|= mem_req.memoryTypeBits;
		mem_property	|= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		bind_mem.push_back( [this, offset] () {
			VK_CALL( vkBindImageMemory( vulkan.GetVkDevice(), renderTarget, sharedMemory, offset ));

			VkImageViewCreateInfo	view = {};
			view.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view.image				= renderTarget;
			view.viewType			= VK_IMAGE_VIEW_TYPE_2D;
			view.format				= VK_FORMAT_R8G8B8A8_UNORM;
			view.components			= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
			view.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			VK_CALL( vkCreateImageView( vulkan.GetVkDevice(), &view, null, OUT &renderTargetView ));
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

	VK_CALL( vkQueueWaitIdle( cmdQueue ));
	return true;
}

/*
=================================================
	CreateDescriptorSet
=================================================
*/
bool CacheTestApp::CreateDescriptorSet ()
{
	const VkDescriptorPoolSize		sizes[] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  100 }
	};

	VkDescriptorPoolCreateInfo		info = {};
	info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.maxSets		= 100;
	info.poolSizeCount	= uint(CountOf( sizes ));
	info.pPoolSizes		= sizes;

	VK_CHECK( vkCreateDescriptorPool( vulkan.GetVkDevice(), &info, null, OUT &descriptorPool ));
	return true;
}

/*
=================================================
	CreatePipeline1
=================================================
*/
bool CacheTestApp::CreatePipeline1 ()
{
	// create compute shader
	{
		static const char	comp_shader_source[] = R"#(
layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0, std140) uniform UB {
	vec4	color1[1024];
};
layout(binding = 2, std430) readonly buffer SSB {
	vec4	color2[1024];
};

layout(binding = 1) writeonly uniform image2D  un_SwapchainImage;

void main()
{
	uvec2 size		= imageSize( un_SwapchainImage );
	bool  is_buffer	= gl_GlobalInvocationID.y*2 < size.y;
	uint  index		= gl_GlobalInvocationID.x & 1023;

	imageStore( un_SwapchainImage, ivec2(gl_GlobalInvocationID.xy), is_buffer ? color2[index] : color1[index] );
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT pipeline1.shader, vulkan, {comp_shader_source}, "main", EShLangCompute ));
	}
	
	// create descriptor set layout
	{
		VkDescriptorSetLayoutBinding	binding[3] = {};
		binding[0].binding			= 0;
		binding[0].descriptorType	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding[0].descriptorCount	= 1;
		binding[0].stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;

		binding[2].binding			= 2;
		binding[2].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		binding[2].descriptorCount	= 1;
		binding[2].stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;

		binding[1].binding			= 1;
		binding[1].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding[1].descriptorCount	= 1;
		binding[1].stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= uint(CountOf( binding ));
		info.pBindings		= binding;

		VK_CHECK( vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &pipeline1.dsLayout ));
	}

	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &pipeline1.dsLayout;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &pipeline1.layout ));
	}

	// create pipeline
	{
		VkComputePipelineCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
		info.stage.module	= pipeline1.shader;
		info.stage.pName	= "main";
		info.layout			= pipeline1.layout;

		VK_CHECK( vkCreateComputePipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &pipeline1.handle ));
	}

	// allocate descriptor set
	{
		VkDescriptorSetAllocateInfo		info = {};
		info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool		= descriptorPool;
		info.descriptorSetCount	= 1;
		info.pSetLayouts		= &pipeline1.dsLayout;

		VK_CHECK( vkAllocateDescriptorSets( vulkan.GetVkDevice(), &info, OUT &pipeline1.descSet ));
	}

	// update descriptor set
	{
		VkDescriptorBufferInfo	buf_desc = {};
		buf_desc.buffer			= buffer;
		buf_desc.offset			= 0;
		buf_desc.range			= bufferSize;

		VkDescriptorImageInfo	img_desc = {};
		img_desc.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		img_desc.imageView		= renderTargetView;
		img_desc.sampler		= VK_NULL_HANDLE;

		VkWriteDescriptorSet	writes[3] = {};
		writes[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet			= pipeline1.descSet;
		writes[0].dstBinding		= 0;
		writes[0].descriptorCount	= 1;
		writes[0].descriptorType	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].pBufferInfo		= &buf_desc;
		
		writes[1].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet			= pipeline1.descSet;
		writes[1].dstBinding		= 1;
		writes[1].descriptorCount	= 1;
		writes[1].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[1].pImageInfo		= &img_desc;

		writes[2].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[2].dstSet			= pipeline1.descSet;
		writes[2].dstBinding		= 2;
		writes[2].descriptorCount	= 1;
		writes[2].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[2].pBufferInfo		= &buf_desc;

		vkUpdateDescriptorSets( vulkan.GetVkDevice(), uint(CountOf( writes )), writes, 0, null );
	}
	return true;
}

/*
=================================================
	CreatePipeline2
=================================================
*/
bool CacheTestApp::CreatePipeline2 ()
{
	// create compute shader
	{
		static const char	comp_shader_source[] = R"#(
layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, std430) writeonly buffer SSB {
	vec4	color[1024];
};

layout(push_constant, std140) uniform PC {
	uvec3	swizzle;
	uint	frameId;
};

void main()
{
	vec4	c = vec4(1.0f);
	float	t = fract( float(frameId) / 360.0f );

	c[swizzle.x] = (gl_GlobalInvocationID.x & 15) > 7 ? 1.0f : 0.0f;
	c[swizzle.y] = gl_GlobalInvocationID.x / 1024.0f;
	c[swizzle.z] = t;

	color[gl_GlobalInvocationID.x] = c;
}
)#";
		CHECK_ERR( spvCompiler.Compile( OUT pipeline2.shader, vulkan, {comp_shader_source}, "main", EShLangCompute ));
	}
	
	// create descriptor set layout
	{
		VkDescriptorSetLayoutBinding	binding[1] = {};
		binding[0].binding			= 0;
		binding[0].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		binding[0].descriptorCount	= 1;
		binding[0].stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= uint(CountOf( binding ));
		info.pBindings		= binding;

		VK_CHECK( vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &pipeline2.dsLayout ));
	}

	// create pipeline layout
	{
		VkPushConstantRange	range = {};
		range.size			= sizeof(uint4);
		range.stageFlags	= VK_SHADER_STAGE_COMPUTE_BIT;

		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &pipeline2.dsLayout;
		info.pushConstantRangeCount	= 1;
		info.pPushConstantRanges	= &range;

		VK_CHECK( vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &pipeline2.layout ));
	}

	// create pipeline
	{
		VkComputePipelineCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
		info.stage.module	= pipeline2.shader;
		info.stage.pName	= "main";
		info.layout			= pipeline2.layout;

		VK_CHECK( vkCreateComputePipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &pipeline2.handle ));
	}

	// allocate descriptor set
	{
		VkDescriptorSetAllocateInfo		info = {};
		info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool		= descriptorPool;
		info.descriptorSetCount	= 1;
		info.pSetLayouts		= &pipeline2.dsLayout;

		VK_CHECK( vkAllocateDescriptorSets( vulkan.GetVkDevice(), &info, OUT &pipeline2.descSet ));
	}

	// update descriptor set
	{
		VkDescriptorBufferInfo	buf_desc = {};
		buf_desc.buffer			= buffer;
		buf_desc.offset			= 0;
		buf_desc.range			= bufferSize;

		VkWriteDescriptorSet	writes[1] = {};
		writes[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet			= pipeline2.descSet;
		writes[0].dstBinding		= 0;
		writes[0].descriptorCount	= 1;
		writes[0].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[0].pBufferInfo		= &buf_desc;

		vkUpdateDescriptorSets( vulkan.GetVkDevice(), uint(CountOf( writes )), writes, 0, null );
	}
	return true;
}
}	// anonymous namespace

/*
=================================================
	CacheTest_Sample1
=================================================
*/
extern void CacheTest_Sample1 ()
{
	CacheTestApp	app;
	
	if ( app.Initialize() )
	{
		app.Run();
		app.Destroy();
	}
}

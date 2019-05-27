// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framework/Vulkan/VulkanDeviceExt.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "framework/Window/WindowSFML.h"
#include "compiler/SpvCompiler.h"
#include "stl/Algorithms/StringUtils.h"

namespace {

class ZOrderPatternApp final : public IWindowEventListener, public VulkanDeviceFn
{
private:
	VulkanDeviceExt			vulkan;
	WindowPtr				window;
	SpvCompiler				spvCompiler;


public:
	ZOrderPatternApp ()
	{
		VulkanDeviceFn_Init( vulkan );
	}
	
	void OnKey (StringView, EKeyAction) override {}
	void OnResize (const uint2 &) override {}
	
	void OnRefresh () override {}
	void OnDestroy () override {}
	void OnUpdate () override {}
	void OnMouseMove (const float2 &) override {}

	bool Initialize ();
	void Destroy ();
	bool Run ();
};


/*
=================================================
	Initialize
=================================================
*/
bool ZOrderPatternApp::Initialize ()
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
		const char	title[] = "Z-ordering pattern";

		CHECK_ERR( window->Create( { 800, 600 }, title ));
		window->AddListener( this );

		CHECK_ERR( vulkan.Create( window->GetVulkanSurface(),
								  title, "Engine",
								  VK_API_VERSION_1_1,
								  "",
								  {{ VK_QUEUE_PRESENT_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0.0f }},
								  VulkanDevice::GetRecomendedInstanceLayers(),
								  VulkanDevice::GetRecomendedInstanceExtensions(),
								  { VK_NV_SHADER_IMAGE_FOOTPRINT_EXTENSION_NAME }
			));
		
		vulkan.CreateDebugUtilsCallback( VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT );
	}
	return true;
}

/*
=================================================
	Destroy
=================================================
*/
void ZOrderPatternApp::Destroy ()
{
	VK_CALL( vkDeviceWaitIdle( vulkan.GetVkDevice() ));

	vulkan.Destroy();

	window->Destroy();
	window.reset();
}

/*
=================================================
	Run
=================================================
*/
bool ZOrderPatternApp::Run ()
{
	VkQueue			cmd_queue	= vulkan.GetVkQueues().front().handle;
	VkCommandPool	cmd_pool	= VK_NULL_HANDLE;
	VkCommandBuffer	cmd_buffer	= VK_NULL_HANDLE;

	// create command pool and buffer
	{
		VkCommandPoolCreateInfo		pool_info = {};
		pool_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex	= vulkan.GetVkQueues().front().familyIndex;
		pool_info.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK( vkCreateCommandPool( vulkan.GetVkDevice(), &pool_info, null, OUT &cmd_pool ));

		VkCommandBufferAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.pNext				= null;
		info.commandPool		= cmd_pool;
		info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandBufferCount	= 1;
		VK_CHECK( vkAllocateCommandBuffers( vulkan.GetVkDevice(), &info, OUT &cmd_buffer ));
	}
	

	// create staging buffer
	VkBuffer		staging_buffer	= VK_NULL_HANDLE;
	VkDeviceMemory	staging_memory	= VK_NULL_HANDLE;
	uint *			mapped			= null;
	const uint2		img_dim			{ 64, 64 };

	{
		VkBufferCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags			= 0;
		info.size			= img_dim.x * img_dim.y * sizeof(uint);
		info.usage			= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vkCreateBuffer( vulkan.GetVkDevice(), &info, null, OUT &staging_buffer ));
		
		VkMemoryRequirements	mem_req;
		vkGetBufferMemoryRequirements( vulkan.GetVkDevice(), staging_buffer, OUT &mem_req );
	
		VkMemoryAllocateInfo	alloc = {};
		alloc.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc.allocationSize	= mem_req.size;
		CHECK_ERR( vulkan.GetMemoryTypeIndex( mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, OUT alloc.memoryTypeIndex ));

		VK_CHECK( vkAllocateMemory( vulkan.GetVkDevice(), &alloc, null, OUT &staging_memory ));
		
		VK_CALL( vkBindBufferMemory( vulkan.GetVkDevice(), staging_buffer, staging_memory, 0 ));

		VK_CALL( vkMapMemory( vulkan.GetVkDevice(), staging_memory, 0, info.size, 0, OUT BitCast<void **>(&mapped) ));

		for (uint y = 0; y < img_dim.y; ++y)
		for (uint x = 0; x < img_dim.x; ++x)
		{
			mapped[y * img_dim.x + x] = x | (y << 16);
		}
	}


	VkImage					image			= VK_NULL_HANDLE;
	VkBuffer				buffer			= VK_NULL_HANDLE;
	VkDeviceMemory			shared_mem		= VK_NULL_HANDLE;
	VkDeviceSize			total_size		= 0;
	uint					mem_type_bits	= 0;
	VkMemoryPropertyFlags	mem_property	= 0;

	// create image
	{
		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.flags			= 0;
		info.imageType		= VK_IMAGE_TYPE_2D;
		info.format			= VK_FORMAT_R32_UINT;
		info.extent			= { img_dim.x, img_dim.y, 1 };
		info.mipLevels		= 1;
		info.arrayLayers	= 1;
		info.samples		= VK_SAMPLE_COUNT_1_BIT;
		info.tiling			= VK_IMAGE_TILING_OPTIMAL;
		info.usage			= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;

		VK_CHECK( vkCreateImage( vulkan.GetVkDevice(), &info, null, OUT &image ));
		
		VkMemoryRequirements	mem_req;
		vkGetImageMemoryRequirements( vulkan.GetVkDevice(), image, OUT &mem_req );
		
		total_size		 = mem_req.size;
		mem_type_bits	|= mem_req.memoryTypeBits;
		mem_property	|= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}

	// create buffer
	{
		VkBufferCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags			= 0;
		info.size			= img_dim.x * img_dim.y * sizeof(uint);
		info.usage			= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vkCreateBuffer( vulkan.GetVkDevice(), &info, null, OUT &buffer ));
		
		VkMemoryRequirements	mem_req;
		vkGetBufferMemoryRequirements( vulkan.GetVkDevice(), buffer, OUT &mem_req );
		
		CHECK( total_size >= mem_req.size );
		mem_type_bits	|= mem_req.memoryTypeBits;
		mem_property	|= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}

	// allocate memory
	{
		VkMemoryAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize		= total_size;
		CHECK_ERR( vulkan.GetMemoryTypeIndex( mem_type_bits, mem_property, OUT info.memoryTypeIndex ));

		VK_CHECK( vkAllocateMemory( vulkan.GetVkDevice(), &info, null, OUT &shared_mem ));
		
		VK_CALL( vkBindImageMemory( vulkan.GetVkDevice(), image, shared_mem, 0 ));
		VK_CALL( vkBindBufferMemory( vulkan.GetVkDevice(), buffer, shared_mem, 0 ));
	}
	
	// update resources
	{
		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vkBeginCommandBuffer( cmd_buffer, &begin_info ));

		// undefined -> transfer_dst
		VkImageMemoryBarrier	barrier = {};
		barrier.sType				= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image				= image;
		barrier.oldLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout			= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask		= 0;
		barrier.dstAccessMask		= VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdPipelineBarrier( cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, null, 0, null, 1, &barrier );

		VkBufferImageCopy	img_copy = {};
		img_copy.bufferOffset		= 0;
		img_copy.bufferRowLength	= img_dim.x;
		img_copy.bufferImageHeight	= img_dim.y;
		img_copy.imageSubresource	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		img_copy.imageOffset		= { 0, 0, 0 };
		img_copy.imageExtent		= { img_dim.x, img_dim.y, 1 };
		vkCmdCopyBufferToImage( cmd_buffer, staging_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_copy );
		
		// read after write
		barrier.oldLayout		= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.srcAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
		vkCmdPipelineBarrier( cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, null, 0, null, 1, &barrier );

		VkBufferCopy	buf_copy = {};
		buf_copy.srcOffset	= 0;
		buf_copy.dstOffset	= 0;
		buf_copy.size		= img_dim.x * img_dim.y * sizeof(uint);
		vkCmdCopyBuffer( cmd_buffer, buffer, staging_buffer, 1, &buf_copy );

		VK_CALL( vkEndCommandBuffer( cmd_buffer ));

		VkSubmitInfo		submit_info = {};
		submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount	= 1;
		submit_info.pCommandBuffers		= &cmd_buffer;
		VK_CHECK( vkQueueSubmit( cmd_queue, 1, &submit_info, VK_NULL_HANDLE ));
	}
	VK_CALL( vkQueueWaitIdle( cmd_queue ));

	
	// find block size
	uint2		max_coord;
	const uint	data_size = img_dim.x * img_dim.y;
	
	for (uint i = 0; i < data_size; ++i)
	{
		uint2	coord = uint2(mapped[i] & 0xFFFF, mapped[i] >> 16) + 1;

		if ( max_coord.x == max_coord.y and (coord.x > max_coord.x or coord.y > max_coord.y) and i > 1 and max_coord.x * max_coord.y == i )
			break;

		max_coord = Max( max_coord, coord );
	}

	// get pattern
	const uint	block_size = max_coord.x * max_coord.y;

	if ( block_size != data_size )
	{
		String	str = "Image z-order:\n";

		for (uint i = 0; i < block_size and i < data_size; ++i)
		{
			uint2	coord = uint2(mapped[i] & 0xFFFF, mapped[i] >> 16);

			str << '[' << ToString(coord.x) <<',' << ToString(coord.y) << ']' << (i and ((i+1) % max_coord.x == 0) ? '\n' : ' ');
		}

		FG_LOGI( str );
	}

	vkUnmapMemory( vulkan.GetVkDevice(), staging_memory );
	vkFreeMemory( vulkan.GetVkDevice(), staging_memory, null );
	vkFreeMemory( vulkan.GetVkDevice(), shared_mem, null );
	vkDestroyImage( vulkan.GetVkDevice(), image, null );
	vkDestroyBuffer( vulkan.GetVkDevice(), buffer, null );
	vkDestroyBuffer( vulkan.GetVkDevice(), staging_buffer, null );
	vkFreeCommandBuffers( vulkan.GetVkDevice(), cmd_pool, 1, &cmd_buffer );
	vkDestroyCommandPool( vulkan.GetVkDevice(), cmd_pool, null );

	return true;
}
}	// anonymous namespace

/*
=================================================
	ZOrderPattern_Sample1
=================================================
*/
extern void ZOrderPattern_Sample1 ()
{
	ZOrderPatternApp	app;
	
	if ( app.Initialize() )
	{
		app.Run();
		app.Destroy();
	}
}

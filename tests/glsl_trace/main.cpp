// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "GLSLShaderTraceTestUtils.h"
#include "stl/Algorithms/ArrayUtils.h"

extern bool GLSLShaderTrace_Test1 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool GLSLShaderTrace_Test2 (VulkanDeviceExt& vulkan, const TestHelpers &helper);


/*
=================================================
	CreateDebugDescSetLayout
=================================================
*/
bool CreateDebugDescriptorSet (VulkanDevice &vulkan, const TestHelpers &helper, VkShaderStageFlagBits stage,
							   OUT VkDescriptorSetLayout &dsLayout, OUT VkDescriptorSet &descSet)
{
	// create layout
	{
		VkDescriptorSetLayoutBinding	binding = {};
		binding.binding			= 0;
		binding.descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		binding.descriptorCount	= 1;
		binding.stageFlags		= stage;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= 1;
		info.pBindings		= &binding;

		VK_CHECK( vulkan.vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &dsLayout ));
	}
	
	// allocate descriptor set
	{
		VkDescriptorSetAllocateInfo		info = {};
		info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool		= helper.descPool;
		info.descriptorSetCount	= 1;
		info.pSetLayouts		= &dsLayout;

		VK_CHECK( vulkan.vkAllocateDescriptorSets( vulkan.GetVkDevice(), &info, OUT &descSet ));
	}

	// update descriptor set
	{
		VkDescriptorBufferInfo	buffer	= {};
		buffer.buffer	= helper.debugOutputBuf;
		buffer.range	= VK_WHOLE_SIZE;
		
		VkWriteDescriptorSet	write	= {};
		write.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet			= descSet;
		write.dstBinding		= 0;
		write.descriptorCount	= 1;
		write.descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write.pBufferInfo		= &buffer;
		
		vulkan.vkUpdateDescriptorSets( vulkan.GetVkDevice(), 1, &write, 0, null );
	}
	return true;
}

/*
=================================================
	CreateRenderTarget
=================================================
*/
bool CreateRenderTarget (VulkanDeviceExt &vulkan, VkFormat colorFormat, uint width, uint height, VkImageUsageFlags imageUsage,
						 OUT VkRenderPass &outRenderPass, OUT VkImage &outImage,
						 OUT VkDeviceMemory &outImageMem, OUT VkFramebuffer &outFramebuffer)
{
	// create image
	{
		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.flags			= 0;
		info.imageType		= VK_IMAGE_TYPE_2D;
		info.format			= colorFormat;
		info.extent			= { width, height, 1 };
		info.mipLevels		= 1;
		info.arrayLayers	= 1;
		info.samples		= VK_SAMPLE_COUNT_1_BIT;
		info.tiling			= VK_IMAGE_TILING_OPTIMAL;
		info.usage			= imageUsage | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;

		VK_CHECK( vulkan.vkCreateImage( vulkan.GetVkDevice(), &info, null, OUT &outImage ));

		VkMemoryRequirements	mem_req;
		vulkan.vkGetImageMemoryRequirements( vulkan.GetVkDevice(), outImage, OUT &mem_req );
		
		// allocate device local memory
		VkMemoryAllocateInfo	alloc_info = {};
		alloc_info.sType			= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize	= mem_req.size;
		CHECK_ERR( vulkan.GetMemoryTypeIndex( mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, OUT alloc_info.memoryTypeIndex ));

		VK_CHECK( vulkan.vkAllocateMemory( vulkan.GetVkDevice(), &alloc_info, null, OUT &outImageMem ));
		VK_CHECK( vulkan.vkBindImageMemory( vulkan.GetVkDevice(), outImage, outImageMem, 0 ));
	}

	// create image view
	VkImageView		view = VK_NULL_HANDLE;
	{
		VkImageViewCreateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.flags				= 0;
		info.image				= outImage;
		info.viewType			= VK_IMAGE_VIEW_TYPE_2D;
		info.format				= colorFormat;
		info.components			= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		info.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		VK_CHECK( vulkan.vkCreateImageView( vulkan.GetVkDevice(), &info, null, OUT &view ));
	}

	// create renderpass
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

		VK_CHECK( vulkan.vkCreateRenderPass( vulkan.GetVkDevice(), &info, null, OUT &outRenderPass ));
	}
	
	// create framebuffer
	{
		VkFramebufferCreateInfo		info = {};
		info.sType				= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.flags				= 0;
		info.renderPass			= outRenderPass;
		info.attachmentCount	= 1;
		info.pAttachments		= &view;
		info.width				= width;
		info.height				= height;
		info.layers				= 1;

		VK_CHECK( vulkan.vkCreateFramebuffer( vulkan.GetVkDevice(), &info, null, OUT &outFramebuffer ));
	}
	return true;
}

/*
=================================================
	main
=================================================
*/
int main ()
{
	VulkanDeviceExt	vulkan;

	// create vulkan device
	{
		CHECK_ERR( vulkan.Create( "GLSL debugger", "Engine",
								  VK_API_VERSION_1_1,
								  "",
								  {{ VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0.0f }},
								  VulkanDevice::GetRecomendedInstanceLayers(),
								  VulkanDevice::GetRecomendedInstanceExtensions(),
								  VulkanDevice::GetRecomendedDeviceExtensions()
			));
		vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All );
	}

	TestHelpers		helper;
	VkDevice		dev	= vulkan.GetVkDevice();
	
	helper.queue		= vulkan.GetVkQuues().front().id;
	helper.queueFamily	= vulkan.GetVkQuues().front().familyIndex;

	// create command pool
	{
		VkCommandPoolCreateInfo		info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		info.queueFamilyIndex	= helper.queueFamily;

		VK_CHECK( vulkan.vkCreateCommandPool( dev, &info, null, OUT &helper.cmdPool ));

		VkCommandBufferAllocateInfo		alloc = {};
		alloc.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc.commandPool			= helper.cmdPool;
		alloc.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc.commandBufferCount	= 1;

		VK_CHECK( vulkan.vkAllocateCommandBuffers( dev, &alloc, OUT &helper.cmdBuffer ));
	}

	// create descriptor pool
	{
		const VkDescriptorPoolSize		sizes[] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 }
		};

		VkDescriptorPoolCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.maxSets		= 100;
		info.poolSizeCount	= uint(CountOf( sizes ));
		info.pPoolSizes		= sizes;

		VK_CHECK( vulkan.vkCreateDescriptorPool( dev, &info, null, OUT &helper.descPool ));
	}
	
	// debug output buffer
	{
		helper.debugOutputSize = Min( helper.debugOutputSize, BytesU{vulkan.GetDeviceProperties().limits.maxStorageBufferRange} );
		FG_LOGI( "Shader debug output storage buffer size: "s + ToString(helper.debugOutputSize) );

		VkBufferCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags			= 0;
		info.size			= VkDeviceSize(helper.debugOutputSize);
		info.usage			= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vulkan.vkCreateBuffer( dev, &info, null, OUT &helper.debugOutputBuf ));
		
		VkMemoryRequirements	mem_req;
		vulkan.vkGetBufferMemoryRequirements( dev, helper.debugOutputBuf, OUT &mem_req );
		
		// allocate device local memory
		VkMemoryAllocateInfo	alloc_info = {};
		alloc_info.sType			= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize	= mem_req.size;
		CHECK_ERR( vulkan.GetMemoryTypeIndex( mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, OUT alloc_info.memoryTypeIndex ));

		VK_CHECK( vulkan.vkAllocateMemory( dev, &alloc_info, null, OUT &helper.debugOutputMem ));
		VK_CHECK( vulkan.vkBindBufferMemory( dev, helper.debugOutputBuf, helper.debugOutputMem, 0 ));
	}

	// debug output read back buffer
	{
		VkBufferCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags			= 0;
		info.size			= VkDeviceSize(helper.debugOutputSize);
		info.usage			= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vulkan.vkCreateBuffer( dev, &info, null, OUT &helper.readBackBuf ));
		
		VkMemoryRequirements	mem_req;
		vulkan.vkGetBufferMemoryRequirements( dev, helper.readBackBuf, OUT &mem_req );
		
		// allocate host visible memory
		VkMemoryAllocateInfo	alloc_info = {};
		alloc_info.sType			= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize	= mem_req.size;
		CHECK_ERR( vulkan.GetMemoryTypeIndex( mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
											  OUT alloc_info.memoryTypeIndex ));

		VK_CHECK( vulkan.vkAllocateMemory( dev, &alloc_info, null, OUT &helper.readBackMem ));
		VK_CHECK( vulkan.vkMapMemory( dev, helper.readBackMem, 0, info.size, 0, OUT &helper.readBackPtr ));

		VK_CHECK( vulkan.vkBindBufferMemory( dev, helper.readBackBuf, helper.readBackMem, 0 ));
	}

	// run tests
	{
		CHECK_FATAL( GLSLShaderTrace_Test1( vulkan, helper ));
		CHECK_FATAL( GLSLShaderTrace_Test2( vulkan, helper ));
	}

	// destroy all
	{
		vulkan.vkDestroyCommandPool( dev, helper.cmdPool, null );
		vulkan.vkDestroyDescriptorPool( dev, helper.descPool, null );
		vulkan.vkDestroyBuffer( dev, helper.debugOutputBuf, null );
		vulkan.vkDestroyBuffer( dev, helper.readBackBuf, null );
		vulkan.vkFreeMemory( dev, helper.debugOutputMem, null );
		vulkan.vkFreeMemory( dev, helper.readBackMem, null );

		helper = {};
	}

	return 0;
}

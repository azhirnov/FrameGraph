// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ShaderTraceTestUtils.h"

extern bool ShaderTrace_Test1 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test2 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test3 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test4 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test5 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test6 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test7 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test8 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test9 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test10 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test11 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test12 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test13 (VulkanDeviceExt& vulkan, const TestHelpers &helper);
extern bool ShaderTrace_Test14 (VulkanDeviceExt& vulkan, const TestHelpers &helper);


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
		CHECK_ERR( vulkan.Create( "GLSL debugger unit tests", "",
								  VK_API_VERSION_1_1,
								  "",
								  {{ VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0.0f }},
								  VulkanDevice::GetRecomendedInstanceLayers(),
								  VulkanDevice::GetRecomendedInstanceExtensions(),
								  VulkanDevice::GetAllDeviceExtensions()
			));
		vulkan.CreateDebugUtilsCallback( DebugUtilsMessageSeverity_All );
	}

	TestHelpers		helper;
	VkDevice		dev	= vulkan.GetVkDevice();
	
	helper.queue		= vulkan.GetVkQueues().front().handle;
	helper.queueFamily	= vulkan.GetVkQueues().front().familyIndex;

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
		VkDescriptorPoolSize	sizes[] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 100 }
		};
		
		if ( vulkan.GetDeviceRayTracingProperties().shaderGroupHandleSize == 0 )
		{
			// if ray-tracing is not supported then change descriptor type for something else
			sizes[3].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		}

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
	bool	passed = true;
	{
		passed &= ShaderTrace_Test1( vulkan, helper );
		passed &= ShaderTrace_Test2( vulkan, helper );
		//passed &= ShaderTrace_Test3( vulkan, helper );
		passed &= ShaderTrace_Test4( vulkan, helper );
		passed &= ShaderTrace_Test5( vulkan, helper );
		passed &= ShaderTrace_Test6( vulkan, helper );
		passed &= ShaderTrace_Test7( vulkan, helper );
		passed &= ShaderTrace_Test8( vulkan, helper );
		passed &= ShaderTrace_Test9( vulkan, helper );
		passed &= ShaderTrace_Test10( vulkan, helper );
		passed &= ShaderTrace_Test11( vulkan, helper );
		passed &= ShaderTrace_Test12( vulkan, helper );
		passed &= ShaderTrace_Test13( vulkan, helper );
		passed &= ShaderTrace_Test14( vulkan, helper );
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

	CHECK_FATAL( passed );
	return 0;
}

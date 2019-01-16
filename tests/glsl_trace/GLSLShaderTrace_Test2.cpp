// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "GLSLShaderTraceTestUtils.h"

/*
=================================================
	CompileShaders
=================================================
*/
static bool CompileShaders (VulkanDevice &vulkan, ShaderCompiler &shaderCompiler, OUT VkShaderModule &compShader)
{
	// create vertex shader
	{
		static const char	comp_shader_source[] = R"#(
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) writeonly uniform image2D  un_OutImage;

void Test1 (out vec4 color)
{
	vec2	point = (vec3(gl_GlobalInvocationID) / vec3(gl_NumWorkGroups * gl_WorkGroupSize - 1)).xy;
	
	if ( (gl_GlobalInvocationID.x & 1) == 1 )
	{
		color = vec4(1.0f);
		return;
	}

	color = point.xyyx;
	color.x = cos(color.y) * color.z;
}

void main()
{
	vec4	color;
	Test1( color );
	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), color );
}
)#";
		CHECK_ERR( shaderCompiler.Compile( OUT compShader, vulkan, {comp_shader_source}, EShLangCompute, 1 ));
	}
	return true;
}

/*
=================================================
	CreatePipeline
=================================================
*/
static bool CreatePipeline (VulkanDevice &vulkan, VkShaderModule compShader, ArrayView<VkDescriptorSetLayout> dsLayouts,
							OUT VkPipelineLayout &outPipelineLayout, OUT VkPipeline &outPipeline)
{
	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= uint(dsLayouts.size());
		info.pSetLayouts			= dsLayouts.data();
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vulkan.vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &outPipelineLayout ));
	}

	VkComputePipelineCreateInfo		info = {};
	info.sType			= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.stage.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
	info.stage.module	= compShader;
	info.stage.pName	= "main";
	info.layout			= outPipelineLayout;

	VK_CHECK( vulkan.vkCreateComputePipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &outPipeline ));
	return true;
}

/*
=================================================
	GLSLShaderTrace_Test2
=================================================
*/
extern bool GLSLShaderTrace_Test2 (VulkanDeviceExt& vulkan, const TestHelpers &helper)
{	
	// create image
	VkImage			image;
	VkImageView		image_view;
	VkDeviceMemory	image_mem;
	uint			width = 16, height = 16;
	{
		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.flags			= 0;
		info.imageType		= VK_IMAGE_TYPE_2D;
		info.format			= VK_FORMAT_R8G8B8A8_UNORM;
		info.extent			= { width, height, 1 };
		info.mipLevels		= 1;
		info.arrayLayers	= 1;
		info.samples		= VK_SAMPLE_COUNT_1_BIT;
		info.tiling			= VK_IMAGE_TILING_OPTIMAL;
		info.usage			= VK_IMAGE_USAGE_STORAGE_BIT;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;

		VK_CHECK( vulkan.vkCreateImage( vulkan.GetVkDevice(), &info, null, OUT &image ));

		VkMemoryRequirements	mem_req;
		vulkan.vkGetImageMemoryRequirements( vulkan.GetVkDevice(), image, OUT &mem_req );
		
		// allocate device local memory
		VkMemoryAllocateInfo	alloc_info = {};
		alloc_info.sType			= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize	= mem_req.size;
		CHECK_ERR( vulkan.GetMemoryTypeIndex( mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, OUT alloc_info.memoryTypeIndex ));

		VK_CHECK( vulkan.vkAllocateMemory( vulkan.GetVkDevice(), &alloc_info, null, OUT &image_mem ));
		VK_CHECK( vulkan.vkBindImageMemory( vulkan.GetVkDevice(), image, image_mem, 0 ));
	}

	// create image view
	{
		VkImageViewCreateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.flags				= 0;
		info.image				= image;
		info.viewType			= VK_IMAGE_VIEW_TYPE_2D;
		info.format				= VK_FORMAT_R8G8B8A8_UNORM;
		info.components			= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		info.subresourceRange	= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		VK_CHECK( vulkan.vkCreateImageView( vulkan.GetVkDevice(), &info, null, OUT &image_view ));
	}


	// create pipeline
	ShaderCompiler	shader_compiler;
	VkShaderModule	comp_shader;
	CHECK_ERR( CompileShaders( vulkan, shader_compiler, OUT comp_shader ));

	VkDescriptorSetLayout	ds1_layout, ds2_layout;
	VkDescriptorSet			desc_set1, desc_set2;
	CHECK_ERR( CreateDebugDescriptorSet( vulkan, helper, VK_SHADER_STAGE_COMPUTE_BIT, OUT ds2_layout, OUT desc_set2 ));

	// create layout
	{
		VkDescriptorSetLayoutBinding	binding = {};
		binding.binding			= 0;
		binding.descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding.descriptorCount	= 1;
		binding.stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= 1;
		info.pBindings		= &binding;

		VK_CHECK( vulkan.vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &ds1_layout ));
	}
	
	// allocate descriptor set
	{
		VkDescriptorSetAllocateInfo		info = {};
		info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool		= helper.descPool;
		info.descriptorSetCount	= 1;
		info.pSetLayouts		= &ds1_layout;

		VK_CHECK( vulkan.vkAllocateDescriptorSets( vulkan.GetVkDevice(), &info, OUT &desc_set1 ));
	}

	// update descriptor set
	{
		VkDescriptorImageInfo	desc_image	= {};
		desc_image.imageView	= image_view;
		desc_image.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		
		VkWriteDescriptorSet	write	= {};
		write.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet			= desc_set1;
		write.dstBinding		= 0;
		write.descriptorCount	= 1;
		write.descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		write.pImageInfo		= &desc_image;
		
		vulkan.vkUpdateDescriptorSets( vulkan.GetVkDevice(), 1, &write, 0, null );
	}

	VkPipelineLayout	ppln_layout;
	VkPipeline			pipeline;
	CHECK_ERR( CreatePipeline( vulkan, comp_shader, {ds1_layout, ds2_layout}, OUT ppln_layout, OUT pipeline ));
	
	
	// build command buffer
	VkCommandBufferBeginInfo	begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, null, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, null };
	VK_CHECK( vulkan.vkBeginCommandBuffer( helper.cmdBuffer, &begin ));

	// image layout undefined -> general
	{
		VkImageMemoryBarrier	barrier = {};
		barrier.sType			= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask	= 0;
		barrier.dstAccessMask	= VK_ACCESS_SHADER_WRITE_BIT;
		barrier.oldLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout		= VK_IMAGE_LAYOUT_GENERAL;
		barrier.image			= image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange	= {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

		vulkan.vkCmdPipelineBarrier( helper.cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
									 0, null, 0, null, 1, &barrier);
	}
	
	// setup storage buffer
	{
		const uint	data[] = { 
			width/2, height/2, 0,		// selected invocation
		};

		vulkan.vkCmdFillBuffer( helper.cmdBuffer, helper.debugOutputBuf, sizeof(data), VK_WHOLE_SIZE, 0 );
		vulkan.vkCmdUpdateBuffer( helper.cmdBuffer, helper.debugOutputBuf, 0, sizeof(data), data );
	}
	
	// debug output storage read/write after write
	{
		VkBufferMemoryBarrier	barrier = {};
		barrier.sType			= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask	= VK_ACCESS_SHADER_WRITE_BIT;
		barrier.buffer			= helper.debugOutputBuf;
		barrier.offset			= 0;
		barrier.size			= VK_WHOLE_SIZE;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		
		vulkan.vkCmdPipelineBarrier( helper.cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
									 0, null, 1, &barrier, 0, null);
	}
	
	vulkan.vkCmdBindPipeline( helper.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline );
	vulkan.vkCmdBindDescriptorSets( helper.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ppln_layout, 0, 1, &desc_set1, 0, null );
	vulkan.vkCmdBindDescriptorSets( helper.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, ppln_layout, 1, 1, &desc_set2, 0, null );
	
	vulkan.vkCmdDispatch( helper.cmdBuffer, width, height, 1 );

	// debug output storage read after write
	{
		VkBufferMemoryBarrier	barrier = {};
		barrier.sType			= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcAccessMask	= VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask	= VK_ACCESS_TRANSFER_READ_BIT;
		barrier.buffer			= helper.debugOutputBuf;
		barrier.offset			= 0;
		barrier.size			= VK_WHOLE_SIZE;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		
		vulkan.vkCmdPipelineBarrier( helper.cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
									 0, null, 1, &barrier, 0, null);
	}
	
	// copy shader debug output into host visible memory
	{
		VkBufferCopy	region = {};
		region.srcOffset	= 0;
		region.dstOffset	= 0;
		region.size			= VkDeviceSize(helper.debugOutputSize);

		vulkan.vkCmdCopyBuffer( helper.cmdBuffer, helper.debugOutputBuf, helper.readBackBuf, 1, &region );
	}

	VK_CHECK( vulkan.vkEndCommandBuffer( helper.cmdBuffer ));
	
	// submit commands and wait
	{
		VkSubmitInfo	submit = {};
		submit.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount	= 1;
		submit.pCommandBuffers		= &helper.cmdBuffer;

		VK_CHECK( vulkan.vkQueueSubmit( helper.queue, 1, &submit, VK_NULL_HANDLE ));
		VK_CHECK( vulkan.vkQueueWaitIdle( helper.queue ));
	}

	// destroy all
	{
		vulkan.vkDestroyDescriptorSetLayout( vulkan.GetVkDevice(), ds1_layout, null );
		vulkan.vkDestroyDescriptorSetLayout( vulkan.GetVkDevice(), ds2_layout, null );
		vulkan.vkDestroyShaderModule( vulkan.GetVkDevice(), comp_shader, null );
		vulkan.vkDestroyPipelineLayout( vulkan.GetVkDevice(), ppln_layout, null );
		vulkan.vkDestroyPipeline( vulkan.GetVkDevice(), pipeline, null );
		vulkan.vkDestroyImageView( vulkan.GetVkDevice(), image_view, null );
		vulkan.vkDestroyImage( vulkan.GetVkDevice(), image, null );
		vulkan.vkFreeMemory( vulkan.GetVkDevice(), image_mem, null );
	}

	// process debug output
	Array<String>	debug_output;
	CHECK_ERR( shader_compiler.GetDebugOutput( comp_shader, helper.readBackPtr, helper.debugOutputSize, OUT debug_output ));
	
	static const char	ref[] = R"#(//> gl_GlobalInvocationID: uint3 {8, 8, 0}
no source

//> gl_LocalInvocationID: uint3 {0, 0, 0}
no source

//> gl_WorkGroupID: uint3 {8, 8, 0}
no source

//> Test1(): void
23. 	Test1( color );

//> point: float2 {0.533333, 0.533333}
//  gl_GlobalInvocationID: uint3 {8, 8, 0}
8. point = (vec3(gl_GlobalInvocationID) / vec3(gl_NumWorkGroups * gl_WorkGroupSize - 1)).xy;

//> (out): bool {false}
//  gl_GlobalInvocationID: uint3 {8, 8, 0}
10. if ( (gl_GlobalInvocationID.x & 1) == 1 )

//> color: float4 {0.533333, 0.533333, 0.533333, 0.533333}
//  point: float2 {0.533333, 0.533333}
16. color = point.xyyx;

//> cos(): float {0.861117}
//  color: float4 {0.533333, 0.533333, 0.533333, 0.533333}
17. 	color.x = cos(color.y) * color.z;

//> color: float4 {0.459263, 0.533333, 0.533333, 0.533333}
//  cos(): float {0.861117}
17. color.x = cos(color.y) * color.z;

//> imageStore(): void
//  gl_GlobalInvocationID: uint3 {8, 8, 0}
24. 	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), color );

)#";

	CHECK_ERR( debug_output.size() == 1 );
	CHECK_ERR( debug_output[0] == ref );

	FG_LOGI( TEST_NAME << " - passed" );
	return true;
}

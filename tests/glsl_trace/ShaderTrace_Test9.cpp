// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ShaderTraceTestUtils.h"

enum {
	RAYGEN_SHADER,
	HIT_SHADER,
	MISS_SHADER,
	NUM_GROUPS
};

/*
=================================================
	CompileShaders
=================================================
*/
static bool CompileShaders (VulkanDevice &vulkan, ShaderCompiler &shaderCompiler,
							OUT VkShaderModule &rayGenShader, OUT VkShaderModule &rayMissShader, OUT VkShaderModule &rayClosestHitShader)
{
	static const char	rt_shader[] = R"#(
#extension GL_NV_ray_tracing : require
#define PAYLOAD_LOC 0
)#";

	// create ray generation shader
	{
		static const char	raygen_shader_source[] = R"#(
layout(binding = 0) uniform accelerationStructureNV  un_RtScene;
layout(binding = 1, rgba8) writeonly restrict uniform image2D  un_Output;
layout(location = PAYLOAD_LOC) rayPayloadNV vec4  payload;

void main ()
{
	const vec2 uv = vec2(gl_LaunchIDNV.xy) / vec2(gl_LaunchSizeNV.xy - 1);
	const vec3 origin = vec3(uv.x, 1.0f - uv.y, -1.0f);
	const vec3 direction = vec3(0.0f, 0.0f, 1.0f);

	traceNV( /*topLevel*/un_RtScene, /*rayFlags*/gl_RayFlagsNoneNV, /*cullMask*/0xFF,
			  /*sbtRecordOffset*/0, /*sbtRecordStride*/0, /*missIndex*/0,
			  /*origin*/origin, /*Tmin*/0.0f,
			  /*direction*/direction, /*Tmax*/10.0f,
			  /*payload*/PAYLOAD_LOC );

	imageStore( un_Output, ivec2(gl_LaunchIDNV), payload );
}
)#";
		CHECK_ERR( shaderCompiler.Compile( OUT rayGenShader, vulkan, {rt_shader, raygen_shader_source}, EShLangRayGenNV, 1 ));
	}

	// create ray miss shader
	{
		static const char	raymiss_shader_source[] = R"#(
layout(location = PAYLOAD_LOC) rayPayloadInNV vec4  payload;

void main ()
{
	payload = vec4( 0.412f, 0.796f, 1.0f, 1.0f );
}
)#";
		CHECK_ERR( shaderCompiler.Compile( OUT rayMissShader, vulkan, {rt_shader, raymiss_shader_source}, EShLangMissNV ));
	}

	// create ray closest hit shader
	{
		static const char	closesthit_shader_source[] = R"#(
layout(location = PAYLOAD_LOC) rayPayloadInNV vec4  payload;
hitAttributeNV vec2  HitAttribs;

void main ()
{
	const vec3 barycentrics = vec3(1.0f - HitAttribs.x - HitAttribs.y, HitAttribs.x, HitAttribs.y);
	payload = vec4(barycentrics, 1.0);
}
)#";
		CHECK_ERR( shaderCompiler.Compile( OUT rayClosestHitShader, vulkan, {rt_shader, closesthit_shader_source}, EShLangClosestHitNV ));
	}
	return true;
}

/*
=================================================
	CreatePipeline
=================================================
*/
static bool CreatePipeline (VulkanDevice &vulkan, VkShaderModule rayGenShader, VkShaderModule rayMissShader, VkShaderModule rayClosestHitShader,
							ArrayView<VkDescriptorSetLayout> dsLayouts, OUT VkPipelineLayout &outPipelineLayout, OUT VkPipeline &outPipeline)
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
	

	VkPipelineShaderStageCreateInfo		stages [NUM_GROUPS] = {};

	stages[RAYGEN_SHADER].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[RAYGEN_SHADER].stage		= VK_SHADER_STAGE_RAYGEN_BIT_NV;
	stages[RAYGEN_SHADER].module	= rayGenShader;
	stages[RAYGEN_SHADER].pName		= "main";

	stages[MISS_SHADER].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[MISS_SHADER].stage		= VK_SHADER_STAGE_MISS_BIT_NV;
	stages[MISS_SHADER].module		= rayMissShader;
	stages[MISS_SHADER].pName		= "main";

	stages[HIT_SHADER].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[HIT_SHADER].stage		= VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
	stages[HIT_SHADER].module		= rayClosestHitShader;
	stages[HIT_SHADER].pName		= "main";


	VkRayTracingShaderGroupCreateInfoNV	shader_groups [NUM_GROUPS] = {};

	shader_groups[RAYGEN_SHADER].sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	shader_groups[RAYGEN_SHADER].type				= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	shader_groups[RAYGEN_SHADER].generalShader		= RAYGEN_SHADER;
	shader_groups[RAYGEN_SHADER].closestHitShader	= VK_SHADER_UNUSED_NV;
	shader_groups[RAYGEN_SHADER].anyHitShader		= VK_SHADER_UNUSED_NV;
	shader_groups[RAYGEN_SHADER].intersectionShader	= VK_SHADER_UNUSED_NV;
	
	shader_groups[HIT_SHADER].sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	shader_groups[HIT_SHADER].type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	shader_groups[HIT_SHADER].generalShader			= VK_SHADER_UNUSED_NV;
	shader_groups[HIT_SHADER].closestHitShader		= HIT_SHADER;
	shader_groups[HIT_SHADER].anyHitShader			= VK_SHADER_UNUSED_NV;
	shader_groups[HIT_SHADER].intersectionShader	= VK_SHADER_UNUSED_NV;
	
	shader_groups[MISS_SHADER].sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	shader_groups[MISS_SHADER].type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	shader_groups[MISS_SHADER].generalShader		= MISS_SHADER;
	shader_groups[MISS_SHADER].closestHitShader		= VK_SHADER_UNUSED_NV;
	shader_groups[MISS_SHADER].anyHitShader			= VK_SHADER_UNUSED_NV;
	shader_groups[MISS_SHADER].intersectionShader	= VK_SHADER_UNUSED_NV;


	// create pipeline
	VkRayTracingPipelineCreateInfoNV 	info = {};
	info.sType				= VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
	info.flags				= 0;
	info.stageCount			= uint(CountOf( stages ));
	info.pStages			= stages;
	info.pGroups			= shader_groups;
	info.groupCount			= uint(CountOf( shader_groups ));
	info.maxRecursionDepth	= 0;
	info.layout				= outPipelineLayout;

	VK_CHECK( vulkan.vkCreateRayTracingPipelinesNV( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &outPipeline ));
	return true;
}

/*
=================================================
	ShaderTrace_Test9
=================================================
*/
extern bool ShaderTrace_Test9 (VulkanDeviceExt& vulkan, const TestHelpers &helper)
{
	if ( vulkan.GetDeviceRayTracingProperties().shaderGroupHandleSize == 0 )
		return true;	// not supported

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
	VkShaderModule	raygen_shader, miss_shader, hit_shader;
	CHECK_ERR( CompileShaders( vulkan, helper.compiler, OUT raygen_shader, OUT miss_shader, OUT hit_shader ));

	VkDescriptorSetLayout	ds1_layout, ds2_layout;
	VkDescriptorSet			desc_set1, desc_set2;
	CHECK_ERR( CreateDebugDescriptorSet( vulkan, helper, VK_SHADER_STAGE_RAYGEN_BIT_NV, OUT ds2_layout, OUT desc_set2 ));

	// create layout
	{
		VkDescriptorSetLayoutBinding		binding[2] = {};
		binding[0].binding			= 0;
		binding[0].descriptorType	= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		binding[0].descriptorCount	= 1;
		binding[0].stageFlags		= VK_SHADER_STAGE_RAYGEN_BIT_NV;

		binding[1].binding			= 1;
		binding[1].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding[1].descriptorCount	= 1;
		binding[1].stageFlags		= VK_SHADER_STAGE_RAYGEN_BIT_NV;

		VkDescriptorSetLayoutCreateInfo		info = {};
		info.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount	= uint(CountOf( binding ));
		info.pBindings		= binding;

		VK_CHECK( vulkan.vkCreateDescriptorSetLayout( vulkan.GetVkDevice(), &info, null, OUT &ds1_layout ));
	}

	// create pipeline
	VkPipelineLayout	ppln_layout;
	VkPipeline			pipeline;
	CHECK_ERR( CreatePipeline( vulkan, raygen_shader, miss_shader, hit_shader, {ds1_layout, ds2_layout}, OUT ppln_layout, OUT pipeline ));
	
	// create BLAS and TLAS
	VkBuffer					shader_binding;
	VkDeviceMemory				dev_memory;
	VkAccelerationStructureNV	top_level_as;
	VkAccelerationStructureNV	bottom_level_as;
	CHECK_ERR( CreateRayTracingScene( vulkan, helper, pipeline, NUM_GROUPS, OUT shader_binding, OUT dev_memory, OUT top_level_as, OUT bottom_level_as ));
	
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
		VkWriteDescriptorSet	writes[2] = {};

		// un_Output
		VkDescriptorImageInfo	desc_image = {};
		desc_image.imageView	= image_view;
		desc_image.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
		
		writes[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet			= desc_set1;
		writes[0].dstBinding		= 1;
		writes[0].descriptorCount	= 1;
		writes[0].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[0].pImageInfo		= &desc_image;
		
		// un_RtScene
		VkWriteDescriptorSetAccelerationStructureNV 	tlas_desc = {};
		tlas_desc.sType							= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
		tlas_desc.accelerationStructureCount	= 1;
		tlas_desc.pAccelerationStructures		= &top_level_as;

		writes[1].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].pNext				= &tlas_desc;
		writes[1].dstSet			= desc_set1;
		writes[1].dstBinding		= 0;
		writes[1].descriptorCount	= 1;
		writes[1].descriptorType	= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

		vulkan.vkUpdateDescriptorSets( vulkan.GetVkDevice(), uint(CountOf( writes )), writes, 0, null );
	}

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

		vulkan.vkCmdPipelineBarrier( helper.cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, 0,
									 0, null, 0, null, 1, &barrier);
	}
	
	// setup storage buffer
	{
		const uint	data[] = { width/2, height/2, 0 };

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
		
		vulkan.vkCmdPipelineBarrier( helper.cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, 0,
									 0, null, 1, &barrier, 0, null);
	}
	
	// trace rays
	{
		vulkan.vkCmdBindPipeline( helper.cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipeline );
		vulkan.vkCmdBindDescriptorSets( helper.cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, ppln_layout, 0, 1, &desc_set1, 0, null );
		vulkan.vkCmdBindDescriptorSets( helper.cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, ppln_layout, 1, 1, &desc_set2, 0, null );

		VkDeviceSize	stride = vulkan.GetDeviceRayTracingProperties().shaderGroupHandleSize;

		vulkan.vkCmdTraceRaysNV( helper.cmdBuffer, 
								 shader_binding, RAYGEN_SHADER * stride,
								 shader_binding, MISS_SHADER * stride, stride,
								 shader_binding, HIT_SHADER * stride, stride,
								 VK_NULL_HANDLE, 0, 0,
								 width, height, 1 );
	}

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
		
		vulkan.vkCmdPipelineBarrier( helper.cmdBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
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
		vulkan.vkDestroyShaderModule( vulkan.GetVkDevice(), raygen_shader, null );
		vulkan.vkDestroyShaderModule( vulkan.GetVkDevice(), miss_shader, null );
		vulkan.vkDestroyShaderModule( vulkan.GetVkDevice(), hit_shader, null );
		vulkan.vkDestroyPipelineLayout( vulkan.GetVkDevice(), ppln_layout, null );
		vulkan.vkDestroyPipeline( vulkan.GetVkDevice(), pipeline, null );
		vulkan.vkDestroyImageView( vulkan.GetVkDevice(), image_view, null );
		vulkan.vkDestroyImage( vulkan.GetVkDevice(), image, null );
		vulkan.vkFreeMemory( vulkan.GetVkDevice(), image_mem, null );
		vulkan.vkDestroyAccelerationStructureNV( vulkan.GetVkDevice(), top_level_as, null );
		vulkan.vkDestroyAccelerationStructureNV( vulkan.GetVkDevice(), bottom_level_as, null );
		vulkan.vkDestroyBuffer( vulkan.GetVkDevice(), shader_binding, null );
		vulkan.vkFreeMemory( vulkan.GetVkDevice(), dev_memory, null );
	}

	CHECK_ERR( TestDebugOutput( helper, raygen_shader, TEST_NAME + ".txt" ));

	FG_LOGI( TEST_NAME << " - passed" );
	return true;
}

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ShaderTraceTestUtils.h"
#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Stream/FileStream.h"

static const bool	UpdateReferences = true;

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
	CreateGraphicsPipelineVar1
=================================================
*/
bool CreateGraphicsPipelineVar1 (VulkanDevice &vulkan, VkShaderModule vertShader, VkShaderModule fragShader,
								 VkDescriptorSetLayout dsLayout, VkRenderPass renderPass,
								 OUT VkPipelineLayout &outPipelineLayout, OUT VkPipeline &outPipeline)
{
	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &dsLayout;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vulkan.vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &outPipelineLayout ));
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
	info.layout					= outPipelineLayout;
	info.renderPass				= renderPass;
	info.subpass				= 0;

	VK_CHECK( vulkan.vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &outPipeline ));
	return true;
}

/*
=================================================
	CreateMeshPipelineVar1
=================================================
*/
bool CreateGraphicsPipelineVar2 (VulkanDevice &vulkan, VkShaderModule vertShader, VkShaderModule tessContShader, VkShaderModule tessEvalShader,
								 VkShaderModule fragShader, VkDescriptorSetLayout dsLayout, VkRenderPass renderPass, uint patchSize,
								 OUT VkPipelineLayout &outPipelineLayout, OUT VkPipeline &outPipeline)
{
	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &dsLayout;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vulkan.vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &outPipelineLayout ));
	}

	VkPipelineShaderStageCreateInfo			stages[4] = {};
	stages[0].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage		= VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module	= vertShader;
	stages[0].pName		= "main";
	stages[1].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage		= VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module	= fragShader;
	stages[1].pName		= "main";
	stages[2].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[2].stage		= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	stages[2].module	= tessContShader;
	stages[2].pName		= "main";
	stages[3].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[3].stage		= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	stages[3].module	= tessEvalShader;
	stages[3].pName		= "main";

	VkPipelineVertexInputStateCreateInfo	vertex_input = {};
	vertex_input.sType		= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	VkPipelineInputAssemblyStateCreateInfo	input_assembly = {};
	input_assembly.sType	= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology	= VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

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

	VkPipelineTessellationStateCreateInfo	tess_state = {};
	tess_state.sType				= VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tess_state.patchControlPoints	= patchSize;

	// create pipeline
	VkGraphicsPipelineCreateInfo	info = {};
	info.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.stageCount				= uint(CountOf( stages ));
	info.pStages				= stages;
	info.pViewportState			= &viewport;
	info.pVertexInputState		= &vertex_input;
	info.pInputAssemblyState	= &input_assembly;
	info.pTessellationState		= &tess_state;
	info.pRasterizationState	= &rasterization;
	info.pMultisampleState		= &multisample;
	info.pDepthStencilState		= &depth_stencil;
	info.pColorBlendState		= &blend_state;
	info.pDynamicState			= &dynamic_state;
	info.layout					= outPipelineLayout;
	info.renderPass				= renderPass;
	info.subpass				= 0;

	VK_CHECK( vulkan.vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &outPipeline ));
	return true;
}

/*
=================================================
	CreateMeshPipelineVar1
=================================================
*/
bool CreateMeshPipelineVar1 (VulkanDevice &vulkan, VkShaderModule meshShader, VkShaderModule fragShader,
							 VkDescriptorSetLayout dsLayout, VkRenderPass renderPass,
							 OUT VkPipelineLayout &outPipelineLayout, OUT VkPipeline &outPipeline)
{
	// create pipeline layout
	{
		VkPipelineLayoutCreateInfo	info = {};
		info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount			= 1;
		info.pSetLayouts			= &dsLayout;
		info.pushConstantRangeCount	= 0;
		info.pPushConstantRanges	= null;

		VK_CHECK( vulkan.vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &outPipelineLayout ));
	}

	VkPipelineShaderStageCreateInfo			stages[2] = {};
	stages[0].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage		= VK_SHADER_STAGE_MESH_BIT_NV;
	stages[0].module	= meshShader;
	stages[0].pName		= "main";
	stages[1].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage		= VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module	= fragShader;
	stages[1].pName		= "main";

	VkPipelineVertexInputStateCreateInfo	vertex_input = {};
	vertex_input.sType		= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	VkPipelineInputAssemblyStateCreateInfo	input_assembly = {};
	input_assembly.sType	= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology	= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

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
	info.pVertexInputState		= &vertex_input;		// ignored in mesh shader
	info.pInputAssemblyState	= &input_assembly;
	info.pRasterizationState	= &rasterization;
	info.pMultisampleState		= &multisample;
	info.pDepthStencilState		= &depth_stencil;
	info.pColorBlendState		= &blend_state;
	info.pDynamicState			= &dynamic_state;
	info.layout					= outPipelineLayout;
	info.renderPass				= renderPass;
	info.subpass				= 0;

	VK_CHECK( vulkan.vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &outPipeline ));
	return true;
}

/*
=================================================
	CreateRayTracingScene
=================================================
*/
bool CreateRayTracingScene (VulkanDeviceExt &vulkan, const TestHelpers &helper, VkPipeline rtPipeline, uint numGroups,
							OUT VkBuffer &shaderBindingTable, OUT VkDeviceMemory &outMemory,
							OUT VkAccelerationStructureNV &topLevelAS, OUT VkAccelerationStructureNV &bottomLevelAS)
{
	struct VkGeometryInstance
	{
		// 4x3 row-major matrix
		float4		transformRow0;
		float4		transformRow1;
		float4		transformRow2;

		uint		instanceId		: 24;
		uint		mask			: 8;
		uint		instanceOffset	: 24;
		uint		flags			: 8;
		uint64_t	accelerationStructureHandle;
	};

	struct MemInfo
	{
		VkDeviceSize			totalSize		= 0;
		uint					memTypeBits		= 0;
		VkMemoryPropertyFlags	memProperty		= 0;
	};

	struct ResourceInit
	{
		using BindMemCallbacks_t	= Array< std::function<bool (void *)> >;
		using DrawCallbacks_t		= Array< std::function<void (VkCommandBuffer)> >;

		MemInfo					host;
		MemInfo					dev;
		BindMemCallbacks_t		onBind;
		DrawCallbacks_t			onDraw;
	};

	static const float3		vertices[] = {
		{ 0.25f, 0.25f, 0.0f },
		{ 0.75f, 0.25f, 0.0f },
		{ 0.50f, 0.75f, 0.0f }
	};
	static const uint		indices[] = {
		0, 1, 2
	};

	VkBuffer		vertex_buffer;
	VkBuffer		index_buffer;
	VkBuffer		instance_buffer;
	VkBuffer		scratch_buffer;
	VkDeviceMemory	host_memory;
	uint64_t		bottom_level_as_handle = 0;

	ResourceInit	res;
	res.dev.memProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	res.host.memProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	// create vertex buffer
	{
		VkBufferCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags			= 0;
		info.size			= sizeof(vertices);
		info.usage			= VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vulkan.vkCreateBuffer( vulkan.GetVkDevice(), &info, null, OUT &vertex_buffer ));
		
		VkMemoryRequirements	mem_req;
		vulkan.vkGetBufferMemoryRequirements( vulkan.GetVkDevice(), vertex_buffer, OUT &mem_req );
		
		VkDeviceSize	offset = AlignToLarger( res.host.totalSize, mem_req.alignment );
		res.host.totalSize		 = offset + mem_req.size;
		res.host.memTypeBits	|= mem_req.memoryTypeBits;

		res.onBind.push_back( [&vulkan, &host_memory, vertex_buffer, offset] (void *ptr) -> bool
		{
			memcpy( ptr + BytesU(offset), vertices, sizeof(vertices) );
			VK_CHECK( vulkan.vkBindBufferMemory( vulkan.GetVkDevice(), vertex_buffer, host_memory, offset ));
			return true;
		});
	}

	// create index buffer
	{
		VkBufferCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags			= 0;
		info.size			= sizeof(indices);
		info.usage			= VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vulkan.vkCreateBuffer( vulkan.GetVkDevice(), &info, null, OUT &index_buffer ));
		
		VkMemoryRequirements	mem_req;
		vulkan.vkGetBufferMemoryRequirements( vulkan.GetVkDevice(), index_buffer, OUT &mem_req );
		
		VkDeviceSize	offset = AlignToLarger( res.host.totalSize, mem_req.alignment );
		res.host.totalSize		 = offset + mem_req.size;
		res.host.memTypeBits	|= mem_req.memoryTypeBits;

		res.onBind.push_back( [&vulkan, index_buffer, &host_memory, offset] (void *ptr) -> bool
		{
			memcpy( ptr + BytesU(offset), indices, sizeof(indices) );
			VK_CHECK( vulkan.vkBindBufferMemory( vulkan.GetVkDevice(), index_buffer, host_memory, offset ));
			return true;
		});
	}

	// create bottom level acceleration structure
	{
		VkGeometryNV	geometry[1] = {};
		geometry[0].sType			= VK_STRUCTURE_TYPE_GEOMETRY_NV;
		geometry[0].geometryType	= VK_GEOMETRY_TYPE_TRIANGLES_NV;
		geometry[0].flags			= VK_GEOMETRY_OPAQUE_BIT_NV;
		geometry[0].geometry.aabbs.sType	= VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
		geometry[0].geometry.triangles.sType		= VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
		geometry[0].geometry.triangles.vertexData	= vertex_buffer;
		geometry[0].geometry.triangles.vertexOffset	= 0;
		geometry[0].geometry.triangles.vertexCount	= uint(CountOf( vertices ));
		geometry[0].geometry.triangles.vertexStride	= sizeof(vertices[0]);
		geometry[0].geometry.triangles.vertexFormat	= VK_FORMAT_R32G32B32_SFLOAT;
		geometry[0].geometry.triangles.indexData	= index_buffer;
		geometry[0].geometry.triangles.indexOffset	= 0;
		geometry[0].geometry.triangles.indexCount	= uint(CountOf( indices ));
		geometry[0].geometry.triangles.indexType	= VK_INDEX_TYPE_UINT32;

		VkAccelerationStructureCreateInfoNV	createinfo = {};
		createinfo.sType				= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		createinfo.info.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		createinfo.info.type			= VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
		createinfo.info.geometryCount	= uint(CountOf( geometry ));
		createinfo.info.pGeometries		= geometry;

		VK_CHECK( vulkan.vkCreateAccelerationStructureNV( vulkan.GetVkDevice(), &createinfo, null, OUT &bottomLevelAS ));
		
		VkAccelerationStructureMemoryRequirementsInfoNV	mem_info = {};
		mem_info.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		mem_info.accelerationStructure	= bottomLevelAS;

		VkMemoryRequirements2	mem_req = {};
		vulkan.vkGetAccelerationStructureMemoryRequirementsNV( vulkan.GetVkDevice(), &mem_info, OUT &mem_req );
		
		VkDeviceSize	offset = AlignToLarger( res.dev.totalSize, mem_req.memoryRequirements.alignment );
		res.dev.totalSize	 = offset + mem_req.memoryRequirements.size;
		res.dev.memTypeBits	|= mem_req.memoryRequirements.memoryTypeBits;
		
		res.onBind.push_back( [&vulkan, bottomLevelAS, &bottom_level_as_handle, &outMemory, offset] (void *) -> bool
		{
			VkBindAccelerationStructureMemoryInfoNV	bind_info = {};
			bind_info.sType					= VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
			bind_info.accelerationStructure	= bottomLevelAS;
			bind_info.memory				= outMemory;
			bind_info.memoryOffset			= offset;
			VK_CHECK( vulkan.vkBindAccelerationStructureMemoryNV( vulkan.GetVkDevice(), 1, &bind_info ));

			VK_CHECK( vulkan.vkGetAccelerationStructureHandleNV( vulkan.GetVkDevice(), bottomLevelAS, sizeof(bottom_level_as_handle), OUT &bottom_level_as_handle ));
			return true;
		});
		
		res.onDraw.push_back( [&vulkan, bottomLevelAS, geometry, &scratch_buffer] (VkCommandBuffer cmd)
		{
			VkAccelerationStructureInfoNV	info = {};
			info.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
			info.type			= VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
			info.geometryCount	= uint(CountOf( geometry ));
			info.pGeometries	= geometry;

			vulkan.vkCmdBuildAccelerationStructureNV( cmd, &info,
													  VK_NULL_HANDLE, 0,				// instance
													  VK_FALSE,							// update
													  bottomLevelAS, VK_NULL_HANDLE,	// dst, src
													  scratch_buffer, 0
													 );
		});
	}
	
	// create instance buffer
	{
		VkBufferCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags			= 0;
		info.size			= sizeof(VkGeometryInstance);
		info.usage			= VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vulkan.vkCreateBuffer( vulkan.GetVkDevice(), &info, null, OUT &instance_buffer ));
		
		VkMemoryRequirements	mem_req;
		vulkan.vkGetBufferMemoryRequirements( vulkan.GetVkDevice(), instance_buffer, OUT &mem_req );
		
		VkDeviceSize	offset = AlignToLarger( res.host.totalSize, mem_req.alignment );
		res.host.totalSize		 = offset + mem_req.size;
		res.host.memTypeBits	|= mem_req.memoryTypeBits;

		res.onBind.push_back( [&vulkan, &host_memory, &bottom_level_as_handle, instance_buffer, offset] (void *ptr) -> bool
		{
			VkGeometryInstance	instance = {};
			instance.transformRow0	= {1.0f, 0.0f, 0.0f, 0.0f};
			instance.transformRow1	= {0.0f, 1.0f, 0.0f, 0.0f};
			instance.transformRow2	= {0.0f, 0.0f, 1.0f, 0.0f};
			instance.instanceId		= 0;
			instance.mask			= 0xFF;
			instance.instanceOffset	= 0;
			instance.flags			= 0;
			instance.accelerationStructureHandle = bottom_level_as_handle;

			memcpy( ptr + BytesU(offset), &instance, sizeof(instance) );

			VK_CHECK( vulkan.vkBindBufferMemory( vulkan.GetVkDevice(), instance_buffer, host_memory, offset ));
			return true;
		});
	}

	// create top level acceleration structure
	{
		VkAccelerationStructureCreateInfoNV	createinfo = {};
		createinfo.sType				= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		createinfo.info.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		createinfo.info.type			= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		createinfo.info.flags			= 0;
		createinfo.info.instanceCount	= 1;

		VK_CHECK( vulkan.vkCreateAccelerationStructureNV( vulkan.GetVkDevice(), &createinfo, null, OUT &topLevelAS ));
		
		VkAccelerationStructureMemoryRequirementsInfoNV	mem_info = {};
		mem_info.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		mem_info.type					= VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
		mem_info.accelerationStructure	= topLevelAS;

		VkMemoryRequirements2	mem_req = {};
		vulkan.vkGetAccelerationStructureMemoryRequirementsNV( vulkan.GetVkDevice(), &mem_info, OUT &mem_req );

		VkDeviceSize	offset = AlignToLarger( res.dev.totalSize, mem_req.memoryRequirements.alignment );
		res.dev.totalSize	 = offset + mem_req.memoryRequirements.size;
		res.dev.memTypeBits	|= mem_req.memoryRequirements.memoryTypeBits;
		
		res.onBind.push_back( [&vulkan, &outMemory, topLevelAS, offset] (void *) -> bool
		{
			VkBindAccelerationStructureMemoryInfoNV	bind_info = {};
			bind_info.sType					= VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
			bind_info.accelerationStructure	= topLevelAS;
			bind_info.memory				= outMemory;
			bind_info.memoryOffset			= offset;
			VK_CHECK( vulkan.vkBindAccelerationStructureMemoryNV( vulkan.GetVkDevice(), 1, &bind_info ));
			return true;
		});

		res.onDraw.push_back( [&vulkan, topLevelAS, instance_buffer, &scratch_buffer] (VkCommandBuffer cmd)
		{
			// write-read memory barrier for 'bottomLevelAS'
			// execution barrier for 'scratchBuffer'
			VkMemoryBarrier		barrier = {};
			barrier.sType			= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask	= VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
			barrier.dstAccessMask	= VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
			
			vulkan.vkCmdPipelineBarrier( cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
										 0, 1, &barrier, 0, null, 0, null );
			
			VkAccelerationStructureInfoNV	info = {};
			info.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
			info.type			= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
			info.flags			= 0;
			info.instanceCount	= 1;

			vulkan.vkCmdBuildAccelerationStructureNV( cmd, &info,
													  instance_buffer, 0,			// instance
													  VK_FALSE,						// update
													  topLevelAS, VK_NULL_HANDLE,	// dst, src
													  scratch_buffer, 0
													);
		});
	}
	
	// create scratch buffer
	{
		VkBufferCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags			= 0;
		info.usage			= VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		// calculate buffer size
		{
			VkMemoryRequirements2								mem_req2	= {};
			VkAccelerationStructureMemoryRequirementsInfoNV		as_info		= {};
			as_info.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
			as_info.type					= VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
			as_info.accelerationStructure	= topLevelAS;

			vulkan.vkGetAccelerationStructureMemoryRequirementsNV( vulkan.GetVkDevice(), &as_info, OUT &mem_req2 );
			info.size = mem_req2.memoryRequirements.size;
		
			as_info.accelerationStructure	= bottomLevelAS;
			vulkan.vkGetAccelerationStructureMemoryRequirementsNV( vulkan.GetVkDevice(), &as_info, OUT &mem_req2 );
			info.size = Max( info.size, mem_req2.memoryRequirements.size );
		}

		VK_CHECK( vulkan.vkCreateBuffer( vulkan.GetVkDevice(), &info, null, OUT &scratch_buffer ));
		
		VkMemoryRequirements	mem_req;
		vulkan.vkGetBufferMemoryRequirements( vulkan.GetVkDevice(), scratch_buffer, OUT &mem_req );
		
		VkDeviceSize	offset = AlignToLarger( res.dev.totalSize, mem_req.alignment );
		res.dev.totalSize	 = offset + mem_req.size;
		res.dev.memTypeBits	|= mem_req.memoryTypeBits;

		res.onBind.push_back( [&vulkan, scratch_buffer, &outMemory, offset] (void *) -> bool
		{
			VK_CHECK( vulkan.vkBindBufferMemory( vulkan.GetVkDevice(), scratch_buffer, outMemory, offset ));
			return true;
		});
	}

	// create shader binding table
	{
		VkBufferCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags			= 0;
		info.size			= numGroups * vulkan.GetDeviceRayTracingProperties().shaderGroupHandleSize;
		info.usage			= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( vulkan.vkCreateBuffer( vulkan.GetVkDevice(), &info, null, OUT &shaderBindingTable ));
		
		VkMemoryRequirements	mem_req;
		vulkan.vkGetBufferMemoryRequirements( vulkan.GetVkDevice(), shaderBindingTable, OUT &mem_req );
		
		VkDeviceSize	offset = AlignToLarger( res.dev.totalSize, mem_req.alignment );
		res.dev.totalSize	 = offset + mem_req.size;
		res.dev.memTypeBits	|= mem_req.memoryTypeBits;

		res.onBind.push_back( [&vulkan, shaderBindingTable, &outMemory, offset] (void *) -> bool
		{
			VK_CHECK( vulkan.vkBindBufferMemory( vulkan.GetVkDevice(), shaderBindingTable, outMemory, offset ));
			return true;
		});

		res.onDraw.push_back( [&vulkan, shaderBindingTable, rtPipeline, numGroups, size = info.size] (VkCommandBuffer cmd)
		{
			Array<uint8_t>	handles;  handles.resize(size);

			VK_CALL( vulkan.vkGetRayTracingShaderGroupHandlesNV( vulkan.GetVkDevice(), rtPipeline, 0, numGroups, handles.size(), OUT handles.data() ));
		
			vulkan.vkCmdUpdateBuffer( cmd, shaderBindingTable, 0, handles.size(), handles.data() );
		});
	}
	
	// allocate device local memory
	{
		VkMemoryAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize		= res.dev.totalSize;
		CHECK_ERR( vulkan.GetMemoryTypeIndex( res.dev.memTypeBits, res.dev.memProperty, OUT info.memoryTypeIndex ));

		VK_CHECK( vulkan.vkAllocateMemory( vulkan.GetVkDevice(), &info, null, OUT &outMemory ));
	}

	// allocate host visible memory
	void* host_ptr = null;
	{
		VkMemoryAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize		= res.host.totalSize;
		CHECK_ERR( vulkan.GetMemoryTypeIndex( res.host.memTypeBits, res.host.memProperty, OUT info.memoryTypeIndex ));

		VK_CHECK( vulkan.vkAllocateMemory( vulkan.GetVkDevice(), &info, null, OUT &host_memory ));

		VK_CHECK( vulkan.vkMapMemory( vulkan.GetVkDevice(), host_memory, 0, res.host.totalSize, 0, &host_ptr ));
	}

	// bind resources
	for (auto& bind : res.onBind) {
		CHECK_ERR( bind( host_ptr ));
	}

	// update resources
	{
		VkCommandBufferBeginInfo	begin_info = {};
		begin_info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vulkan.vkBeginCommandBuffer( helper.cmdBuffer, &begin_info ));

		for (auto& cb : res.onDraw) {
			cb( helper.cmdBuffer );
		}

		VK_CALL( vulkan.vkEndCommandBuffer( helper.cmdBuffer ));

		VkSubmitInfo		submit_info = {};
		submit_info.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount	= 1;
		submit_info.pCommandBuffers		= &helper.cmdBuffer;

		VK_CHECK( vulkan.vkQueueSubmit( helper.queue, 1, &submit_info, VK_NULL_HANDLE ));
	}
	VK_CALL( vulkan.vkQueueWaitIdle( helper.queue ));
	
	vulkan.vkDestroyBuffer( vulkan.GetVkDevice(), vertex_buffer, null );
	vulkan.vkDestroyBuffer( vulkan.GetVkDevice(), index_buffer, null );
	vulkan.vkDestroyBuffer( vulkan.GetVkDevice(), instance_buffer, null );
	vulkan.vkDestroyBuffer( vulkan.GetVkDevice(), scratch_buffer, null );
	vulkan.vkFreeMemory( vulkan.GetVkDevice(), host_memory, null );

	return true;
}

/*
=================================================
	TestDebugOutput
=================================================
*/
bool TestDebugOutput (const TestHelpers &helper, VkShaderModule module, StringView referenceFile)
{
	CHECK_ERR( referenceFile.size() );

	String			merged;
	Array<String>	debug_output;

	CHECK_ERR( helper.compiler.GetDebugOutput( module, helper.readBackPtr, helper.debugOutputSize, OUT debug_output ));
	CHECK_ERR( debug_output.size() );

	std::sort( debug_output.begin(), debug_output.end() );
	for (auto& str : debug_output) { merged << str << "//---------------------------\n\n"; }

	if ( UpdateReferences )
	{
		FileWStream		file{ String{FG_DATA_PATH} << referenceFile };
		CHECK_ERR( file.IsOpen() );
		CHECK_ERR( file.Write( StringView{merged} ));
		return true;
	}

	String	file_data;
	{
		FileRStream		file{ String{FG_DATA_PATH} << referenceFile };
		CHECK_ERR( file.IsOpen() );

		CHECK_ERR( file.Read( size_t(file.Size()), OUT file_data ));
	}

	CHECK_ERR( file_data == merged );
	return true;
}

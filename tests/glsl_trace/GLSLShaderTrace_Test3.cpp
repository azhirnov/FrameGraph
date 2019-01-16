// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "GLSLShaderTraceTestUtils.h"

/*
=================================================
	CompileShaders
=================================================
*/
static bool CompileShaders (VulkanDevice &vulkan, ShaderCompiler &shaderCompiler, OUT VkShaderModule &vertShader, OUT VkShaderModule &fragShader)
{
	// create vertex shader
	{
		static const char	vert_shader_source[] = R"#(
const vec2	g_Positions[] = {
	{-1.0f, -1.0f}, {-1.0f, 2.0f}, {2.0f, -1.0f},	// primitive 0 - must hit
};

void main()
{
	gl_Position	= vec4( g_Positions[gl_VertexIndex], float(gl_VertexIndex) * 0.02f, 1.0f );
})#";

		CHECK_ERR( shaderCompiler.Compile( OUT vertShader, vulkan, {vert_shader_source}, EShLangVertex ));
	}

	// create fragment shader
	{
		static const char	frag_shader_source[] = R"#(
layout(location = 0) out vec4  out_Color;

void main ()
{
	mat4x4	m1;
	m1[0] = vec4(1.0f);		m1[1] = vec4(2.0f);		m1[2] = vec4(3.0f);		m1[3] = vec4(4.0f);

	mat2x3	m2;
	m2[0] = vec3(11.1f);	m2[1] = vec3(12.2f);

	mat4x4	m3;
	m3[0] = vec4(6.0f);		m3[1] = vec4(7.0f);		m3[2] = vec4(8.0f);		m3[3] = vec4(9.0f);
	
	m3[0][1] = 44.0f;

	mat4x4	m4 = m1 * m3;

	out_Color = (m4 * vec4(8.0f)) + vec4(m2 * vec2(9.0f), 1.0f);
})#";

		CHECK_ERR( shaderCompiler.Compile( OUT fragShader, vulkan, {frag_shader_source}, EShLangFragment, 0 ));
	}
	return true;
}

/*
=================================================
	CreatePipeline
=================================================
*/
static bool CreatePipeline (VulkanDevice &vulkan, VkShaderModule vertShader, VkShaderModule fragShader,
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
	GLSLShaderTrace_Test3
=================================================
*/
extern bool GLSLShaderTrace_Test3 (VulkanDeviceExt& vulkan, const TestHelpers &helper)
{
	// create renderpass and framebuffer
	uint			width = 16, height = 16;
	VkRenderPass	render_pass;
	VkImage			color_target;
	VkDeviceMemory	image_mem;
	VkFramebuffer	framebuffer;
	CHECK_ERR( CreateRenderTarget( vulkan, VK_FORMAT_R8G8B8A8_UNORM, width, height, 0,
								   OUT render_pass, OUT color_target, OUT image_mem, OUT framebuffer ));


	// create pipeline
	ShaderCompiler	shader_compiler;
	VkShaderModule	vert_shader, frag_shader;
	CHECK_ERR( CompileShaders( vulkan, shader_compiler, OUT vert_shader, OUT frag_shader ));

	VkDescriptorSetLayout	ds_layout;
	VkDescriptorSet			desc_set;
	CHECK_ERR( CreateDebugDescriptorSet( vulkan, helper, VK_SHADER_STAGE_FRAGMENT_BIT, OUT ds_layout, OUT desc_set ));

	VkPipelineLayout	ppln_layout;
	VkPipeline			pipeline;
	CHECK_ERR( CreatePipeline( vulkan, vert_shader, frag_shader, ds_layout, render_pass, OUT ppln_layout, OUT pipeline ));


	// build command buffer
	VkCommandBufferBeginInfo	begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, null, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, null };
	VK_CHECK( vulkan.vkBeginCommandBuffer( helper.cmdBuffer, &begin ));

	// image layout undefined -> color_attachment
	{
		VkImageMemoryBarrier	barrier = {};
		barrier.sType			= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask	= 0;
		barrier.dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.oldLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.image			= color_target;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange	= {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

		vulkan.vkCmdPipelineBarrier( helper.cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
									 0, null, 0, null, 1, &barrier);
	}
	
	// setup storage buffer
	{
		const uint	data[] = { 
			width/2, height/2,		// selected pixel
			~0u,					// any sample
			~0u						// any primitive
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
		
		vulkan.vkCmdPipelineBarrier( helper.cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
									 0, null, 1, &barrier, 0, null);
	}

	// begin render pass
	{
		VkClearValue			clear_value = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
		VkRenderPassBeginInfo	begin_rp	= {};
		begin_rp.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		begin_rp.framebuffer		= framebuffer;
		begin_rp.renderPass			= render_pass;
		begin_rp.renderArea			= { {0,0}, {width, height} };
		begin_rp.clearValueCount	= 1;
		begin_rp.pClearValues		= &clear_value;

		vulkan.vkCmdBeginRenderPass( helper.cmdBuffer, &begin_rp, VK_SUBPASS_CONTENTS_INLINE );
	}
			
	vulkan.vkCmdBindPipeline( helper.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
	vulkan.vkCmdBindDescriptorSets( helper.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ppln_layout, 0, 1, &desc_set, 0, null );
	
	// set dynamic states
	{
		VkViewport	viewport = {};
		viewport.x			= 0.0f;
		viewport.y			= 0.0f;
		viewport.width		= float(width);
		viewport.height		= float(height);
		viewport.minDepth	= 0.0f;
		viewport.maxDepth	= 1.0f;
		vulkan.vkCmdSetViewport( helper.cmdBuffer, 0, 1, &viewport );

		VkRect2D	scissor_rect = { {0,0}, {width, height} };
		vulkan.vkCmdSetScissor( helper.cmdBuffer, 0, 1, &scissor_rect );
	}
			
	vulkan.vkCmdDraw( helper.cmdBuffer, 3, 1, 0, 0 );
			
	vulkan.vkCmdEndRenderPass( helper.cmdBuffer );

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
		
		vulkan.vkCmdPipelineBarrier( helper.cmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
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
		vulkan.vkDestroyDescriptorSetLayout( vulkan.GetVkDevice(), ds_layout, null );
		vulkan.vkDestroyShaderModule( vulkan.GetVkDevice(), vert_shader, null );
		vulkan.vkDestroyShaderModule( vulkan.GetVkDevice(), frag_shader, null );
		vulkan.vkDestroyPipelineLayout( vulkan.GetVkDevice(), ppln_layout, null );
		vulkan.vkDestroyPipeline( vulkan.GetVkDevice(), pipeline, null );
		vulkan.vkFreeMemory( vulkan.GetVkDevice(), image_mem, null );
		vulkan.vkDestroyImage( vulkan.GetVkDevice(), color_target, null );
		vulkan.vkDestroyRenderPass( vulkan.GetVkDevice(), render_pass, null );
		vulkan.vkDestroyFramebuffer( vulkan.GetVkDevice(), framebuffer, null );
	}
	

	// process debug output
	Array<String>	debug_output;
	CHECK_ERR( shader_compiler.GetDebugOutput( frag_shader, helper.readBackPtr, helper.debugOutputSize, OUT debug_output ));

	{
		static const char	ref[] = R"#(// gl_FragCoord: float4 {8.500000, 8.500000, 0.021250, 1.000000}
no source

// gl_PrimitiveID: int {0}
no source

// m1: float4 {1.000000, 1.000000, 1.000000, 1.000000}
7. m1[0] = vec4(1.0f);		m1[1] = vec4(2.0f);		m1[2] = vec4(3.0f);		m1[3] = vec4(4.0f);

// m1: float4 {2.000000, 2.000000, 2.000000, 2.000000}
7. m1[1] = vec4(2.0f);		m1[2] = vec4(3.0f);		m1[3] = vec4(4.0f);

// m1: float4 {3.000000, 3.000000, 3.000000, 3.000000}
7. m1[2] = vec4(3.0f);		m1[3] = vec4(4.0f);

// m1: float4 {4.000000, 4.000000, 4.000000, 4.000000}
7. m1[3] = vec4(4.0f);

// m2: float3 {11.100000, 11.100000, 11.100000}
10. m2[0] = vec3(11.1f);	m2[1] = vec3(12.2f);

// m2: float3 {12.200000, 12.200000, 12.200000}
10. m2[1] = vec3(12.2f);

// m3: float4 {6.000000, 6.000000, 6.000000, 6.000000}
13. m3[0] = vec4(6.0f);		m3[1] = vec4(7.0f);		m3[2] = vec4(8.0f);		m3[3] = vec4(9.0f);

// m3: float4 {7.000000, 7.000000, 7.000000, 7.000000}
13. m3[1] = vec4(7.0f);		m3[2] = vec4(8.0f);		m3[3] = vec4(9.0f);

// m3: float4 {8.000000, 8.000000, 8.000000, 8.000000}
13. m3[2] = vec4(8.0f);		m3[3] = vec4(9.0f);

// m3: float4 {9.000000, 9.000000, 9.000000, 9.000000}
13. m3[3] = vec4(9.0f);

// m3: float {44.000000}
15. m3[0][1] = 44.0f;

// m4: float4x4 {{90.000000, 90.000000, 90.000000, 90.000000}, {90.000000, 90.000000, 90.000000, 90.000000}, {90.000000, 90.000000, 90.000000, 90.000000}, {90.000000, 90.000000, 90.000000, 90.000000}, }
// m1: float4 {4.000000, 4.000000, 4.000000, 4.000000}
// m3: float {44.000000}
17. m4 = m1 * m3;

// out_Color: float4 {3217.699951, 3217.699951, 3217.699951, 3009.000000}
// m2: float3 {12.200000, 12.200000, 12.200000}
// m4: float4x4 {{90.000000, 90.000000, 90.000000, 90.000000}, {90.000000, 90.000000, 90.000000, 90.000000}, {90.000000, 90.000000, 90.000000, 90.000000}, {90.000000, 90.000000, 90.000000, 90.000000}, }
19. out_Color = (m4 * vec4(8.0f)) + vec4(m2 * vec2(9.0f), 1.0f);

)#";

		CHECK_ERR( debug_output.size() == 1 );
		CHECK_ERR( debug_output[0] == ref );
	}

	FG_LOGI( TEST_NAME << " - passed" );
	return true;
}

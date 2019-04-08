// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"
#include "pipeline_compiler/VPipelineCompiler.h"

namespace FG
{

	static bool CreatePipeline (const VulkanDevice &vulkan, const VulkanDrawContext &ctx, const GraphicsPipelineDesc &desc,
								OUT VkPipelineLayout &pplnLayout, OUT VkPipeline &pipeline)
	{
		// create pipeline layout
		{
			VkPipelineLayoutCreateInfo	info = {};
			info.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.setLayoutCount			= 0;
			info.pSetLayouts			= null;
			info.pushConstantRangeCount	= 0;
			info.pPushConstantRanges	= null;

			VK_CHECK( vulkan.vkCreatePipelineLayout( vulkan.GetVkDevice(), &info, null, OUT &pplnLayout ));
		}

		const auto	FindShader = [&desc] (EShader type) -> VkShaderModule
		{
			auto	sh_iter = desc._shaders.find( type );
			if ( sh_iter != desc._shaders.end() )
			{
				auto	m_iter = sh_iter->second.data.find( EShaderLangFormat::VkShader_100 );
				if ( m_iter != sh_iter->second.data.end() )
				{
					if ( auto* module = UnionGetIf< PipelineDescription::VkShaderPtr >( &m_iter->second ))
						return BitCast<VkShaderModule>( (*module)->GetData() );
				}
			}
			return VK_NULL_HANDLE;
		};

		VkShaderModule	vert_shader = FindShader( EShader::Vertex );
		VkShaderModule	frag_shader = FindShader( EShader::Fragment );
		CHECK_ERR( vert_shader and frag_shader );

		VkPipelineShaderStageCreateInfo			stages[2] = {};
		stages[0].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[0].stage		= VK_SHADER_STAGE_VERTEX_BIT;
		stages[0].module	= vert_shader;
		stages[0].pName		= "main";
		stages[1].sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[1].stage		= VK_SHADER_STAGE_FRAGMENT_BIT;
		stages[1].module	= frag_shader;
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
		info.pVertexInputState		= &vertex_input;
		info.pInputAssemblyState	= &input_assembly;
		info.pRasterizationState	= &rasterization;
		info.pMultisampleState		= &multisample;
		info.pDepthStencilState		= &depth_stencil;
		info.pColorBlendState		= &blend_state;
		info.pDynamicState			= &dynamic_state;
		info.layout					= pplnLayout;
		info.renderPass				= BitCast<VkRenderPass>( ctx.renderPass );
		info.subpass				= 0;

		VK_CHECK( vulkan.vkCreateGraphicsPipelines( vulkan.GetVkDevice(), VK_NULL_HANDLE, 1, &info, null, OUT &pipeline ));
		return true;
	}


	bool FGApp::Test_RawDraw1 ()
	{
		GraphicsPipelineDesc	ppln;
		
		ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

out vec3  v_Color;

const vec2	g_Positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

const vec3	g_Colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

void main() {
	gl_Position	= vec4( g_Positions[gl_VertexIndex], 0.0, 1.0 );
	v_Color		= g_Colors[gl_VertexIndex];
}
)#" );
		
		ppln.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location=0) out vec4  out_Color;

in  vec3  v_Color;

void main() {
	out_Color = vec4(v_Color, 1.0);
}
)#" );

		CHECK_ERR( _pplnCompiler->Compile( INOUT ppln, EShaderLangFormat::VkShader_100 ));
		
		const uint2		view_size	= {800, 600};
		ImageID			image		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
																			EImageUsage::ColorAttachment | EImageUsage::TransferSrc }, Default, "RenderTarget" );

		VkPipeline			pipeline	= VK_NULL_HANDLE;
		VkPipelineLayout	ppln_layout	= VK_NULL_HANDLE;
		
		bool		data_is_correct = false;

		const auto	OnLoaded =	[OUT &data_is_correct] (const ImageView &imageData)
		{
			const auto	TestPixel = [&imageData] (float x, float y, const RGBA32f &color)
			{
				uint	ix	 = uint( (x + 1.0f) * 0.5f * float(imageData.Dimension().x) + 0.5f );
				uint	iy	 = uint( (y + 1.0f) * 0.5f * float(imageData.Dimension().y) + 0.5f );

				RGBA32f	col;
				imageData.Load( uint3(ix, iy, 0), OUT col );

				bool	is_equal = All(Equals( col, color, 0.1f ));
				ASSERT( is_equal );
				return is_equal;
			};

			data_is_correct  = true;
			data_is_correct &= TestPixel( 0.00f, -0.49f, RGBA32f{1.0f, 0.0f, 0.0f, 1.0f} );
			data_is_correct &= TestPixel( 0.49f,  0.49f, RGBA32f{0.0f, 1.0f, 0.0f, 1.0f} );
			data_is_correct &= TestPixel(-0.49f,  0.49f, RGBA32f{0.0f, 0.0f, 1.0f, 1.0f} );
			
			data_is_correct &= TestPixel( 0.00f, -0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel( 0.51f,  0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel(-0.51f,  0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel( 0.00f,  0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel( 0.51f, -0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel(-0.51f, -0.51f, RGBA32f{0.0f} );
		};

		
		CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{}.SetDebugFlags( EDebugFlags::Default ));
		CHECK_ERR( cmd );

		LogicalPassID	render_pass	= cmd->CreateRenderPass( RenderPassDesc( view_size )
											.AddTarget( RenderTargetID(0), image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
											.AddViewport( view_size ) );
		
		cmd->AddTask( render_pass, CustomDraw{ [&] (IDrawContext &ctx)
											{
												VulkanDrawContext const&	vk_ctx = UnionGet<VulkanDrawContext>( ctx.GetData() );
												VkCommandBuffer				cmdbuf = BitCast<VkCommandBuffer>( vk_ctx.commandBuffer );

												if ( not pipeline )
													CreatePipeline( _vulkan, vk_ctx, ppln, OUT ppln_layout, OUT pipeline );

												VkViewport	viewport = {};
												viewport.minDepth	= 0.0f;					viewport.maxDepth	= 1.0f;
												viewport.x			= 0.0f;					viewport.y			= 0.0f;
												viewport.width		= float(view_size.x);	viewport.height		= float(view_size.y);

												VkRect2D	scissor = { {}, {view_size.x, view_size.y} };

												_vulkan.vkCmdBindPipeline( cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
												_vulkan.vkCmdSetViewport( cmdbuf, 0, 1, &viewport );
												_vulkan.vkCmdSetScissor( cmdbuf, 0, 1, &scissor );
												_vulkan.vkCmdDraw( cmdbuf, 3, 1, 0, 0 );
											} });

		Task	t_draw	= cmd->AddTask( SubmitRenderPass{ render_pass });
		Task	t_read	= cmd->AddTask( ReadImage().SetImage( image, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_draw ) );
		FG_UNUSED( t_read );

		CHECK_ERR( _frameGraph->Execute( cmd ));
		CHECK_ERR( _frameGraph->WaitIdle() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( data_is_correct );

		DeleteResources( image );

		_vulkan.vkDestroyPipeline( _vulkan.GetVkDevice(), pipeline, null );
		_vulkan.vkDestroyPipelineLayout( _vulkan.GetVkDevice(), ppln_layout, null );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

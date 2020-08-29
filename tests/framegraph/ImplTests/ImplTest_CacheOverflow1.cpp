// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::ImplTest_CacheOverflow1 ()
	{
		GraphicsPipelineDesc	ppln;

		ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
const vec2	g_Positions[3] = vec2[]( vec2(0.0f, -0.5f), vec2(0.5f, 0.5f), vec2(-0.5f, 0.5f) );

void main() {
	gl_Position	= vec4( g_Positions[gl_VertexIndex], 0.0f, 1.0f );
}
)#" );
		
		ppln.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", R"#(
layout(binding=0) uniform sampler2D  un_Image;
layout(location=0) out vec4  out_Color;

void main() {
	out_Color = vec4(sin(gl_FragCoord.x), cos(gl_FragCoord.y), texelFetch(un_Image, ivec2(gl_FragCoord.xy), 0).r, 1.0f) * 0.5f + 0.5f;
}
)#" );
		
		GPipelineID		pipeline	= _frameGraph->CreatePipeline( ppln );
		CHECK_ERR( pipeline );

		SamplerID		sampler		= _frameGraph->CreateSampler( SamplerDesc{} );
		CommandBuffer	cmdBuffers[2] = {};
		const uint2		view_size	= {800, 600};

		PipelineResources	resources1, resources2;
		CHECK_ERR( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources1 ));
		CHECK_ERR( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources2 ));

		
		for (uint i = 0; i < 1000'000; ++i)
		{
			CHECK_ERR( _frameGraph->Wait({ cmdBuffers[i&1] }) );

			CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{} );
			CHECK_ERR( cmd );
			
			cmdBuffers[i&1] = cmd;

			ImageID			rt	= _frameGraph->CreateImage( ImageDesc{}.SetDimension( view_size ).SetFormat( EPixelFormat::RGBA8_UNorm )
																	.SetUsage( EImageUsage::ColorAttachment | EImageUsage::TransferSrc ),
															Default, "RenderTarget" );
			ImageID			img	= _frameGraph->CreateImage( ImageDesc{}.SetDimension( view_size ).SetFormat( EPixelFormat::R8_UNorm )
																	.SetUsage( EImageUsage::Sampled | EImageUsage::TransferDst ),
															Default, "Color" );
			CHECK_ERR( rt and img );

			LogicalPassID	rp1	= cmd->CreateRenderPass( RenderPassDesc( view_size )
												.AddTarget( RenderTargetID::Color_0, rt, RGBA32f(0.0f), EAttachmentStoreOp::Store )
												.AddViewport( view_size ));
			LogicalPassID	rp2	= cmd->CreateRenderPass( RenderPassDesc( view_size )
												.AddTarget( RenderTargetID::Color_0, rt, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
												.AddViewport( view_size ));
			CHECK_ERR( rp1 and rp2 );
		
			resources1.BindTexture( UniformID("un_Image"), img, sampler );
			resources2.BindTexture( UniformID("un_Image"), img, sampler );

			cmd->AddTask( rp1, DrawVertices().Draw( 3 ).SetPipeline( pipeline ).SetTopology( EPrimitive::TriangleList ).AddResources( DescriptorSetID("0"), &resources1 ));
			cmd->AddTask( rp2, DrawVertices().Draw( 3 ).SetPipeline( pipeline ).SetTopology( EPrimitive::TriangleList ).AddResources( DescriptorSetID("0"), &resources2 ));

			Task	t_clear	= cmd->AddTask( ClearColorImage{}.SetImage(img).Clear(RGBA32f{1.0f}).AddRange( 0_mipmap, 1, 0_layer, 1 ));
			Task	t_draw1	= cmd->AddTask( SubmitRenderPass{ rp1 }.DependsOn( t_clear ));
			Task	t_draw2	= cmd->AddTask( SubmitRenderPass{ rp2 }.DependsOn( t_draw1 ));
			Task	t_read	= cmd->AddTask( ReadImage().SetImage( rt, int2{}, uint2{1} ).SetCallback([](auto&) {}).DependsOn( t_draw2 ));
			Unused( t_read );

			CHECK_ERR( _frameGraph->Execute( cmd ));

			_frameGraph->ReleaseResource( rt );
			_frameGraph->ReleaseResource( img );
			
			CHECK_ERR( _frameGraph->Flush() );
		}

		CHECK_ERR( _frameGraph->WaitIdle() );

		DeleteResources( pipeline );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
out vec4	out_Color;

void main() {
	out_Color = vec4(sin(gl_FragCoord.x), cos(gl_FragCoord.y), texelFetch(un_Image, ivec2(gl_FragCoord.xy), 0).r, 1.0f) * 0.5f + 0.5f;
}
)#" );
		
		FGThreadPtr		frame_graph	= _fgGraphics1;
		const uint2		view_size	= {800, 600};
		GPipelineID		pipeline	= frame_graph->CreatePipeline( std::move(ppln) );
		SamplerID		sampler		= frame_graph->CreateSampler( SamplerDesc{} );

		PipelineResources	resources1, resources2;
		CHECK_ERR( frame_graph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources1 ));
		CHECK_ERR( frame_graph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources2 ));

		CommandBatchID	batch_id {"main"};
		SubmissionGraph	submission_graph;
		submission_graph.AddBatch( batch_id );
		
		for (uint i = 0; i < 1000'000; ++i)
		{
			CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
			CHECK_ERR( frame_graph->Begin( batch_id, 0, EThreadUsage::Graphics ));
			
			ImageID			rt	= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
																			EImageUsage::ColorAttachment | EImageUsage::TransferSrc }, Default, "RenderTarget" );
			ImageID			img	= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::R8_UNorm,
																			EImageUsage::Sampled | EImageUsage::TransferDst }, Default, "Color" );
			LogicalPassID	rp1	= frame_graph->CreateRenderPass( RenderPassDesc( view_size )
												.AddTarget( RenderTargetID("out_Color"), rt, RGBA32f(0.0f), EAttachmentStoreOp::Store )
												.AddViewport( view_size ));
			LogicalPassID	rp2	= frame_graph->CreateRenderPass( RenderPassDesc( view_size )
												.AddTarget( RenderTargetID("out_Color"), rt, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
												.AddViewport( view_size ));
		
			resources1.BindTexture( UniformID("un_Image"), img, sampler );
			resources2.BindTexture( UniformID("un_Image"), img, sampler );

			frame_graph->AddTask( rp1, DrawVertices().AddDrawCmd( 3 ).SetPipeline( pipeline ).SetTopology( EPrimitive::TriangleList ).AddResources( DescriptorSetID("0"), &resources1 ));
			frame_graph->AddTask( rp2, DrawVertices().AddDrawCmd( 3 ).SetPipeline( pipeline ).SetTopology( EPrimitive::TriangleList ).AddResources( DescriptorSetID("0"), &resources2 ));

			Task	t_clear	= frame_graph->AddTask( ClearColorImage{}.SetImage(img).Clear(RGBA32f{1.0f}).AddRange( 0_mipmap, 1, 0_layer, 1 ));
			Task	t_draw1	= frame_graph->AddTask( SubmitRenderPass{ rp1 }.DependsOn( t_clear ));
			Task	t_draw2	= frame_graph->AddTask( SubmitRenderPass{ rp2 }.DependsOn( t_draw1 ));
			Task	t_read	= frame_graph->AddTask( ReadImage().SetImage( rt, int2{}, uint2{1} ).SetCallback([](auto&) {}).DependsOn( t_draw2 ));
			FG_UNUSED( t_read );

			CHECK_ERR( frame_graph->Execute() );
			CHECK_ERR( _fgInstance->EndFrame() );

			frame_graph->ReleaseResource( rt );
			frame_graph->ReleaseResource( img );
		}

		CHECK_ERR( _fgInstance->WaitIdle() );

		DeleteResources( pipeline );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

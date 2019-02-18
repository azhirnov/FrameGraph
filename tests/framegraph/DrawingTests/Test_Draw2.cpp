// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"
#include <thread>

namespace FG
{

	bool FGApp::Test_Draw2 ()
	{
		GraphicsPipelineDesc	ppln;

		ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

out vec3	v_Color;

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

in  vec3	v_Color;
out vec4	out_Color;

void main() {
	out_Color = vec4(v_Color, 1.0);
}
)#" );
		
		FGThreadPtr		frame_graph	= _fgThreads[0];

		const uint2		view_size	= _window->GetSize();
		ImageID			image		= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
																			EImageUsage::ColorAttachment | EImageUsage::TransferSrc }, Default, "RenderTarget" );

		GPipelineID		pipeline	= frame_graph->CreatePipeline( ppln );
		CHECK_ERR( pipeline );

		
		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
		CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
		CHECK_ERR( frame_graph->Begin( batch_id, 0, EQueueUsage::Graphics ));

		LogicalPassID		render_pass	= frame_graph->CreateRenderPass( RenderPassDesc( view_size )
												.AddTarget( RenderTargetID("out_Color"), image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
												.AddViewport( view_size ) );
		
		frame_graph->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( pipeline ).SetTopology( EPrimitive::TriangleList ));

		Task	t_draw		= frame_graph->AddTask( SubmitRenderPass{ render_pass });
		Task	t_present	= frame_graph->AddTask( Present{ image }.DependsOn( t_draw ));
		FG_UNUSED( t_present );

		CHECK_ERR( frame_graph->Execute() );
		CHECK_ERR( _fgInstance->EndFrame() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( _fgInstance->WaitIdle() );

		DeleteResources( image, pipeline );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

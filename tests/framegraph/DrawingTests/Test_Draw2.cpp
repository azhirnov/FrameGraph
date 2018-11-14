// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"
#include <thread>

namespace FG
{

	bool FGApp::Test_Draw2 ()
	{
		GraphicsPipelineDesc	ppln;

		ppln.AddShader( EShader::Vertex,
						EShaderLangFormat::GLSL_450,
						"main",
R"#(
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
		
		ppln.AddShader( EShader::Fragment,
						EShaderLangFormat::GLSL_450,
						"main",
R"#(
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

in  vec3	v_Color;
out vec4	out_Color;

void main() {
	out_Color = vec4(v_Color, 1.0);
}
)#" );
		
		FGThreadPtr		frame_graph	= _frameGraph1;

		uint2			view_size	= _window->GetSize();
		ImageID			image		= CreateImage2D( view_size, EPixelFormat::RGBA8_UNorm, "DstImage" );

		GPipelineID		pipeline	= frame_graph->CreatePipeline( std::move(ppln) );

		
		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
		//for (;;)
		{
			CHECK_ERR( _frameGraphInst->Begin( submission_graph ));
			CHECK_ERR( frame_graph->Begin( batch_id, 0, EThreadUsage::Graphics ));

			LogicalPassID		render_pass	= frame_graph->CreateRenderPass( RenderPassDesc( view_size )
													.AddTarget( RenderTargetID("out_Color"), image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
													.AddViewport( view_size ) );
		
			frame_graph->AddDrawTask( render_pass,
									  DrawTask()
										.SetPipeline( pipeline ).SetVertices( 0, 3 )
										.SetRenderState( RenderState().SetTopology( EPrimitive::TriangleList ) )
			);

			Task	t_draw	= frame_graph->AddTask( SubmitRenderPass{ render_pass });
			Task	t_read	= frame_graph->AddTask( Present{ image } );
			FG_UNUSED( t_read );

			CHECK_ERR( frame_graph->Compile() );
			CHECK_ERR( _frameGraphInst->Execute() );

			//std::this_thread::sleep_for( std::chrono::milliseconds(10) );
		}

		//CHECK_ERR( _frameGraphInst->WaitIdle() );

		frame_graph->DestroyResource( image );
		frame_graph->DestroyResource( pipeline );
		//DeleteResources( image, pipeline );

        //FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

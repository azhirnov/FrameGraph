// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This test affects:
		- frame graph building and execution
		- tasks: SubmitRenderPass, DrawTask, ReadImage
		- resources: render pass, image, framebuffer, pipeline, pipeline resources
		- staging buffers
		- memory managment
*/

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_Draw1 ()
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
		
		uint2			view_size	= {800, 600};
		ImageID			image		= CreateImage2D( view_size, EPixelFormat::RGBA8_UNorm, "DstImage" );

		GPipelineID		pipeline	= _frameGraph->CreatePipeline( std::move(ppln) );

		
		bool		data_is_correct = false;

        const auto	OnLoaded =	[OUT &data_is_correct] (const ImageView &imageData)
		{
			const auto	TestPixel = [&imageData] (float x, float y, const RGBA32f &color)
			{
				uint	ix	 = uint( (x + 1.0f) * 0.5f * float(imageData.Dimension().x) + 0.5f );
				uint	iy	 = uint( (y + 1.0f) * 0.5f * float(imageData.Dimension().y) + 0.5f );

				RGBA32f	col;
				imageData.Load( uint3(ix, iy, 0), OUT col );

				bool	is_equal	= Equals( col.r, color.r, 0.1f ) and
									  Equals( col.g, color.g, 0.1f ) and
									  Equals( col.b, color.b, 0.1f ) and
									  Equals( col.a, color.a, 0.1f );
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

		
		CommandBatchID		batch_id{"0"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
        CHECK_ERR( _frameGraphInst->Begin( submission_graph ));
        CHECK_ERR( _frameGraph->Begin( batch_id, 0, EThreadUsage::Graphics ));

		RenderPass		render_pass	= _frameGraph->CreateRenderPass( RenderPassDesc( view_size )
											.AddTarget( RenderTargetID("out_Color"), image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
											.AddViewport( view_size ) );
		
		_frameGraph->AddDrawTask( render_pass,
								  DrawTask()
									.SetPipeline( pipeline ).SetVertices( 0, 3 )
									.SetRenderState( RenderState().SetTopology( EPrimitive::TriangleList ) )
		);

		Task	t_draw	= _frameGraph->AddTask( SubmitRenderPass{ render_pass });
		Task	t_read	= _frameGraph->AddTask( ReadImage().SetImage( image, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_draw ) );
		(void)(t_read);

		CHECK_ERR( _frameGraph->Compile() );		
		CHECK_ERR( _frameGraphInst->Execute() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME, EGraphVizFlags::Default ));

		CHECK_ERR( _frameGraphInst->WaitIdle() );

		CHECK_ERR( data_is_correct );

		DeleteResources( image, pipeline );

        FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_ShaderDebugger2 ()
	{
		GraphicsPipelineDesc	ppln;

		ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100 | EShaderLangFormat::EnableDebugTrace, "main", R"#(
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

void dbg_EnableTraceRecording (bool b) {}

void main()
{
	dbg_EnableTraceRecording( gl_VertexIndex == 1 || gl_VertexIndex == 2 );

	gl_Position	= vec4( g_Positions[gl_VertexIndex], 0.0, 1.0 );
	v_Color		= g_Colors[gl_VertexIndex];
}
)#", "VertexShader" );
		
		ppln.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100 | EShaderLangFormat::EnableDebugTrace, "main", R"#(
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

in  vec3	v_Color;
out vec4	out_Color;

void dbg_EnableTraceRecording (bool b) {}

void main()
{
	dbg_EnableTraceRecording( int(gl_FragCoord.x) == 400 && int(gl_FragCoord.y) == 300 );

	out_Color.rgb = v_Color.rgb;
	out_Color.a   = fract(v_Color.r + v_Color.g + v_Color.b + 0.5f);
}
)#", "FragmentShader" );
		
		FGThreadPtr		frame_graph	= _fgGraphics1;

		const uint2		view_size	= {800, 600};
		ImageID			image		= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
																			EImageUsage::ColorAttachment | EImageUsage::TransferSrc }, Default, "RenderTarget" );

		GPipelineID		pipeline	= frame_graph->CreatePipeline( ppln );
		CHECK_ERR( pipeline );

		
		bool	data_is_correct				= false;
		bool	shader_output_is_correct	= true;

		const auto	OnShaderTraceReady = [OUT &shader_output_is_correct] (StringView taskName, StringView shaderName, EShaderStages stages, ArrayView<String> output)
		{
			shader_output_is_correct &= (stages == (EShaderStages::Vertex | EShaderStages::Fragment));
			shader_output_is_correct &= (taskName == "DebuggableDraw");

			if ( shaderName == "VertexShader" )
			{
				const char	ref0[] = R"#(//> gl_VertexIndex: int {1}
//> gl_InstanceIndex: int {0}
no source

//> (out): float4 {0.500000, 0.500000, 0.000000, 1.000000}
//  gl_VertexIndex: int {1}
26. gl_Position	= vec4( g_Positions[gl_VertexIndex], 0.0, 1.0 );

//> v_Color: float3 {0.000000, 1.000000, 0.000000}
//  gl_VertexIndex: int {1}
27. v_Color		= g_Colors[gl_VertexIndex];

)#";
				const char	ref1[] = R"#(//> gl_VertexIndex: int {2}
//> gl_InstanceIndex: int {0}
no source

//> (out): float4 {-0.500000, 0.500000, 0.000000, 1.000000}
//  gl_VertexIndex: int {2}
26. gl_Position	= vec4( g_Positions[gl_VertexIndex], 0.0, 1.0 );

//> v_Color: float3 {0.000000, 0.000000, 1.000000}
//  gl_VertexIndex: int {2}
27. v_Color		= g_Colors[gl_VertexIndex];

)#";
				shader_output_is_correct &= (output.size() == 2 and ((output[0] == ref0 and output[1] == ref1) or (output[0] == ref1 and output[1] == ref0)));
				ASSERT( shader_output_is_correct );
			}
			else
			{
				const char	ref2[] = R"#(//> gl_FragCoord: float4 {400.500000, 300.500000, 0.000000, 1.000000}
//> gl_PrimitiveID: int {0}
//> v_Color: float3 {0.498333, 0.252083, 0.249583}
no source

//> out_Color: float3 {0.498333, 0.252083, 0.249583}
//  v_Color: float3 {0.498333, 0.252083, 0.249583}
15. out_Color.rgb = v_Color.rgb;

//> out_Color: float4 {0.498333, 0.252083, 0.249583, 0.500000}
//  v_Color: float3 {0.498333, 0.252083, 0.249583}
16. out_Color.a   = fract(v_Color.r + v_Color.g + v_Color.b + 0.5f);

)#";
				shader_output_is_correct &= (shaderName == "FragmentShader");
				shader_output_is_correct &= (output.size() == 1 and output[0] == ref2);
				ASSERT( shader_output_is_correct );
			}
		};
		frame_graph->SetShaderDebugCallback( OnShaderTraceReady );

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
			data_is_correct &= TestPixel( 0.00f, -0.49f, RGBA32f{1.0f, 0.0f, 0.0f, 0.5f} );
			data_is_correct &= TestPixel( 0.49f,  0.49f, RGBA32f{0.0f, 1.0f, 0.0f, 0.5f} );
			data_is_correct &= TestPixel(-0.49f,  0.49f, RGBA32f{0.0f, 0.0f, 1.0f, 0.5f} );
			
			data_is_correct &= TestPixel( 0.00f, -0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel( 0.51f,  0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel(-0.51f,  0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel( 0.00f,  0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel( 0.51f, -0.51f, RGBA32f{0.0f} );
			data_is_correct &= TestPixel(-0.51f, -0.51f, RGBA32f{0.0f} );
		};

		
		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
		CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
		CHECK_ERR( frame_graph->Begin( batch_id, 0, EThreadUsage::Graphics ));

		LogicalPassID		render_pass	= frame_graph->CreateRenderPass( RenderPassDesc( view_size )
												.AddTarget( RenderTargetID("out_Color"), image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
												.AddViewport( view_size ) );
		
		frame_graph->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( pipeline ).SetTopology( EPrimitive::TriangleList )
														 .SetName( "DebuggableDraw" )
														 .EnableDebugTrace( EShaderStages::Vertex | EShaderStages::Fragment ));

		Task	t_draw	= frame_graph->AddTask( SubmitRenderPass{ render_pass });
		Task	t_read	= frame_graph->AddTask( ReadImage().SetImage( image, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_draw ) );
		FG_UNUSED( t_read );

		CHECK_ERR( frame_graph->Execute() );
		CHECK_ERR( _fgInstance->EndFrame() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));

		CHECK_ERR( _fgInstance->WaitIdle() );

		CHECK_ERR( data_is_correct );

		DeleteResources( image, pipeline );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

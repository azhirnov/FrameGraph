// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_AsyncCompute1 ()
	{
		GraphicsPipelineDesc	gppln;
		ComputePipelineDesc		cppln;

		gppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
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
		
		gppln.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

in  vec3	v_Color;
out vec4	out_Color;

void main() {
	out_Color = 1.0f - vec4(v_Color, 1.0);
}
)#" );

		cppln.AddShader( EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(compute)
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(rgba8) uniform image2D  un_Image;

void main ()
{
	ivec2	coord = ivec2(gl_GlobalInvocationID.xy);
	vec4	color = imageLoad( un_Image, coord );

	imageStore( un_Image, coord, 1.0f - color );
}
)#" );
		
		// to create resources with sharing mode use framegraph that contains graphics and async compute queues
		FGThreadPtr		frame_graph	= _fgGraphicsCompute;

		const uint2		view_size	= {800, 600};
		ImageID			image		= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
																			EImageUsage::ColorAttachment | EImageUsage::TransferSrc | EImageUsage::Storage },
															    Default, "RenderTarget" );

		GPipelineID		gpipeline	= frame_graph->CreatePipeline( gppln );
		CPipelineID		cpipeline	= frame_graph->CreatePipeline( cppln );
		
		CHECK_ERR( gpipeline and cpipeline );

		PipelineResources	resources;
		CHECK_ERR( frame_graph->InitPipelineResources( cpipeline, DescriptorSetID("0"), OUT resources ));

		
		bool		data_is_correct = true;

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

		
		CommandBatchID		gbatch_id {"graphics"};
		CommandBatchID		cbatch_id {"compute"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( gbatch_id, 1, EThreadUsage::Graphics );
		submission_graph.AddBatch( cbatch_id, 1, EThreadUsage::AsyncCompute, { gbatch_id });
		
		// frame 1
		{
			CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));

			// graphics queue
			{
				CHECK_ERR( frame_graph->Begin( gbatch_id, 0, EThreadUsage::Graphics ));

				LogicalPassID	render_pass	= frame_graph->CreateRenderPass( RenderPassDesc( view_size )
														.AddTarget( RenderTargetID("out_Color"), image, RGBA32f(1.0f), EAttachmentStoreOp::Store )
														.AddViewport( view_size ) );
		
				frame_graph->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( gpipeline ).SetTopology( EPrimitive::TriangleList ));

				Task	t_draw	= frame_graph->AddTask( SubmitRenderPass{ render_pass });
				FG_UNUSED( t_draw );

				CHECK_ERR( frame_graph->Execute() );
			}

			// compute queue
			{
				CHECK_ERR( frame_graph->Begin( cbatch_id, 0, EThreadUsage::AsyncCompute ));
				
				resources.BindImage( UniformID("un_Image"), image );
				Task	t_comp	= frame_graph->AddTask( DispatchCompute().SetPipeline( cpipeline ).AddResources( DescriptorSetID("0"), &resources ).Dispatch( view_size ));
				Task	t_read	= frame_graph->AddTask( ReadImage().SetImage( image, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_comp ));
				FG_UNUSED( t_read );

				CHECK_ERR( frame_graph->Execute() );
			}

			CHECK_ERR( _fgInstance->EndFrame() );
		}
		
		// frame 2
		{
			// add synchronization between compute queue from previous frame and graphics queue in current frame
			CommandBatchID	sync_batch_id {"compute-graphics-sync"};

			submission_graph.Clear();
			submission_graph.AddBatch( sync_batch_id, 1, EThreadUsage::AsyncCompute );
			submission_graph.AddBatch( gbatch_id, 1, EThreadUsage::Graphics, {sync_batch_id} );
			submission_graph.AddBatch( cbatch_id, 1, EThreadUsage::AsyncCompute, { gbatch_id });

			CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));

			// this batch just enqueue the semaphore signal command
			CHECK_ERR( _fgInstance->SkipBatch( sync_batch_id, 0 ));

			// graphics queue
			{
				CHECK_ERR( _fgGraphics1->Begin( gbatch_id, 0, EThreadUsage::Graphics ));

				LogicalPassID	render_pass	= _fgGraphics1->CreateRenderPass( RenderPassDesc( view_size )
														.AddTarget( RenderTargetID("out_Color"), image, RGBA32f(1.0f), EAttachmentStoreOp::Store )
														.AddViewport( view_size ) );
		
				_fgGraphics1->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( gpipeline ).SetTopology( EPrimitive::TriangleList ));

				Task	t_draw	= _fgGraphics1->AddTask( SubmitRenderPass{ render_pass });
				FG_UNUSED( t_draw );

				CHECK_ERR( _fgGraphics1->Execute() );
			}

			// compute queue
			{
				CHECK_ERR( _fgCompute->Begin( cbatch_id, 0, EThreadUsage::AsyncCompute ));
				
				resources.BindImage( UniformID("un_Image"), image );
				Task	t_comp	= _fgCompute->AddTask( DispatchCompute().SetPipeline( cpipeline ).AddResources( DescriptorSetID("0"), &resources ).Dispatch( view_size ));
				Task	t_read	= _fgCompute->AddTask( ReadImage().SetImage( image, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_comp ));
				FG_UNUSED( t_read );

				CHECK_ERR( _fgCompute->Execute() );
			}

			CHECK_ERR( _fgInstance->EndFrame() );
		}

		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( _fgInstance->WaitIdle() );

		CHECK_ERR( data_is_correct );

		DeleteResources( image, gpipeline, cpipeline );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

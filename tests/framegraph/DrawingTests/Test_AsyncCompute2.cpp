// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Async compute without synchronizations between frames,
	used double buffering for render targets to avoid race condition.

  .------------------------.
  |      frame 1           |
  |             .------------------------.
  |             |        frame 2         |
  |-------------|-------------.----------|
  |  graphics1  |  graphics2  |          |
  |-------------|-------------|----------|
  |             | compute1 |  | compute2 |
  '-------------'----------'--'----------'
*/

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_AsyncCompute2 ()
	{
		GraphicsPipelineDesc	gppln;
		ComputePipelineDesc		cppln;

		gppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
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
		
		gppln.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location=0) out vec4  out_Color;

in  vec3  v_Color;

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
		
		const uint2		view_size	= {800, 600};
		ImageDesc		image_desc	{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
									  EImageUsage::ColorAttachment | EImageUsage::TransferSrc | EImageUsage::Storage };
						image_desc.queues = EQueueUsage::Graphics | EQueueUsage::AsyncCompute;

		ImageID			image1		= _frameGraph->CreateImage( image_desc, Default, "RenderTarget" );
		ImageID			image2		= _frameGraph->CreateImage( image_desc, Default, "RenderTarget" );

		GPipelineID		gpipeline	= _frameGraph->CreatePipeline( gppln );
		CPipelineID		cpipeline	= _frameGraph->CreatePipeline( cppln );
		
		CHECK_ERR( gpipeline and cpipeline );

		PipelineResources	resources;
		CHECK_ERR( _frameGraph->InitPipelineResources( cpipeline, DescriptorSetID("0"), OUT resources ));

		
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

		
		// frame 1
		CommandBuffer	cmd1 = _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics }.SetDebugFlags( ECompilationDebugFlags::Default ));
		CommandBuffer	cmd2 = _frameGraph->Begin( CommandBufferDesc{ EQueueType::AsyncCompute }.SetDebugFlags( ECompilationDebugFlags::Default ), {cmd1} );
		CHECK_ERR( cmd1 and cmd2 );
		{
			// graphics queue
			{
				LogicalPassID	render_pass	= cmd1->CreateRenderPass( RenderPassDesc( view_size )
														.AddTarget( RenderTargetID(0), image1, RGBA32f(1.0f), EAttachmentStoreOp::Store )
														.AddViewport( view_size ) );
		
				cmd1->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( gpipeline ).SetTopology( EPrimitive::TriangleList ));

				Task	t_draw	= cmd1->AddTask( SubmitRenderPass{ render_pass });
				FG_UNUSED( t_draw );

				CHECK_ERR( _frameGraph->Execute( cmd1 ));
			}

			// compute queue
			{
				resources.BindImage( UniformID("un_Image"), image1 );
				Task	t_comp	= cmd2->AddTask( DispatchCompute().SetPipeline( cpipeline ).AddResources( DescriptorSetID("0"), &resources ).Dispatch( view_size ));
				Task	t_read	= cmd2->AddTask( ReadImage().SetImage( image1, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_comp ));
				FG_UNUSED( t_read );

				CHECK_ERR( _frameGraph->Execute( cmd2 ));
			}
		}
		
		// frame 2
		CommandBuffer	cmd3 = _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics }.SetDebugFlags( ECompilationDebugFlags::Default ));
		CommandBuffer	cmd4 = _frameGraph->Begin( CommandBufferDesc{ EQueueType::AsyncCompute }.SetDebugFlags( ECompilationDebugFlags::Default ), {cmd3} );
		CHECK_ERR( cmd3 and cmd4 );
		{
			// graphics queue
			{
				LogicalPassID	render_pass	= cmd3->CreateRenderPass( RenderPassDesc( view_size )
														.AddTarget( RenderTargetID(0), image2, RGBA32f(1.0f), EAttachmentStoreOp::Store )
														.AddViewport( view_size ) );
		
				cmd3->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( gpipeline ).SetTopology( EPrimitive::TriangleList ));

				Task	t_draw	= cmd3->AddTask( SubmitRenderPass{ render_pass });
				FG_UNUSED( t_draw );

				CHECK_ERR( _frameGraph->Execute( cmd3 ));
			}

			// compute queue
			{
				resources.BindImage( UniformID("un_Image"), image2 );
				Task	t_comp	= cmd4->AddTask( DispatchCompute().SetPipeline( cpipeline ).AddResources( DescriptorSetID("0"), &resources ).Dispatch( view_size ));
				Task	t_read	= cmd4->AddTask( ReadImage().SetImage( image2, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_comp ));
				FG_UNUSED( t_read );

				CHECK_ERR( _frameGraph->Execute( cmd4 ));
			}
		}
		
		// frame 3
		CommandBuffer	cmd5 = _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics }.SetDebugFlags( ECompilationDebugFlags::Default ), {cmd2} );
		CommandBuffer	cmd6 = _frameGraph->Begin( CommandBufferDesc{ EQueueType::AsyncCompute }.SetDebugFlags( ECompilationDebugFlags::Default ), {cmd5} );
		CHECK_ERR( cmd5 and cmd6 );
		{
			// graphics queue
			{
				LogicalPassID	render_pass	= cmd5->CreateRenderPass( RenderPassDesc( view_size )
														.AddTarget( RenderTargetID(0), image1, RGBA32f(1.0f), EAttachmentStoreOp::Store )
														.AddViewport( view_size ) );
		
				cmd5->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( gpipeline ).SetTopology( EPrimitive::TriangleList ));

				Task	t_draw	= cmd5->AddTask( SubmitRenderPass{ render_pass });
				FG_UNUSED( t_draw );

				CHECK_ERR( _frameGraph->Execute( cmd5 ));
			}

			// compute queue
			{
				resources.BindImage( UniformID("un_Image"), image1 );
				Task	t_comp	= cmd6->AddTask( DispatchCompute().SetPipeline( cpipeline ).AddResources( DescriptorSetID("0"), &resources ).Dispatch( view_size ));
				Task	t_read	= cmd6->AddTask( ReadImage().SetImage( image1, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_comp ));
				FG_UNUSED( t_read );

				CHECK_ERR( _frameGraph->Execute( cmd6 ));
			}
		}

		CHECK_ERR( _frameGraph->WaitIdle() );

		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( data_is_correct );

		DeleteResources( image1, image2, gpipeline, cpipeline );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

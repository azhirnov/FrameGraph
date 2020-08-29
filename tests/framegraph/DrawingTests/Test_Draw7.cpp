// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"
#include <thread>

namespace FG
{

	bool FGApp::Test_Draw7 ()
	{
		GraphicsPipelineDesc	ppln;

		ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location=0) out vec3  out_Color1;
layout(location=1) out vec3  out_Color2;

const vec2	g_Positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

void main() {
	gl_Position	= vec4( g_Positions[gl_VertexIndex], 0.0, 1.0 );
	out_Color1	= vec3( 1.0, 0.0, 0.0 );
	out_Color2	= vec3( 0.0, 1.0, 0.0 );
}
)#" );
		
		ppln.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location=0) out vec4  out_Color1;
layout(location=1) out vec4  out_Color2;

layout(location=0) in vec3  in_Color1;
layout(location=1) in vec3  in_Color2;

void main() {
	out_Color1 = vec4(in_Color1, 1.0);
	out_Color2 = vec4(in_Color2, 1.0);
}
)#" );
		
		const uint2		view_size	= {800, 600};
		ImageID			image_1		= _frameGraph->CreateImage( ImageDesc{}.SetDimension( view_size ).SetFormat( EPixelFormat::RGBA8_UNorm )
																		.SetUsage( EImageUsage::ColorAttachment | EImageUsage::TransferSrc ),
																Default, "RenderTarget_1" );
		ImageID			image_2		= _frameGraph->CreateImage( ImageDesc{}.SetDimension( view_size ).SetFormat( EPixelFormat::RGBA8_UNorm )
																		.SetUsage( EImageUsage::ColorAttachment | EImageUsage::TransferSrc ),
																Default, "RenderTarget_2" );

		GPipelineID		pipeline	= _frameGraph->CreatePipeline( ppln );
		CHECK_ERR( image_1 and image_2 and pipeline );

		
		Optional<bool>		data_is_correct;
		
		const auto	TestPixel = [] (const ImageView &imageData, float x, float y, const RGBA32f &color)
		{
			uint	ix	 = uint( (x + 1.0f) * 0.5f * float(imageData.Dimension().x) + 0.5f );
			uint	iy	 = uint( (y + 1.0f) * 0.5f * float(imageData.Dimension().y) + 0.5f );

			RGBA32f	col;
			imageData.Load( uint3(ix, iy, 0), OUT col );

			bool	is_equal = All(Equals( col, color, 0.1f ));
			ASSERT( is_equal );
			return is_equal;
		};

		const auto	OnLoaded1 =	[OUT &data_is_correct, &TestPixel] (const ImageView &imageData)
		{
			if ( not data_is_correct )
				data_is_correct = true;

			*data_is_correct &= TestPixel( imageData, 0.0f,  0.0f, RGBA32f{1.0f, 0.0f, 0.0f, 1.0f} );
			*data_is_correct &= TestPixel( imageData, 0.0f, -0.7f, RGBA32f{0.0f} );
			*data_is_correct &= TestPixel( imageData, 0.7f,  0.7f, RGBA32f{0.0f} );
			*data_is_correct &= TestPixel( imageData,-0.7f,  0.7f, RGBA32f{0.0f} );
			*data_is_correct &= TestPixel( imageData, 0.0f,  0.7f, RGBA32f{0.0f} );
			*data_is_correct &= TestPixel( imageData, 0.7f, -0.7f, RGBA32f{0.0f} );
			*data_is_correct &= TestPixel( imageData,-0.7f, -0.7f, RGBA32f{0.0f} );
		};

		const auto	OnLoaded2 =	[OUT &data_is_correct, &TestPixel] (const ImageView &imageData)
		{
			if ( not data_is_correct )
				data_is_correct = true;

			*data_is_correct &= TestPixel( imageData, 0.0f,  0.0f, RGBA32f{0.0f, 1.0f, 0.0f, 1.0f} );
			*data_is_correct &= TestPixel( imageData, 0.0f, -0.7f, RGBA32f{0.0f} );
			*data_is_correct &= TestPixel( imageData, 0.7f,  0.7f, RGBA32f{0.0f} );
			*data_is_correct &= TestPixel( imageData,-0.7f,  0.7f, RGBA32f{0.0f} );
			*data_is_correct &= TestPixel( imageData, 0.0f,  0.7f, RGBA32f{0.0f} );
			*data_is_correct &= TestPixel( imageData, 0.7f, -0.7f, RGBA32f{0.0f} );
			*data_is_correct &= TestPixel( imageData,-0.7f, -0.7f, RGBA32f{0.0f} );
		};
		
		CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{}.SetDebugFlags( EDebugFlags::Default ));
		CHECK_ERR( cmd );

		LogicalPassID	render_pass	= cmd->CreateRenderPass( RenderPassDesc( view_size )
											.AddTarget( RenderTargetID::Color_0, image_1, RGBA32f(0.0f), EAttachmentStoreOp::Store )
											.AddTarget( RenderTargetID::Color_1, image_2, RGBA32f(0.0f), EAttachmentStoreOp::Store )
											.AddViewport( view_size ));
		CHECK_ERR( render_pass );
		
		cmd->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( pipeline ).SetTopology( EPrimitive::TriangleList ));

		Task	t_draw	= cmd->AddTask( SubmitRenderPass{ render_pass });
		Task	t_read1	= cmd->AddTask( ReadImage().SetImage( image_1, int2(), view_size ).SetCallback( OnLoaded1 ).DependsOn( t_draw ));
		Task	t_read2	= cmd->AddTask( ReadImage().SetImage( image_2, int2(), view_size ).SetCallback( OnLoaded2 ).DependsOn( t_draw ));
		Unused( t_read1, t_read2 );

		CHECK_ERR( _frameGraph->Execute( cmd ));
		CHECK_ERR( _frameGraph->WaitIdle() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( data_is_correct.value_or(false) );

		DeleteResources( image_1, image_2, pipeline );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

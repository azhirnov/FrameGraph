// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_Draw4 ()
	{
		GraphicsPipelineDesc	ppln;

		ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// @set 0 PerObject
layout(set=0, binding=0, std140) uniform VertexPositionsUB {
	vec4	positions[3];
};

// @set 1 PerPass
layout(set=1, binding=0, std140) uniform VertexColorsUB {
	vec4	colors[3];
};

out vec3  v_Color;

void main() {
	gl_Position	= vec4( positions[gl_VertexIndex].xy, 0.0, 1.0 );
	v_Color		= colors[gl_VertexIndex].rgb;
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
		
		const uint2			view_size	= {800, 600};
		ImageID				image		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
																				EImageUsage::ColorAttachment | EImageUsage::TransferSrc }, Default, "RenderTarget" );

		const float4		positions[] = { {0.0f, -0.5f, 0.0f, 0.0f},  {0.5f, 0.5f, 0.0f, 0.0f},  {-0.5f, 0.5f, 0.0f, 0.0f} };
		const float4		colors[]	= { {1.0f, 0.0f, 0.0f, 1.0f},   {0.0f, 1.0f, 0.0f, 1.0f},  {0.0f, 0.0f, 1.0f, 1.0f}  };
		
		BufferID			positions_ub = _frameGraph->CreateBuffer( BufferDesc{ BytesU::SizeOf(positions), EBufferUsage::Uniform },
																	  MemoryDesc{ EMemoryType::HostWrite }, "PositionsUB" );
		BufferID			colors_ub	 = _frameGraph->CreateBuffer( BufferDesc{ BytesU::SizeOf(colors), EBufferUsage::Uniform },
																	  MemoryDesc{ EMemoryType::HostWrite }, "ColorsUB" );

		CHECK_ERR( _frameGraph->UpdateHostBuffer( positions_ub, 0_b, BytesU::SizeOf(positions), positions ));
		CHECK_ERR( _frameGraph->UpdateHostBuffer( colors_ub, 0_b, BytesU::SizeOf(colors), colors ));

		GPipelineID			pipeline	= _frameGraph->CreatePipeline( ppln );
		PipelineResources	resources0;
		PipelineResources	resources1;

		CHECK_ERR( pipeline );
		CHECK_ERR( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID{"PerObject"}, OUT resources0 ));
		CHECK_ERR( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID{"PerPass"},   OUT resources1 ));

		resources0.BindBuffer( UniformID{"VertexPositionsUB"}, positions_ub );
		resources1.BindBuffer( UniformID{"VertexColorsUB"},    colors_ub );
		

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

		
		CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{} );
		CHECK_ERR( cmd );

		LogicalPassID	render_pass	= cmd->CreateRenderPass( RenderPassDesc( view_size )
											.AddTarget( RenderTargetID(0), image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
											.AddViewport( view_size )
											.AddResources( DescriptorSetID{"PerPass"}, &resources1 ));
		
		cmd->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( pipeline ).SetTopology( EPrimitive::TriangleList )
														 .AddResources( DescriptorSetID{"PerObject"}, &resources0 ));

		Task	t_draw	= cmd->AddTask( SubmitRenderPass{ render_pass });
		Task	t_read	= cmd->AddTask( ReadImage().SetImage( image, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_draw ) );
		FG_UNUSED( t_read );

		CHECK_ERR( _frameGraph->Execute( cmd ));
		
		CHECK_ERR( CompareDumps( TEST_NAME ));

		CHECK_ERR( _frameGraph->WaitIdle() );

		CHECK_ERR( data_is_correct );

		DeleteResources( image, pipeline, positions_ub, colors_ub );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_Draw6 ()
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

layout(location=0) out vec3  v_Color;
layout(location=1) out vec2  v_Texcoord;

void main() {
	gl_Position	= vec4( positions[gl_VertexIndex].xy, 0.0, 1.0 );
	v_Texcoord	= positions[gl_VertexIndex].xy * 0.5f + 0.5f;
	v_Color		= colors[gl_VertexIndex].rgb;
}
)#" );
		
		ppln.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// @set 0 PerObject
layout(set=0, binding=1) uniform sampler2D  un_ColorTexture;

layout(location=0) out vec4  out_Color;

layout(location=0) in  vec3  v_Color;
layout(location=1) in  vec2  v_Texcoord;

void main() {
	out_Color = vec4(v_Color, 1.0) * texture( un_ColorTexture, v_Texcoord );
}
)#" );
		
		const uint2		view_size	= {800, 600};
		ImageID			image		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
																			EImageUsage::ColorAttachment | EImageUsage::TransferSrc }, Default, "RenderTarget" );
		
		const uint2		tex_size	= {128, 128};
		ImageID			texture		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{tex_size.x, tex_size.y, 1}, EPixelFormat::RGBA8_UNorm,
																				EImageUsage::Sampled | EImageUsage::TransferDst }, Default, "Texture" );

		RawSamplerID	sampler		= _frameGraph->CreateSampler( SamplerDesc{}.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Nearest )).Release();
		
		const float4	positions[] = { {0.0f, -0.5f, 0.0f, 0.0f},  {0.5f, 0.5f, 0.0f, 0.0f},  {-0.5f, 0.5f, 0.0f, 0.0f} };
		const float4	colors[]	= { {1.0f, 0.0f, 0.0f, 1.0f},   {0.0f, 1.0f, 0.0f, 1.0f},  {0.0f, 0.0f, 1.0f, 1.0f}  };
		
		BufferID		positions_ub = _frameGraph->CreateBuffer( BufferDesc{ BytesU::SizeOf(positions), EBufferUsage::Uniform },
																	  MemoryDesc{ EMemoryType::HostWrite }, "PositionsUB" );
		BufferID		colors_ub	 = _frameGraph->CreateBuffer( BufferDesc{ BytesU::SizeOf(colors), EBufferUsage::Uniform },
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
		resources0.BindTexture( UniformID{"un_ColorTexture"},  texture, sampler );
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

		
		CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{}.SetDebugFlags( EDebugFlags::Default ));
		CHECK_ERR( cmd );

		LogicalPassID	render_pass	= cmd->CreateRenderPass( RenderPassDesc( view_size )
											.AddTarget( RenderTargetID(0), image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
											.AddViewport( view_size ) );
		
		cmd->AddTask( render_pass, CustomDraw{ [&] (IDrawContext &ctx)
											{
												RenderState::InputAssemblyState	ia;
												ia.topology = EPrimitive::TriangleList;

												ctx.SetInputAssembly( ia );
												ctx.BindPipeline( pipeline );
												ctx.BindResources( DescriptorSetID{"PerObject"}, resources0 );
												ctx.BindResources( DescriptorSetID{"PerPass"},   resources1 );
												ctx.DrawVertices( 3 );
											}}.AddImage( texture, EResourceState::ShaderSample ));
		
		Task	t_clear	= cmd->AddTask( ClearColorImage{}.SetImage( texture ).AddRange( 0_mipmap, 1, 0_layer, 1 ).Clear(RGBA32f{HtmlColor::White}) );
		Task	t_draw	= cmd->AddTask( SubmitRenderPass{ render_pass }.DependsOn( t_clear ));
		Task	t_read	= cmd->AddTask( ReadImage().SetImage( image, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_draw ) );
		FG_UNUSED( t_read );

		CHECK_ERR( _frameGraph->Execute( cmd ));
		CHECK_ERR( _frameGraph->WaitIdle() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( data_is_correct );
		
		DeleteResources( image, texture, pipeline, positions_ub, colors_ub );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

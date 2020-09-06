// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_ReadAttachment1 ()
	{
		if ( not _pplnCompiler )
		{
			FG_LOGI( TEST_NAME << " - skipped" );
			return true;
		}

		GraphicsPipelineDesc	ppln;

		ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location=0) out vec3  v_Color;

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

layout(binding=0) uniform sampler2D  un_DepthImage;

layout(location=0) out vec4  out_Color;

layout(location=0) in  vec3  v_Color;

void main() {
	out_Color = vec4(v_Color * texelFetch(un_DepthImage, ivec2(gl_FragCoord.xy), 0).r, 1.0);
}
)#" );
		
		const uint2		view_size	= {800, 600};
		EPixelFormat	ds_format	= Default;
		
		// find suitable depth format
		const Tuple<EPixelFormat, String> formats[] = {
			{ EPixelFormat::Depth24_Stencil8,	"Depth24_Stencil8" },
			{ EPixelFormat::Depth32F_Stencil8,	"Depth32F_Stencil8" },
			{ EPixelFormat::Depth16_Stencil8,	"Depth16_Stencil8" },
			{ EPixelFormat::Depth16,			"Depth16" }
		};

		for (auto fmt : formats)
		{
			ImageDesc	desc;
			desc.SetDimension( view_size );
			desc.SetFormat( std::get<0>(fmt) );
			desc.SetUsage( EImageUsage::Sampled | EImageUsage::TransferDst | EImageUsage::DepthStencilAttachment );

			if ( _frameGraph->IsSupported( desc ))
			{
				ds_format = std::get<0>(fmt);
				FG_LOGI( "Used depth format: " + std::get<1>(fmt) );
				break;
			}
		}
		CHECK_ERR( ds_format != Default );

		ImageID			color_image	= _frameGraph->CreateImage( ImageDesc{}.SetDimension( view_size ).SetFormat( EPixelFormat::RGBA8_UNorm )
																			.SetUsage( EImageUsage::ColorAttachment | EImageUsage::TransferSrc ),
																Default, "ColorTarget" );
		ImageID			depth_image	= _frameGraph->CreateImage( ImageDesc{}.SetDimension( view_size ).SetFormat( ds_format )
																			.SetUsage( EImageUsage::DepthStencilAttachment | EImageUsage::TransferDst | EImageUsage::Sampled ),
																Default, "DepthTarget" );

		SamplerID		sampler		= _frameGraph->CreateSampler( SamplerDesc{} );

		GPipelineID		pipeline	= _frameGraph->CreatePipeline( ppln );
		CHECK_ERR( color_image and depth_image and sampler and pipeline );

		PipelineResources	resources;
		CHECK_ERR( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources ));

		
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

		
		CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{}.SetDebugFlags( EDebugFlags::Default ));
		CHECK_ERR( cmd );

		LogicalPassID	render_pass	= cmd->CreateRenderPass( RenderPassDesc( view_size )
												.AddTarget( RenderTargetID::Color_0, color_image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
												.AddTarget( RenderTargetID::Depth, depth_image, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
												.SetDepthTestEnabled( true ).SetDepthWriteEnabled( false )
												.AddViewport( view_size ));
		
		ImageViewDesc	view_desc;	view_desc.aspectMask = EImageAspect::Depth;
		resources.BindTexture( UniformID("un_DepthImage"), depth_image, sampler, view_desc );

		cmd->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( pipeline )
												.SetTopology( EPrimitive::TriangleList )
												.AddResources( DescriptorSetID("0"), &resources ));

		Task	t_clear	= cmd->AddTask( ClearDepthStencilImage{}.SetImage( depth_image ).Clear( 1.0f ).AddRange( 0_mipmap, 1, 0_layer, 1 ));
		Task	t_draw	= cmd->AddTask( SubmitRenderPass{ render_pass }.DependsOn( t_clear ));
		Task	t_read	= cmd->AddTask( ReadImage().SetImage( color_image, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_draw ));
		Unused( t_read );

		CHECK_ERR( _frameGraph->Execute( cmd ));
		CHECK_ERR( _frameGraph->WaitIdle() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( data_is_correct );

		DeleteResources( color_image, depth_image, sampler, pipeline );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

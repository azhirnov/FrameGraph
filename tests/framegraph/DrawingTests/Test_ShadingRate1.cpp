// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_ShadingRate1 ()
	{
		if ( not _vulkan.GetDeviceShadingRateImageFeatures().shadingRateImage )
			return true;

		GraphicsPipelineDesc	ppln;

		ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
const vec2	g_Positions[3] = vec2[](
	vec2(-1.0, -1.0),
	vec2(-1.0,  3.0),
	vec2( 3.0, -1.0)
);

void main() {
	gl_Position	= vec4( g_Positions[gl_VertexIndex], 0.0, 1.0 );
}
)#" );
		
		ppln.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_110, "main", R"#(
#extension GL_NV_shading_rate_image : require

out vec4	out_Color;

//in ivec2 gl_FragmentSizeNV;
//in int   gl_InvocationsPerPixelNV;

const vec3 g_Colors[] = {
	vec3( 1.0, 0.0, 0.0 ),	// 0
	vec3( 0.5, 0.0, 0.0 ),	// 1
	vec3( 0.0 ),
	vec3( 0.0, 1.0, 0.0 ),	// 3
	vec3( 0.0, 0.5, 0.0 ),	// 4
	vec3( 0.0, 0.0, 1.0 ),	// 5
	vec3( 0.0 ),
	vec3( 0.0, 0.0, 0.5 ),	// 7
	vec3( 0.0 ),
	vec3( 0.0 ),
	vec3( 0.0 ),
	vec3( 0.0 ),
	vec3( 1.0, 1.0, 0.0 ),	// 12
	vec3( 0.5, 1.0, 0.0 ),	// 13
	vec3( 0.0 ),
	vec3( 0.0, 1.0, 1.0 )	// 15
};

void main ()
{
	int	index = (gl_FragmentSizeNV.x-1) + (gl_FragmentSizeNV.y-1) * 4;	// 0, 1, 3, 4, 5, 7, 12, 13, 15

	out_Color = vec4( g_Colors[ clamp(index, 0, 16) ], 1.0f );
}
)#" );

		const EShadingRatePalette	shading_rate_palette[] = {
			EShadingRatePalette::Block_1x1_2,
			EShadingRatePalette::Block_2x1_1,
			EShadingRatePalette::Block_2x2_1,
			EShadingRatePalette::Block_2x4_1,
			EShadingRatePalette::Block_4x4_1
		};
		CHECK( CountOf(shading_rate_palette) <= _vulkan.GetDeviceShadingRateImageProperties().shadingRatePaletteSize );


		FGThreadPtr		frame_graph	= _fgGraphics1;

		GPipelineID		pipeline	= frame_graph->CreatePipeline( ppln );
		CHECK_ERR( pipeline );

		const uint2		view_size	= {256, 256};
		const auto&		sri_size	= _vulkan.GetDeviceShadingRateImageProperties().shadingRateTexelSize;
		const uint2		sr_img_size	= { view_size.x / sri_size.width, view_size.y / sri_size.height };

		ImageID			image		= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::ColorAttachment | EImageUsage::TransferSrc }, Default, "RenderTarget" );

		ImageID			shading_rate = frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{sr_img_size.x, sr_img_size.y, 1}, EPixelFormat::R8U,
																			EImageUsage::ShadingRate | EImageUsage::TransferDst }, Default, "ShadingRate" );

		Array<uint8_t>	sri_data;	sri_data.resize( sr_img_size.x * sr_img_size.y );

		for (uint y = 0; y < sr_img_size.y; ++y)
		for (uint x = 0; x < sr_img_size.x; ++x)
		{
			sri_data[x + y * sr_img_size.x] = uint8_t((x/2) % CountOf(shading_rate_palette));
		}

		
		bool		data_is_correct = false;

		const auto	OnLoaded =	[&sri_size, &sri_data, OUT &data_is_correct] (const ImageView &imageData)
		{
			const RGBA32f	colors[] = { RGBA32f{ 1.0f, 0.0f, 0.0f, 1.0f }, RGBA32f{ 0.5f, 0.0f, 0.0f, 1.0f },
										 RGBA32f{ 0.0f, 0.0f, 1.0f, 1.0f }, RGBA32f{ 0.5f, 1.0f, 0.0f, 1.0f },
										 RGBA32f{ 0.0f, 1.0f, 1.0f, 1.0f } };
			data_is_correct = true;

			for (uint y = 0; y < imageData.Dimension().y; ++y)
			for (uint x = 0; x < imageData.Dimension().x; ++x)
			{
				RGBA32f	rhs;
				imageData.Load( uint3(x, y, 0), OUT rhs );

				const uint	idx = (x / sri_size.width) + (y / sri_size.height) * (imageData.Dimension().x / sri_size.width);
				RGBA32f		lhs	= colors[ sri_data[idx] ];
				const bool	eq  = All(Equals( lhs, rhs, 0.1f ));
				ASSERT( eq );
				data_is_correct &= eq;
			}
		};

		
		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
		CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
		CHECK_ERR( frame_graph->Begin( batch_id, 0, EThreadUsage::Graphics ));

		LogicalPassID		render_pass	= frame_graph->CreateRenderPass( RenderPassDesc( view_size )
												.AddTarget( RenderTargetID("out_Color"), image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
												.AddViewport( view_size, 0.0f, 1.0f, shading_rate_palette )
												.SetShadingRateImage( shading_rate ));
		
		frame_graph->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( pipeline ).SetTopology( EPrimitive::TriangleList ));

		Task	t_update	= frame_graph->AddTask( UpdateImage{}.SetImage( shading_rate ).SetData( sri_data, sr_img_size, 1_b * sr_img_size.x ));
		Task	t_draw		= frame_graph->AddTask( SubmitRenderPass{ render_pass }.DependsOn( t_update ));
		Task	t_read		= frame_graph->AddTask( ReadImage().SetImage( image, int2(), view_size ).SetCallback( OnLoaded ).DependsOn( t_draw ));
		FG_UNUSED( t_read );

		CHECK_ERR( frame_graph->Execute() );
		CHECK_ERR( _fgInstance->EndFrame() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( _fgInstance->WaitIdle() );

		CHECK_ERR( data_is_correct );

		DeleteResources( image, pipeline, shading_rate );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

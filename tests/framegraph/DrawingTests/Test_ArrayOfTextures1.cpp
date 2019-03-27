// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_ArrayOfTextures1 ()
	{
		ComputePipelineDesc	ppln;

		ppln.AddShader( EShaderLangFormat::VKSL_100, "main", R"#(
#version 460 core
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x = 8, local_size_y = 1, local_size_z = 1) in;

layout(binding=0) uniform sampler2D  un_Textures[8];

layout(binding=1) writeonly uniform image2D  un_OutImage;

void main ()
{
	const int	i		= int(gl_LocalInvocationIndex);
	const vec2	coord	= vec2(gl_GlobalInvocationID.xy) / vec2(gl_WorkGroupSize.xy * gl_NumWorkGroups.xy - 1);
		  vec4	color	= texture( un_Textures[i], coord );

	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), color );
}
)#" );
		
		const uint2		image_dim	= { 32, 32 };
		const uint2		tex_dim		= { 16, 16 };

		ImageID			textures[8];
		for (size_t i = 0; i < CountOf(textures); ++i) {
			textures[i] = _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{tex_dim.x, tex_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
															   EImageUsage::Sampled | EImageUsage::TransferDst }, Default, "Texture-" + ToString(i) );
		}

		ImageID			dst_image	= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{image_dim.x, image_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::Storage | EImageUsage::TransferSrc }, Default, "OutImage" );
		
		SamplerID		sampler		= _frameGraph->CreateSampler( SamplerDesc{} );

		CPipelineID		pipeline	= _frameGraph->CreatePipeline( ppln );
		CHECK_ERR( pipeline );
		
		PipelineResources	resources;
		CHECK_ERR( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID{"0"}, OUT resources ));
		

		bool	data_is_correct	= false;

		const auto	OnLoaded =	[OUT &data_is_correct] (const ImageView &imageData)
		{
			const auto	TestPixel = [&imageData] (uint x, uint y, const RGBA32f &color)
			{
				RGBA32f	col;
				imageData.Load( uint3{x, y, 0}, OUT col );

				bool	is_equal = All(Equals( col, color, 0.1f ));
				ASSERT( is_equal );
				return is_equal;
			};

			data_is_correct	= true;
			data_is_correct &= TestPixel(  0,  4, RGBA32f{RGBA8u{ 255,   0,   0, 255 }} );
			data_is_correct &= TestPixel(  1,  5, RGBA32f{RGBA8u{ 255, 191,   0, 255 }} );
			data_is_correct &= TestPixel(  2,  6, RGBA32f{RGBA8u{ 127, 255,   0, 255 }} );
			data_is_correct &= TestPixel(  3,  7, RGBA32f{RGBA8u{   0, 255,  64, 255 }} );
			data_is_correct &= TestPixel(  4,  8, RGBA32f{RGBA8u{   0, 255, 255, 255 }} );
			data_is_correct &= TestPixel(  5,  1, RGBA32f{RGBA8u{   0,  64, 255, 255 }} );
			data_is_correct &= TestPixel(  6,  2, RGBA32f{RGBA8u{ 127,   0, 255, 255 }} );
			data_is_correct &= TestPixel(  7,  3, RGBA32f{RGBA8u{ 255,   0, 191, 255 }} );
			
			data_is_correct &= TestPixel(  8, 20, RGBA32f{RGBA8u{ 255,   0,   0, 255 }} );
			data_is_correct &= TestPixel(  9, 26, RGBA32f{RGBA8u{ 255, 191,   0, 255 }} );
			data_is_correct &= TestPixel( 10, 13, RGBA32f{RGBA8u{ 127, 255,   0, 255 }} );
			data_is_correct &= TestPixel( 11,  9, RGBA32f{RGBA8u{   0, 255,  64, 255 }} );
			data_is_correct &= TestPixel( 12, 18, RGBA32f{RGBA8u{   0, 255, 255, 255 }} );
			data_is_correct &= TestPixel( 13,  7, RGBA32f{RGBA8u{   0,  64, 255, 255 }} );
			data_is_correct &= TestPixel( 14,  8, RGBA32f{RGBA8u{ 127,   0, 255, 255 }} );
			data_is_correct &= TestPixel( 15, 22, RGBA32f{RGBA8u{ 255,   0, 191, 255 }} );
		};

		
		CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{}.SetDebugFlags( EDebugFlags::Default ));
		CHECK_ERR( cmd );
		
		resources.BindTextures( UniformID("un_Textures"), textures, sampler );
		resources.BindImage( UniformID{"un_OutImage"}, dst_image );
		
		Task	t_update;
		for (size_t i = 0; i < CountOf(textures); ++i)
		{
			RGBA32f	color{ HSVColor{ float(i) / CountOf(textures) }};

			t_update = cmd->AddTask( ClearColorImage{}.SetImage( textures[i] ).AddRange( 0_mipmap, 1, 0_layer, 1 )
															  .Clear( color ).DependsOn( t_update ));
		}

		Task	t_comp	= cmd->AddTask( DispatchCompute().SetPipeline( pipeline ).Dispatch({ image_dim.x / 8, image_dim.y })
																.AddResources( DescriptorSetID{"0"}, &resources ).DependsOn( t_update ));
		
		Task	t_read	= cmd->AddTask( ReadImage().SetImage( dst_image, int2(), image_dim ).SetCallback( OnLoaded ).DependsOn( t_comp ));
		FG_UNUSED( t_read );
		
		CHECK_ERR( _frameGraph->Execute( cmd ));
		CHECK_ERR( _frameGraph->WaitIdle() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( data_is_correct );
		
		DeleteResources( pipeline, dst_image, sampler );

		for (auto& tex : textures) {
			DeleteResources( tex );
		}

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

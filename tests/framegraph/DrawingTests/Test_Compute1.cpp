// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_Compute1 ()
	{
		ComputePipelineDesc	ppln;

		ppln.AddShader( EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(compute)
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x_id = 0, local_size_y_id = 1, local_size_z = 1) in;
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba8) writeonly uniform image2D  un_OutImage;

void main ()
{
	vec4 fragColor = vec4(float(gl_LocalInvocationID.x) / float(gl_WorkGroupSize.x),
						  float(gl_LocalInvocationID.y) / float(gl_WorkGroupSize.y),
						  1.0, 0.0);

	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), fragColor );
}
)#" );
		
		const uint2		image_dim	= { 16, 16 };

		ImageID			image0		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{image_dim.x, image_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::Storage | EImageUsage::TransferSrc }, Default, "MyImage_0" );

		ImageID			image1		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{image_dim.x, image_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::Storage | EImageUsage::TransferSrc }, Default, "MyImage_1" );

		ImageID			image2		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{image_dim.x, image_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::Storage | EImageUsage::TransferSrc }, Default, "MyImage_2" );

		CPipelineID		pipeline	= _frameGraph->CreatePipeline( ppln );
		CHECK_ERR( pipeline );
		
		PipelineResources	resources;
		CHECK_ERR( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources ));

		
		const auto	CheckData = [] (const ImageView &imageData, uint blockSize, OUT bool &isCorrect)
		{
			isCorrect = true;

			for (uint y = 0; y < imageData.Dimension().y; ++y)
			{
				for (uint x = 0; x < imageData.Dimension().x; ++x)
				{
					RGBA32u		dst;
					imageData.Load( uint3(x,y,0), OUT dst );

					const uint	r	= uint(float(x % blockSize) * 255.0f / float(blockSize) + 0.5f);
					const uint	g	= uint(float(y % blockSize) * 255.0f / float(blockSize) + 0.5f);

					const bool	is_equal = (Equals( dst[0], r, 1u )	and
											Equals( dst[1], g, 1u )	and
											dst[2] == 255			and
											dst[3] == 0);

					ASSERT( is_equal );
					isCorrect &= is_equal;
				}
			}
		};
		
		bool	data0_is_correct = false;
		bool	data1_is_correct = false;
		bool	data2_is_correct = false;

		const auto	OnLoaded0 =	[&CheckData, OUT &data0_is_correct] (const ImageView &imageData) {
			CheckData( imageData, 8, OUT data0_is_correct );
		};
		const auto	OnLoaded1 =	[&CheckData, OUT &data1_is_correct] (const ImageView &imageData) {
			CheckData( imageData, 4, OUT data1_is_correct );
		};
		const auto	OnLoaded2 =	[&CheckData, OUT &data2_is_correct] (const ImageView &imageData) {
			CheckData( imageData, 16, OUT data2_is_correct );
		};

		
		CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{}.SetDebugFlags( EDebugFlags::Default ));
		CHECK_ERR( cmd );
		
		resources.BindImage( UniformID("un_OutImage"), image0 );
		Task	t_run0	= cmd->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources ).Dispatch({ 2, 2 }) );

		resources.BindImage( UniformID("un_OutImage"), image1 );
		Task	t_run1	= cmd->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources ).SetLocalSize( 4, 4 ).Dispatch({ 4, 4 }) );
		
		resources.BindImage( UniformID("un_OutImage"), image2 );
		Task	t_run2	= cmd->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources ).SetLocalSize( 16, 16 ).Dispatch({ 1, 1 }) );
		
		Task	t_read0	= cmd->AddTask( ReadImage().SetImage( image0, int2(), image_dim ).SetCallback( OnLoaded0 ).DependsOn( t_run0 ) );
		Task	t_read1	= cmd->AddTask( ReadImage().SetImage( image1, int2(), image_dim ).SetCallback( OnLoaded1 ).DependsOn( t_run1 ) );
		Task	t_read2	= cmd->AddTask( ReadImage().SetImage( image2, int2(), image_dim ).SetCallback( OnLoaded2 ).DependsOn( t_run2 ) );
		
		FG_UNUSED( t_read0 and t_read1 and t_read2 );
		
		CHECK_ERR( _frameGraph->Execute( cmd ));
		CHECK_ERR( _frameGraph->WaitIdle() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( data0_is_correct and data1_is_correct and data2_is_correct );
		
		DeleteResources( pipeline, image0, image1, image2 );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This test affects:
		- frame graph building and execution
		- tasks: DispatchCompute, ReadImage
		- resources: image, pipeline, pipeline resources
		- specialization constants in compute pipeline local group size
		- staging buffers
		- memory managment
*/

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
		
		FGThreadPtr		frame_graph	= _fgGraphics1;
		const uint2		image_dim	= { 16, 16 };

		ImageID			image0		= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{image_dim.x, image_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::Storage | EImageUsage::TransferSrc }, Default, "MyImage_0" );

		ImageID			image1		= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{image_dim.x, image_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::Storage | EImageUsage::TransferSrc }, Default, "MyImage_1" );

		ImageID			image2		= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{image_dim.x, image_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::Storage | EImageUsage::TransferSrc }, Default, "MyImage_2" );

		CPipelineID		pipeline	= frame_graph->CreatePipeline( std::move(ppln) );
		
		PipelineResources	resources;
		CHECK_ERR( frame_graph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources ));

		
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

		
		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
		CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
		CHECK_ERR( frame_graph->Begin( batch_id, 0, EThreadUsage::Graphics ));
		
		resources.BindImage( UniformID("un_OutImage"), image0 );
		Task	t_run0	= frame_graph->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources ).SetGroupCount( 2, 2 ) );

		resources.BindImage( UniformID("un_OutImage"), image1 );
		Task	t_run1	= frame_graph->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources ).SetLocalSize( 4, 4 ).SetGroupCount( 4, 4 ) );
		
		resources.BindImage( UniformID("un_OutImage"), image2 );
		Task	t_run2	= frame_graph->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources ).SetLocalSize( 16, 16 ).SetGroupCount( 1, 1 ) );
		
		Task	t_read0	= frame_graph->AddTask( ReadImage().SetImage( image0, int2(), image_dim ).SetCallback( OnLoaded0 ).DependsOn( t_run0 ) );
		Task	t_read1	= frame_graph->AddTask( ReadImage().SetImage( image1, int2(), image_dim ).SetCallback( OnLoaded1 ).DependsOn( t_run1 ) );
		Task	t_read2	= frame_graph->AddTask( ReadImage().SetImage( image2, int2(), image_dim ).SetCallback( OnLoaded2 ).DependsOn( t_run2 ) );
		
		FG_UNUSED( t_read0 and t_read1 and t_read2 );
		
		CHECK_ERR( frame_graph->Execute() );		
		CHECK_ERR( _fgInstance->EndFrame() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		CHECK_ERR( _fgInstance->WaitIdle() );

		CHECK_ERR( data0_is_correct and data1_is_correct and data2_is_correct );
		
		DeleteResources( pipeline, image0, image1, image2 );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

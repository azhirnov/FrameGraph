// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This test affects:
		...
*/

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_InvalidID ()
	{
		ComputePipelineDesc	ppln;

		ppln.AddShader( EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(compute)
#extension GL_ARB_shading_language_420pack : enable

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
		
		FGThreadPtr		frame_graph	= _fgThreads[0];
		const uint2		image_dim	= { 16, 16 };

		ImageID			image0		= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{image_dim.x, image_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::Storage | EImageUsage::TransferSrc }, Default, "MyImage_0" );

		ImageID			image1		= frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{image_dim.x, image_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::Storage | EImageUsage::TransferSrc }, Default, "MyImage_1" );

		ImageID			image2		{ image0.Get() };
		ImageID			image3		{RawImageID{ 1111, 0 }};
		ImageID			image4		{RawImageID{ 2222, 0 }};

		CPipelineID		pipeline	= frame_graph->CreatePipeline( ppln );
		
		PipelineResources	resources;
		CHECK_ERR( frame_graph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources ));

		
		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
		auto&	desc = frame_graph->GetDescription( image3 );
		FG_LOGI( ToString(desc.dimension.x) );

		// frame 1
		{
			CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
			CHECK_ERR( frame_graph->Begin( batch_id, 0, EQueueUsage::Graphics ));
		
			resources.BindImage( UniformID("un_OutImage"), image0 );
			Task	t_run	= frame_graph->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources ) );
			Task	t_copy	= frame_graph->AddTask( CopyImage().From( image0 ).To( image4 ).AddRegion({}, int2(), {}, int2(), image_dim) );
			FG_UNUSED( t_run );
		
			CHECK_ERR( frame_graph->Execute() );		
			CHECK_ERR( _fgInstance->EndFrame() );
		}

		frame_graph->ReleaseResource( image0 );
		//frame_graph->ReleaseResource( image3 );	// TODO
		
		// frame 1
		{
			CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
			CHECK_ERR( frame_graph->Begin( batch_id, 0, EQueueUsage::Graphics ));
			
			Task	t_run	= frame_graph->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources ) );
			Task	t_copy	= frame_graph->AddTask( CopyImage().From( image2 ).To( image1 ).AddRegion({}, int2(), {}, int2(), image_dim) );
			FG_UNUSED( t_run, t_copy );

			CHECK_ERR( frame_graph->Execute() );		
			CHECK_ERR( _fgInstance->EndFrame() );
		}

		CHECK_ERR( _fgInstance->WaitIdle() );
		
		DeleteResources( pipeline, image1, image2 );
		FG_UNUSED( image3.Release() );
		FG_UNUSED( image4.Release() );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

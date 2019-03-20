// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
		
		const uint2		image_dim	= { 16, 16 };

		ImageID			image0		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{image_dim.x, image_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::Storage | EImageUsage::TransferSrc }, Default, "MyImage_0" );

		ImageID			image1		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{image_dim.x, image_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::Storage | EImageUsage::TransferSrc }, Default, "MyImage_1" );

		ImageID			image2		{ image0.Get() };
		ImageID			image3		{RawImageID{ 1111, 0 }};
		ImageID			image4		{RawImageID{ 2222, 0 }};

		CPipelineID		pipeline	= _frameGraph->CreatePipeline( ppln );
		
		PipelineResources	resources;
		CHECK_ERR( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources ));
		
		auto&	desc = _frameGraph->GetDescription( image3 );
		FG_LOGI( ToString(desc.dimension.x) );

		// frame 1
		CommandBuffer	cmd1 = _frameGraph->Begin( CommandBufferDesc{} );
		CHECK_ERR( cmd1 );
		{
			resources.BindImage( UniformID("un_OutImage"), image0 );
			Task	t_run	= cmd1->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources ) );
			Task	t_copy	= cmd1->AddTask( CopyImage().From( image0 ).To( image4 ).AddRegion({}, int2(), {}, int2(), image_dim) );
			FG_UNUSED( t_run );
		
			CHECK_ERR( _frameGraph->Execute( cmd1 ));
		}

		_frameGraph->ReleaseResource( image0 );
		//_frameGraph->ReleaseResource( image3 );	// TODO
		
		// frame 2
		CommandBuffer	cmd2 = _frameGraph->Begin( CommandBufferDesc{} );
		CHECK_ERR( cmd2 );
		{
			Task	t_run	= cmd2->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources ) );
			Task	t_copy	= cmd2->AddTask( CopyImage().From( image2 ).To( image1 ).AddRegion({}, int2(), {}, int2(), image_dim) );
			FG_UNUSED( t_run, t_copy );

			CHECK_ERR( _frameGraph->Execute( cmd2 ));
		}

		CHECK_ERR( _frameGraph->WaitIdle() );
		
		DeleteResources( pipeline, image1, image2 );
		FG_UNUSED( image3.Release() );
		FG_UNUSED( image4.Release() );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

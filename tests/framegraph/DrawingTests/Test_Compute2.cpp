// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_Compute2 ()
	{
		ComputePipelineDesc	ppln;

		ppln.AddShader( EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(compute)
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(set=2, binding=0, rgba8) writeonly uniform image2D  un_OutImage;

void main ()
{
	vec4 fragColor = vec4(float(gl_LocalInvocationID.x) / float(gl_WorkGroupSize.x),
						  float(gl_LocalInvocationID.y) / float(gl_WorkGroupSize.y),
						  1.0, 0.0);

	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), fragColor );
}
)#" );
		
		const uint2		image_dim	= { 16, 16 };

		ImageID			image		= _frameGraph->CreateImage( ImageDesc{ EImage::Tex2D, uint3{image_dim.x, image_dim.y, 1}, EPixelFormat::RGBA8_UNorm,
																		   EImageUsage::Storage | EImageUsage::TransferSrc }, Default, "MyImage_0" );

		CPipelineID		pipeline	= _frameGraph->CreatePipeline( ppln );
		CHECK_ERR( pipeline );
		
		PipelineResources	resources;
		CHECK_ERR( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID("2"), OUT resources ));


		bool	data_is_correct = true;
		const auto	OnLoaded =	[OUT &data_is_correct] (const ImageView &imageData)
		{
			const uint	block_size = 8;
			for (uint y = 0; y < imageData.Dimension().y; ++y)
			{
				for (uint x = 0; x < imageData.Dimension().x; ++x)
				{
					RGBA32u		dst;
					imageData.Load( uint3(x,y,0), OUT dst );

					const uint	r	= uint(float(x % block_size) * 255.0f / float(block_size) + 0.5f);
					const uint	g	= uint(float(y % block_size) * 255.0f / float(block_size) + 0.5f);

					const bool	is_equal = (Equals( dst[0], r, 1u )	and
											Equals( dst[1], g, 1u )	and
											dst[2] == 255			and
											dst[3] == 0);

					ASSERT( is_equal );
					data_is_correct &= is_equal;
				}
			}
		};

		
		CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{}.SetDebugFlags( ECompilationDebugFlags::Default ));
		CHECK_ERR( cmd );
		
		resources.BindImage( UniformID("un_OutImage"), image );

		Task	t_run	= cmd->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("2"), &resources ).Dispatch({ 2, 2 }) );
		Task	t_read	= cmd->AddTask( ReadImage().SetImage( image, int2(), image_dim ).SetCallback( OnLoaded ).DependsOn( t_run ));
		FG_UNUSED( t_read );
		
		CHECK_ERR( _frameGraph->Execute( cmd ));
		CHECK_ERR( _frameGraph->WaitIdle() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));

		CHECK_ERR( data_is_correct );
		
		DeleteResources( pipeline, image );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

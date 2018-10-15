// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'
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

	bool FGApp::_DrawTest_GLSLCompute ()
	{
		ComputePipelineDesc	ppln;

		ppln.AddShader( EShaderLangFormat::GLSL_450,
						"main",
R"#(
#pragma shader_stage(compute)
#extension GL_ARB_separate_shader_objects : enable
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
		
		ImagePtr				image0		= _CreateImage2D( {16,16}, EPixelFormat::RGBA8_UNorm, "MyImage_0" );
		ImagePtr				image1		= _CreateImage2D( {16,16}, EPixelFormat::RGBA8_UNorm, "MyImage_1" );
		ImagePtr				image2		= _CreateImage2D( {16,16}, EPixelFormat::RGBA8_UNorm, "MyImage_2" );

		PipelinePtr				pipeline	= _frameGraph->CreatePipeline( std::move(ppln) );

		PipelineResourcesPtr	resources0	= pipeline->CreateResources( DescriptorSetID("0") );
		PipelineResourcesPtr	resources1	= pipeline->CreateResources( DescriptorSetID("0") );
		PipelineResourcesPtr	resources2	= pipeline->CreateResources( DescriptorSetID("0") );
		
		resources0->BindImage( UniformID("un_OutImage"), image0 );
		resources1->BindImage( UniformID("un_OutImage"), image1 );
		resources2->BindImage( UniformID("un_OutImage"), image2 );

		
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


		_frameGraph->Begin();
		
		Task	t_run0	= _frameGraph->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), resources0 ).SetGroupCount( 2, 2 ) );
		Task	t_run1	= _frameGraph->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), resources1 ).SetLocalSize( 4, 4 ).SetGroupCount( 4, 4 ) );
		Task	t_run2	= _frameGraph->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), resources2 ).SetLocalSize( 16, 16 ).SetGroupCount( 1, 1 ) );
		
		Task	t_read0	= _frameGraph->AddTask( ReadImage().SetImage( image0, int3(), image0->Dimension() ).SetCallback( OnLoaded0 ).DependsOn( t_run0 ) );
		Task	t_read1	= _frameGraph->AddTask( ReadImage().SetImage( image1, int3(), image1->Dimension() ).SetCallback( OnLoaded1 ).DependsOn( t_run1 ) );
		Task	t_read2	= _frameGraph->AddTask( ReadImage().SetImage( image2, int3(), image2->Dimension() ).SetCallback( OnLoaded2 ).DependsOn( t_run2 ) );
		
        (void)(t_read0 and t_read1 and t_read2);
		
		CHECK( _frameGraph->Compile() );		
		CHECK( _frameGraph->Execute() );

		_frameGraph->WaitIdle();

		CHECK_ERR( data0_is_correct and data1_is_correct and data2_is_correct );

        FG_LOGI( "DrawTest_GLSLCompute - passed" );
		return true;
	}

}	// FG

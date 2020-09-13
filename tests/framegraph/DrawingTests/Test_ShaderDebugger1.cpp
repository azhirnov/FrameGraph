// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
*/

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_ShaderDebugger1 ()
	{
		if ( not _hasShaderDebugger or not _pplnCompiler )
		{
			FG_LOGI( TEST_NAME << " - skipped" );
			return true;
		}

		ComputePipelineDesc	ppln;

		ppln.AddShader( EShaderLangFormat::VKSL_100 | EShaderLangFormat::EnableDebugTrace, "main", R"#(
#pragma shader_stage(compute)
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding=0, r32f) writeonly uniform image2D  un_OutImage;

void main ()
{
	uint	index	= gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;
	uint	size	= gl_NumWorkGroups.x * gl_NumWorkGroups.y * gl_WorkGroupSize.x * gl_WorkGroupSize.y;
	float	value	= fract( float(index) / size );
	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), vec4(value) );
}
)#", "ComputeShader" );
		
		const uint2		image_dim	= { 16, 16 };
		const uint2		debug_coord	= image_dim / 2;

		ImageID			image		= _frameGraph->CreateImage( ImageDesc{}.SetDimension( image_dim ).SetFormat( EPixelFormat::R32F )
																			.SetUsage( EImageUsage::Storage | EImageUsage::TransferSrc ),
																Default, "OutImage" );
		CPipelineID		pipeline	= _frameGraph->CreatePipeline( ppln );
		CHECK_ERR( image and pipeline );
		
		PipelineResources	resources;
		CHECK_ERR( _frameGraph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources ));

		
		bool	data_is_correct				= false;
		bool	shader_output_is_correct	= false;

		const auto	OnShaderTraceReady = [OUT &shader_output_is_correct] (StringView taskName, StringView shaderName, EShaderStages stages, ArrayView<String> output) {
			const char	ref[] = R"#(//> gl_GlobalInvocationID: uint3 {8, 8, 0}
//> gl_LocalInvocationID: uint3 {0, 0, 0}
//> gl_WorkGroupID: uint3 {1, 1, 0}
no source

//> index: uint {136}
//  gl_GlobalInvocationID: uint3 {8, 8, 0}
11. index	= gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;

//> size: uint {256}
12. size	= gl_NumWorkGroups.x * gl_NumWorkGroups.y * gl_WorkGroupSize.x * gl_WorkGroupSize.y;

//> value: float {0.531250}
//  index: uint {136}
//  size: uint {256}
13. value	= fract( float(index) / size );

//> imageStore(): void
//  gl_GlobalInvocationID: uint3 {8, 8, 0}
//  value: float {0.531250}
14. 	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), vec4(value) );

)#";
			shader_output_is_correct = true;
			shader_output_is_correct &= (stages == EShaderStages::Compute);
			shader_output_is_correct &= (taskName == "DebuggableCompute");
			shader_output_is_correct &= (shaderName == "ComputeShader");
			shader_output_is_correct &= (output.size() == 1);
			shader_output_is_correct &= (output.size() ? output[0] == ref : false);
			ASSERT( shader_output_is_correct );
		};
		_frameGraph->SetShaderDebugCallback( OnShaderTraceReady );

		const auto	OnLoaded =	[OUT &data_is_correct, &debug_coord] (const ImageView &imageData) {
			RGBA32f		dst;
			imageData.Load( uint3(debug_coord,0), OUT dst );
			data_is_correct = Equals( dst.r, 0.53125f, 0.000001f );	// must contains same value as in trace
			ASSERT( data_is_correct );
		};
		
		
		CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{}.SetDebugFlags( EDebugFlags::Default ));
		CHECK_ERR( cmd );
		
		resources.BindImage( UniformID("un_OutImage"), image );

		Task	t_comp	= cmd->AddTask( DispatchCompute().SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources )
																.Dispatch({ 2, 2 }).EnableDebugTrace(uint3{ debug_coord, 0 })
																.SetName("DebuggableCompute") );

		Task	t_read	= cmd->AddTask( ReadImage().SetImage( image, int2(), image_dim ).SetCallback( OnLoaded ).DependsOn( t_comp ));
		Unused( t_read );
		
		CHECK_ERR( _frameGraph->Execute( cmd ));
		CHECK_ERR( _frameGraph->WaitIdle() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));

		CHECK_ERR( data_is_correct and shader_output_is_correct );
		
		DeleteResources( pipeline, image );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

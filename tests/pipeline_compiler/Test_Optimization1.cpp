// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Optimization1 (VPipelineCompiler* compiler)
{
	ComputePipelineDesc	ppln1;
	ppln1.AddShader( EShaderLangFormat::VKSL_100, "main", R"#(
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding=0, rgba8) writeonly uniform image2D  un_OutImage;

void main ()
{
	ivec2	coord	= ivec2(gl_GlobalInvocationID.xy);
	float	xsin	= sin(float(coord.x));
	float	ycos	= cos(float(coord.y));
	float	xysin	= sin(float(coord.x + coord.y));

	imageStore( un_OutImage, coord, vec4(xsin, ycos, xysin, 1.0f) );
}
)#" );

	ComputePipelineDesc	ppln2 = ppln1;

	TEST( compiler->Compile( INOUT ppln1, EShaderLangFormat::SPIRV_100 ));

	const EShaderCompilationFlags	old_flags = compiler->GetCompilationFlags();
	compiler->SetCompilationFlags( EShaderCompilationFlags::StrongOptimization );

	TEST( compiler->Compile( INOUT ppln2, EShaderLangFormat::SPIRV_100 ));

	compiler->SetCompilationFlags( old_flags );

	auto	iter1 = ppln1._shader.data.find( EShaderLangFormat::SPIRV_100 );
	TEST( iter1 != ppln1._shader.data.end() );

	auto	iter2 = ppln2._shader.data.find( EShaderLangFormat::SPIRV_100 );
	TEST( iter2 != ppln2._shader.data.end() );

	auto*	shader1 = UnionGetIf< PipelineDescription::SharedShaderPtr<Array<uint>> >( &iter1->second );
	auto*	shader2 = UnionGetIf< PipelineDescription::SharedShaderPtr<Array<uint>> >( &iter2->second );
	TEST( shader1 and shader2 );

	TEST( (*shader1)->GetEntry() == (*shader2)->GetEntry() );
	
	// TODO: fix on CI
#ifndef FG_CI_BUILD
	TEST( (*shader1)->GetData().size() > (*shader2)->GetData().size() );	// optimized code should be smaller
#endif
	
	TEST_PASSED();
}

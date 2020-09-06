// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_ShaderTrace1 (VPipelineCompiler* compiler)
{
#ifdef FG_ENABLE_GLSL_TRACE
	ComputePipelineDesc	ppln;

	ppln.AddShader( EShaderLangFormat::VKSL_100 | EShaderLangFormat::EnableDebugTrace | EShaderLangFormat::EnableTimeMap, "main", R"#(
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

	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));


	auto	iter1 = ppln._shader.data.find( EShaderLangFormat::SPIRV_100 );
	TEST( iter1 != ppln._shader.data.end() );

	auto	iter2 = ppln._shader.data.find( EShaderLangFormat::SPIRV_100 | EShaderLangFormat::EnableDebugTrace );
	TEST( iter2 != ppln._shader.data.end() );
	
	auto	iter3 = ppln._shader.data.find( EShaderLangFormat::SPIRV_100 | EShaderLangFormat::EnableTimeMap );
	TEST( iter3 != ppln._shader.data.end() );
	
	TEST_PASSED();
#endif
}

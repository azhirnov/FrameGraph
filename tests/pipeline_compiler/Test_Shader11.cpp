// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Shader11 (VPipelineCompiler* compiler)
{
	ComputePipelineDesc	ppln;

	ppln.AddShader( EShaderLangFormat::VKSL_100 | EShaderLangFormat::EnableDebugTrace, "main", R"#(
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

	FG_LOGI( "Test_Shader11 - passed" );
}

// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Reflection3 (VPipelineCompiler* compiler)
{
	ComputePipelineDesc	ppln;

	ppln.AddShader( EShaderLangFormat::GLSL_450, "main", R"#(
#pragma shader_stage(compute)

layout (local_size_x=16, local_size_y=8, local_size_z=1) in;

layout(binding=0, rgba8) writeonly uniform image2D  un_OutImage;

layout(binding=1, std140) readonly buffer un_SSBO
{
	vec4 ssb_data;
};

void main ()
{
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / vec2((gl_WorkGroupSize * gl_NumWorkGroups).xy);

	vec4 fragColor = vec4(sin(uv.x), cos(uv.y), 1.0, ssb_data.r);

	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), fragColor );
}
)#" );


	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));

	auto ds = FindDescriptorSet( ppln, DescriptorSetID("0") );
	TEST( ds );

	TEST( TestImageUniform( *ds, UniformID("un_OutImage"), EImage::Tex2D, EPixelFormat::RGBA8_UNorm, EShaderAccess::WriteOnly, 0, EShaderStages::Compute ));
	TEST( TestStorageBuffer( *ds, UniformID("un_SSBO"), 16_b, 0_b, EShaderAccess::ReadOnly, 1, EShaderStages::Compute ));

	TEST(All( ppln._defaultLocalGroupSize == uint3(16, 8, 1) ));
	
	TEST_PASSED();
}

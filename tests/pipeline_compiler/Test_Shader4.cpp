// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Shader4 (VPipelineCompiler* compiler)
{
	ComputePipelineDesc	ppln;

	ppln.AddShader( EShaderLangFormat::GLSL_450, "main", R"#(
#pragma shader_stage(compute)
#extension GL_ARB_separate_shader_objects : enable

layout (local_size_x=16, local_size_y=8, local_size_z=1) in;

struct DynamicBuffer_Struct
{
	ivec2	i2;
	bool	b1;
	vec2	f2;
	ivec3	i3;
	bvec2	b2;
};

layout(binding=0, std140) coherent buffer DynamicBuffer_SSBO
{
	// static part
	vec2	f2;
	ivec4	i4;

	// dynamic part
	DynamicBuffer_Struct	arr[];

} ssb;

void main ()
{
	int idx = int(gl_GlobalInvocationID.x);

	ssb.arr[idx].i2 += ivec2(ssb.i4.x, idx);
	ssb.arr[idx].b1 = ((idx & 1) == 0);
	ssb.arr[idx].f2 -= vec2(ssb.f2);
	ssb.arr[idx].i3.xy *= 2;
	ssb.arr[idx].i3.z = ~ssb.arr[idx].i3.z;
	ssb.arr[idx].b2 = not(ssb.arr[idx].b2);
}
)#" );


	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));

	auto ds = FindDescriptorSet( ppln, DescriptorSetID("0") );
	TEST( ds );

	TEST( TestStorageBuffer( *ds, UniformID("DynamicBuffer_SSBO"), 32_b, 64_b, EShaderAccess::ReadWrite, 0, EShaderStages::Compute ));

	TEST(All( ppln._defaultLocalGroupSize == uint3(16, 8, 1) ));

	FG_LOGI( "Test_Shader4 - passed" );
}

// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Annotation3 (VPipelineCompiler* compiler)
{
	ComputePipelineDesc	ppln;

	ppln.AddShader( EShaderLangFormat::GLSL_450, "main", R"#(
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// @set 0 test, @dynamic-offset
layout (std140, binding=1, std140) uniform UB {
	vec4	data[4];
} ub;

// @dynamic-offset, @discard
layout (std430, binding=0, std430) restrict writeonly buffer SSB {
	vec4	data[4];
} ssb;

layout(push_constant, std140) uniform PC {
	vec4	data;
} pc;

void main ()
{
	ssb.data[0] = ub.data[1] + pc.data[3];
	ssb.data[3] = ub.data[2] + pc.data[1];
	ssb.data[1] = ub.data[3] + pc.data[0];
	ssb.data[2] = ub.data[0] + pc.data[2];
}
)#" );

	
	const auto	old_flags = compiler->GetCompilationFlags();
	compiler->SetCompilationFlags( old_flags | EShaderCompilationFlags::ParseAnnotations );

	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));
	
	compiler->SetCompilationFlags( old_flags );

	auto ds = FindDescriptorSet( ppln, DescriptorSetID("test") );
	TEST( ds );

	TEST( TestUniformBuffer( *ds, UniformID("UB"),  64_b, 1, EShaderStages::Compute, /*arraySize*/1, /*dynamicOffset*/1 ));
	TEST( TestStorageBuffer( *ds, UniformID("SSB"), 64_b, 0_b, EShaderAccess::WriteDiscard, 0, EShaderStages::Compute, /*arraySize*/1, /*dynamicOffset*/0 ));

	TEST(All( ppln._defaultLocalGroupSize == uint3(1, 1, 1) ));
	
	TEST_PASSED();
}

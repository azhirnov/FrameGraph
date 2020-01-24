// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Shader16 (VPipelineCompiler* compiler)
{
	ComputePipelineDesc	ppln;

	ppln.AddShader( EShaderLangFormat::GLSL_450, "main", R"#(
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (std140, binding=1) uniform UB {
	vec4	data[4];
} ub;

// @dynamic-offset
layout (std430, binding=0) writeonly buffer SSB {
	vec4	data[4];
} ssb;

void main ()
{
	ssb.data[0] = ub.data[1];
	ssb.data[3] = ub.data[2];
	ssb.data[1] = ub.data[3];
	ssb.data[2] = ub.data[0];
}
)#" );

	
	const auto	old_flags = compiler->GetCompilationFlags();
	compiler->SetCompilationFlags( old_flags | EShaderCompilationFlags::ParseAnnotations );

	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));
	
	compiler->SetCompilationFlags( old_flags );

	auto ds = FindDescriptorSet( ppln, DescriptorSetID("0") );
	TEST( ds );

	TEST( TestUniformBuffer( *ds, UniformID("UB"),  64_b, 1, EShaderStages::Compute, /*arraySize*/1, /*dynamicOffset*/UMax ));
	TEST( TestStorageBuffer( *ds, UniformID("SSB"), 64_b, 0_b, EShaderAccess::WriteOnly, 0, EShaderStages::Compute, /*arraySize*/1, /*dynamicOffset*/0 ));

	TEST(All( ppln._defaultLocalGroupSize == uint3(1, 1, 1) ));

	FG_LOGI( "Test_Shader16 - passed" );
}

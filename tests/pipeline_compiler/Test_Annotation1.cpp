// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Annotation1 (VPipelineCompiler* compiler)
{
	ComputePipelineDesc	ppln;

	ppln.AddShader( EShaderLangFormat::GLSL_450, "main", R"#(
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// @set 0 test // comment
layout (std140, set=0, binding=0) uniform UB {
	vec4	data[4];
} ub;

// @set 1 "test 2"  
layout (std430, set=1, binding=0) writeonly buffer SSB {
	vec4	data[4];
} ssb;

// @set 2 test3
layout (std430, set=2, binding=0) writeonly buffer SSB2 {
	vec4	data[4];
} ssb2;

void main ()
{
	ssb.data[0] = ub.data[1];
	ssb.data[3] = ub.data[2];
	ssb.data[1] = ub.data[3];
	ssb.data[2] = ub.data[0];

	ssb2.data[0] = ub.data[1];
	ssb2.data[3] = ub.data[2];
	ssb2.data[1] = ub.data[3];
	ssb2.data[2] = ub.data[0];
}
)#" );


	const auto	old_flags = compiler->GetCompilationFlags();
	compiler->SetCompilationFlags( old_flags | EShaderCompilationFlags::ParseAnnotations );

	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));
	
	compiler->SetCompilationFlags( old_flags );

	TEST( FindDescriptorSet( ppln, DescriptorSetID{"test"} ));
	TEST( FindDescriptorSet( ppln, DescriptorSetID{"test 2"} ));
	TEST( FindDescriptorSet( ppln, DescriptorSetID{"test3"} ));
	
	TEST_PASSED();
}

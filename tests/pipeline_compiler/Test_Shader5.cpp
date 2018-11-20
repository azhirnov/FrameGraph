// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Shader5 (VPipelineCompiler* compiler)
{
	ComputePipelineDesc	ppln;

	ppln.AddShader( EShaderLangFormat::GLSL_450,
				    "main",
R"#(
#pragma shader_stage(compute)
#extension GL_ARB_separate_shader_objects : enable

layout (constant_id = 2) const float SCALE = 0.5f;

layout (local_size_x=8, local_size_y=8, local_size_z=1) in;
layout (local_size_x_id = 0, local_size_y_id = 1) in;

layout(binding=0, rgba8) writeonly uniform image2D  un_OutImage;

layout(binding=1, std140) readonly buffer un_SSBO
{
	vec4 ssb_data;
};

const float uv_scale = 0.1f;

void main ()
{
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / vec2((gl_WorkGroupSize * gl_NumWorkGroups).xy) * (uv_scale + SCALE);

	vec4 fragColor = vec4(sin(uv.x), cos(uv.y), 1.0, ssb_data.r);

	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), fragColor );
}
)#" );


	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::Vulkan_100 | EShaderLangFormat::SPIRV ));

	auto ds = FindDescriptorSet( ppln, DescriptorSetID("0") );
	TEST( ds );

	TEST( TestImageUniform( *ds, UniformID("un_OutImage"), EImage::Tex2D, EPixelFormat::RGBA8_UNorm, EShaderAccess::WriteOnly, 0, EShaderStages::Compute ));
	TEST( TestStorageBuffer( *ds, UniformID("un_SSBO"), 16_b, 0_b, EShaderAccess::ReadOnly, 1, EShaderStages::Compute ));

	TEST(All( ppln._defaultLocalGroupSize == uint3(8, 8, 1) ));
	TEST( ppln._localSizeSpec.x == 0 );
	TEST( ppln._localSizeSpec.y == 1 );
	TEST( ppln._localSizeSpec.z == ~0u );

	TEST( ppln._shader.specConstants.size() == 1 );
	TEST( TestSpecializationConstant( ppln._shader, SpecializationID("SCALE"), 2 ));

    FG_LOGI( "Test_Shader5 - passed" );
}

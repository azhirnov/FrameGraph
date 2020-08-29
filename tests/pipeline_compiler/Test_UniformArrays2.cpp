// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_UniformArrays2 (VPipelineCompiler* compiler)
{
	ComputePipelineDesc	ppln;

	ppln.AddShader( EShaderLangFormat::GLSL_450, "main", R"#(
#version 460 core
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_nonuniform_qualifier : require

layout (local_size_x = 8, local_size_y = 1, local_size_z = 1) in;

layout(binding=0) uniform texture2D  un_Textures[];
layout(binding=2) uniform sampler    un_Sampler;

layout(binding=1) writeonly uniform image2D  un_OutImage;

void main ()
{
	const int	i		= int(gl_LocalInvocationIndex);
	const vec2	coord	= vec2(gl_GlobalInvocationID.xy) / vec2(gl_WorkGroupSize.xy * gl_NumWorkGroups.xy - 1);
		  vec4	color	= texture( sampler2D(un_Textures[nonuniformEXT(i)], un_Sampler), coord );

	imageStore( un_OutImage, ivec2(gl_GlobalInvocationID.xy), color );
}
)#" );

	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));
	
	auto ds = FindDescriptorSet( ppln, DescriptorSetID("0") );
	TEST( ds );
	
	TEST( TestTextureUniform( *ds, UniformID("un_Textures"), EImageSampler::Float2D, /*binding*/0, EShaderStages::Compute, /*arraySize*/0 ));
	TEST( TestSamplerUniform( *ds, UniformID{"un_Sampler"}, 2, EShaderStages::Compute, /*arraySize*/1 ));
	TEST( TestImageUniform( *ds, UniformID{"un_OutImage"}, EImageSampler::Float2D, EShaderAccess::WriteOnly, 1, EShaderStages::Compute, /*arraySize*/1 ));
	
	TEST_PASSED();
}

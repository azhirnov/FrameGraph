// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"
#include "pipeline_compiler/PipelineCppSerializer.h"


extern void Test_Serializer2 (VPipelineCompiler* compiler)
{
	ComputePipelineDesc	ppln;

	ppln.AddShader( EShaderLangFormat::GLSL_450, "main", R"#(
#pragma shader_stage(compute)
#extension GL_ARB_separate_shader_objects : enable

layout (local_size_x=16, local_size_y=8, local_size_z=1) in;

layout (push_constant, std140) uniform PushConst {
	vec4	f0;
	float	f1;
	vec2	f2;
} pc;

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

	ppln._shader.data.clear();


	PipelineCppSerializer	serializer;

	String	src;
	TEST( serializer.Serialize( ppln, "default", OUT src ));

	const String	serialized_ref = R"##(CPipelineID  Create_default (const FGThreadPtr &fg)
{
	ComputePipelineDesc  desc;

	desc.SetLocalGroupSize( 16, 8, 1 );

	desc.AddDescriptorSet(
			DescriptorSetID{"0"},
			0,
			{},
			{},
			{},
			{{ UniformID{"un_OutImage"}, EImage::Tex2D, EPixelFormat::RGBA8_UNorm, EShaderAccess::WriteOnly, BindingIndex{UMax, 0u}, EShaderStages::Compute }},
			{},
			{{ UniformID{"un_SSBO"}, 16_b, 0_b, EShaderAccess::ReadOnly, BindingIndex{UMax, 1u}, EShaderStages::Compute }} );

	desc.SetPushConstants({ {PushConstantID("PushConst"), EShaderStages::Compute, 0_b, 32_b}});

	return fg->CreatePipeline( std::move(desc) );
}
)##";

	TEST( serialized_ref == src );

	FG_LOGI( "Test_Serializer2 - passed" );
}

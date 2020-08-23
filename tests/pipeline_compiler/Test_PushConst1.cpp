// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_PushConst1 (VPipelineCompiler* compiler)
{
	GraphicsPipelineDesc	ppln;

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
#version 450 core
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in  vec2	at_Position;
layout(location=1) in  vec2	at_Texcoord;

layout(location=0) out vec2	v_Texcoord;

layout (push_constant, std140) uniform VSPushConst {
	vec3	f1;
	ivec2	f2;
} pc;

void main() {
	gl_Position	= vec4( at_Position, 0.0, 1.0 );
	v_Texcoord	= at_Texcoord;
}
)#" );

	ppln.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", R"#(
#version 450 core
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout (binding=1, std140) uniform UB {
	vec4	color;
} ub;

layout(binding=0) uniform sampler2D un_ColorTexture;

layout (push_constant, std140) uniform FSPushConst {
	layout(offset = 32)	float	f3;
						ivec2	f4;
	layout(offset = 64) vec4	f5;
} pc;

layout(location=0) in  vec2	v_Texcoord;

layout(location=0) out vec4	out_Color;

void main() {
	out_Color = texture(un_ColorTexture, v_Texcoord) * ub.color;
}
)#" );


	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));
	
	TEST( TestPushConstant( ppln, PushConstantID("VSPushConst"), EShaderStages::Vertex,    0_b, 24_b ));
	TEST( TestPushConstant( ppln, PushConstantID("FSPushConst"), EShaderStages::Fragment, 32_b, 48_b ));

	TEST_PASSED();
}

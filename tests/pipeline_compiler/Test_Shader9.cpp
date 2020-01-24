// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Shader9 (VPipelineCompiler* compiler)
{
	GraphicsPipelineDesc	ppln;

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::GLSL_450, "main", R"#(
#version 450 core
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

in  vec2	at_Position;
in  vec2	at_Texcoord;

out vec2	v_Texcoord;

layout (push_constant, std140) uniform VSPushConst {
						vec3	f1;
						ivec2	f2;
	layout(offset = 32) float	f3;
} pc;

void main() {
	gl_Position	= vec4( at_Position, pc.f3, 1.0 );
	v_Texcoord	= at_Texcoord;
}
)#" );

	ppln.AddShader( EShader::Fragment, EShaderLangFormat::GLSL_450, "main", R"#(
#version 450 core
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout (std140) uniform UB {
	vec4	color;
} ub;

uniform sampler2D un_ColorTexture;

layout (push_constant, std140) uniform FSPushConst {
	layout(offset = 32)	float	f3;
						ivec2	f4;
	layout(offset = 64) vec4	f5;
} pc;

in  vec2	v_Texcoord;

out vec4	out_Color;

void main() {
	out_Color = texture(un_ColorTexture, v_Texcoord) * ub.color;
}
)#" );


	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));
	
	TEST( TestPushConstant( ppln, PushConstantID("VSPushConst"), EShaderStages::Vertex,    0_b, 36_b ));
	TEST( TestPushConstant( ppln, PushConstantID("FSPushConst"), EShaderStages::Fragment, 32_b, 80_b ));

	FG_LOGI( "Test_Shader9 - passed" );
}

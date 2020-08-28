// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_PushConst2 (VPipelineCompiler* compiler)
{
	GraphicsPipelineDesc	ppln;

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::GLSL_450, "main", R"#(
#version 450 core
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in  vec2	at_Position;
layout(location=1) in  vec2	at_Texcoord;

layout(location=0) out vec2	v_Texcoord;

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
	out_Color = texture(un_ColorTexture, v_Texcoord) * ub.color * pc.f3;
}
)#" );
	
	auto	old_flags = compiler->GetCompilationFlags();
	compiler->SetCompilationFlags( EShaderCompilationFlags::Quiet );

	TEST( not compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));
	
	compiler->SetCompilationFlags( old_flags );

	TEST_PASSED();
}

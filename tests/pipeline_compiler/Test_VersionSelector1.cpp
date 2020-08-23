// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_VersionSelector1 (VPipelineCompiler* compiler)
{
	GraphicsPipelineDesc	ppln;

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
#version 450 core
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in  vec2	at_Position;
layout(location=1) in  vec2	at_Texcoord;

layout(location=0) out vec2	v_Texcoord;

void main() {
	gl_Position	= vec4( at_Position, 0.0, 1.0 );
	v_Texcoord	= at_Texcoord;
}
)#" );

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::GLSL_460, "main",
R"#(
#error 1
)#" );

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_110, "main",
R"#(
#error 2
)#" );

	ppln.AddShader( EShader::Fragment, EShaderLangFormat::GLSL_460, "main", R"#(
#version 450 core
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout(binding=0, std140) uniform UB
{
	vec4	color;

} ub;

layout(binding=1) uniform sampler2D un_ColorTexture;

layout(location=0) in  vec2	v_Texcoord;

layout(location=0) out vec4	out_Color;

void main() {
	out_Color = texture(un_ColorTexture, v_Texcoord) * ub.color;
}
)#" );
	
	ppln.AddShader( EShader::Fragment, EShaderLangFormat::GLSL_450, "main",
R"#(
#error 3
)#" );
	
	GraphicsPipelineDesc	ppln1 = ppln;
	GraphicsPipelineDesc	ppln2 = ppln;
	GraphicsPipelineDesc	ppln3 = ppln;

	auto	old_flags = compiler->GetCompilationFlags();
	compiler->SetCompilationFlags( EShaderCompilationFlags::Quiet );

	TEST(     compiler->Compile( INOUT ppln1, EShaderLangFormat::SPIRV_100 ));
	TEST( not compiler->Compile( INOUT ppln2, EShaderLangFormat::SPIRV_110 ));
	

	// restore previous state
	compiler->SetCompilationFlags( old_flags );
	
	TEST_PASSED();
}

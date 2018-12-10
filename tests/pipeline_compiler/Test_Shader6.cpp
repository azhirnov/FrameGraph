// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Shader6 (VPipelineCompiler* compiler)
{
	GraphicsPipelineDesc	ppln;

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
#version 450 core
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

in  vec2	at_Position;
in  vec2	at_Texcoord;

out vec2	v_Texcoord;

void main() {
	gl_Position	= vec4( at_Position, 0.0, 1.0 );
	v_Texcoord	= at_Texcoord;
}
)#" );

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::GLSL_450, "main",
R"#(
#error 1
)#" );

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_110, "main",
R"#(
#error 2
)#" );

	ppln.AddShader( EShader::Fragment, EShaderLangFormat::GLSL_450, "main", R"#(
#version 450 core
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout (std140) uniform UB
{
	vec4	color;

} ub;

uniform sampler2D un_ColorTexture;

in  vec2	v_Texcoord;

out vec4	out_Color;

void main() {
	out_Color = texture(un_ColorTexture, v_Texcoord) * ub.color;
}
)#" );
	
	ppln.AddShader( EShader::Fragment, EShaderLangFormat::GLSL_440, "main",
R"#(
#error 3
)#" );
	
	GraphicsPipelineDesc	ppln1 = ppln;
	GraphicsPipelineDesc	ppln2 = ppln;
	GraphicsPipelineDesc	ppln3 = ppln;


	compiler->SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations | EShaderCompilationFlags::Quiet );

	TEST(	 compiler->Compile( INOUT ppln1, EShaderLangFormat::SPIRV_100 ));
	TEST( not compiler->Compile( INOUT ppln2, EShaderLangFormat::SPIRV_110 ));
	

	// restore previous state
	compiler->SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations | EShaderCompilationFlags::GenerateDebugInfo );
	FG_LOGI( "Test_Shader6 - passed" );
}

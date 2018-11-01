// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Shader9 (VPipelineCompiler* compiler)
{
	GraphicsPipelineDesc	ppln;

	ppln.AddShader( EShader::Fragment,
				    EShaderLangFormat::GLSL_450,
				    "main",
R"#(
#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_NV_fragment_shader_barycentric : require

//in vec3 gl_BaryCoordNV;
//in vec3 gl_BaryCoordNoPerspNV;

layout(location = 0) out vec4  out_Color;

void main ()
{
	out_Color = vec4( gl_BaryCoordNV.x );	// TODO: the result of .x and .y and .z is the same value (bug?) and differs with .yyyy and .zzzz
}
)#" );


	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::Vulkan_100 | EShaderLangFormat::SPIRV ));

    FG_LOGI( "Test_Shader9 - passed" );
}

// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_MRT1 (VPipelineCompiler* compiler)
{
	GraphicsPipelineDesc	ppln;

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::GLSL_450, "main", R"#(
#version 450 core
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in  vec3	in_Position;
layout(location=2) in  vec2	in_Texcoord;

layout(location=0) out vec2	out_Texcoord;

void main() {
	gl_Position	 = vec4( in_Position, 1.0 );
	out_Texcoord = in_Texcoord;
}
)#" );

	ppln.AddShader( EShader::Fragment, EShaderLangFormat::GLSL_450, "main", R"#(
#version 450 core
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout(binding=0) uniform sampler2D  un_Texture1;
layout(binding=1) uniform usampler2D un_Texture2;

layout(location=0) in  vec2	in_Texcoord;

layout(location=0) out vec4	 out_Color0;
layout(location=2) out uvec4 out_Color1;

void main() {
	out_Color0 = texture(un_Texture1, in_Texcoord);
	out_Color1 = texture(un_Texture2, in_Texcoord);
}
)#" );
	
	const auto	old_flags = compiler->GetCompilationFlags();
	compiler->SetCompilationFlags( EShaderCompilationFlags::Unknown );

	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));
	
	compiler->SetCompilationFlags( old_flags );

	TEST( TestVertexInput( ppln, VertexID("in_Position"), EVertexType::Float3, 0 ));
	TEST( TestVertexInput( ppln, VertexID("in_Texcoord"), EVertexType::Float2, 2 ));

	TEST( TestFragmentOutput( ppln, EFragOutput::Float4, 0 ));
	TEST( TestFragmentOutput( ppln, EFragOutput::UInt4,  2 ));
	
	auto ds = FindDescriptorSet( ppln, DescriptorSetID("0") );
	TEST( ds );
	
	TEST( TestTextureUniform( *ds, UniformID("un_Texture1"), EImage::Tex2D, /*binding*/0, EShaderStages::Fragment, /*arraySize*/1 ));
	TEST( TestTextureUniform( *ds, UniformID("un_Texture2"), EImage::Tex2D, /*binding*/1, EShaderStages::Fragment, /*arraySize*/1 ));
	
	TEST_PASSED();
}

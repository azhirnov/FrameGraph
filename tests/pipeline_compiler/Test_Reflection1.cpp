// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Reflection1 (VPipelineCompiler* compiler)
{
	GraphicsPipelineDesc	ppln;

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::GLSL_450, "main", R"#(
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

	ppln.AddShader( EShader::Fragment, EShaderLangFormat::GLSL_450, "main", R"#(
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


	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));

	TEST( FindVertexInput( ppln, VertexID("at_Position") ));
	TEST( FindVertexInput( ppln, VertexID("at_Texcoord") ));

	TEST( TestFragmentOutput( ppln, EFragOutput::Float4, 0 ));

	auto ds = FindDescriptorSet( ppln, DescriptorSetID("0") );
	TEST( ds );

	TEST( FindUniform< PipelineDescription::Texture >( *ds, UniformID("un_ColorTexture") ).second );
	TEST( FindUniform< PipelineDescription::UniformBuffer >( *ds, UniformID("UB") ).second );

	TEST( ppln._earlyFragmentTests );
	
	TEST( ppln._supportedTopology.test( uint(EPrimitive::Point) ));
	TEST( ppln._supportedTopology.test( uint(EPrimitive::LineList) ));
	TEST( ppln._supportedTopology.test( uint(EPrimitive::LineStrip) ));
	TEST( ppln._supportedTopology.test( uint(EPrimitive::TriangleList) ));
	TEST( ppln._supportedTopology.test( uint(EPrimitive::TriangleStrip) ));
	TEST( ppln._supportedTopology.test( uint(EPrimitive::TriangleFan) ));
	TEST( not ppln._supportedTopology.test( uint(EPrimitive::LineListAdjacency) ));
	TEST( not ppln._supportedTopology.test( uint(EPrimitive::LineStripAdjacency) ));
	TEST( not ppln._supportedTopology.test( uint(EPrimitive::TriangleListAdjacency) ));
	TEST( not ppln._supportedTopology.test( uint(EPrimitive::TriangleStripAdjacency) ));
	TEST( not ppln._supportedTopology.test( uint(EPrimitive::Patch) ));
	
	TEST_PASSED();
}

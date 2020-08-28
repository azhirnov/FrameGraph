// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Reflection2 (VPipelineCompiler* compiler)
{
	GraphicsPipelineDesc	ppln;

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::GLSL_450, "main", R"#(
#version 450 core
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout (location=1) in  vec2	at_Position;
layout (location=0) in  vec2	at_Texcoord;
layout (location=2) in  uint	at_MaterialID;

layout (binding=0) uniform sampler2D un_ColorTexture;

layout(location=0) out vec2	v_Texcoord;
layout(location=1) out uint	v_MaterialID;

void main() {
	gl_Position	 = vec4( at_Position, 0.0, 1.0 );
	v_Texcoord	 = at_Texcoord * texelFetch( un_ColorTexture, ivec2(at_Texcoord), 0 ).xy;
	v_MaterialID = at_MaterialID;
}
)#" );

	ppln.AddShader( EShader::Fragment, EShaderLangFormat::GLSL_450, "main", R"#(
#version 450 core
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout (std140, binding=1) uniform UB
{
	vec4	color;

} ub;

layout (binding=0) uniform sampler2D un_ColorTexture;

layout(location=0) in  vec2	v_Texcoord;

layout(location=0) out vec4	out_Color;

void main() {
	out_Color = texture(un_ColorTexture, v_Texcoord) * ub.color;
}
)#" );


	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));

	TEST( TestVertexInput( ppln, VertexID("at_Position"), EVertexType::Float2, 1 ));
	TEST( TestVertexInput( ppln, VertexID("at_Texcoord"), EVertexType::Float2, 0 ));
	TEST( TestVertexInput( ppln, VertexID("at_MaterialID"), EVertexType::UInt, 2 ));

	TEST( TestFragmentOutput( ppln, EFragOutput::Float4, 0 ));

	auto ds = FindDescriptorSet( ppln, DescriptorSetID("0") );
	TEST( ds );

	TEST( TestTextureUniform( *ds, UniformID("un_ColorTexture"), EImage::Tex2D, 0, EShaderStages::Vertex | EShaderStages::Fragment ));
	TEST( TestUniformBuffer( *ds, UniformID("UB"), 16_b, 1, EShaderStages::Fragment ));

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

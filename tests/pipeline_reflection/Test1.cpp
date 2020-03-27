// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "tests/pipeline_compiler/Utils.h"
#include "pipeline_reflection/VPipelineReflection.h"


extern void Test1 (VPipelineCompiler* compiler)
{
	GraphicsPipelineDesc	ppln;

	ppln.AddShader( EShader::Vertex, EShaderLangFormat::GLSL_450, "main", R"#(
#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in  vec2  at_Position;
layout(location=1) in  vec2  at_Texcoord;

layout(location=0) out vec2  v_Texcoord;

void main() {
	gl_Position	= vec4( at_Position, 0.0, 1.0 );
	v_Texcoord	= at_Texcoord;
}
)#" );

	ppln.AddShader( EShader::Fragment, EShaderLangFormat::GLSL_450, "main", R"#(
#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout (binding=0, std140) uniform UB
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

	TEST( FindFragmentOutput( ppln, RenderTargetID::Color_0 ));

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

	
	GraphicsPipelineDesc	ppln2 = ppln;
	ppln2._fragmentOutput.clear();
	ppln2._vertexAttribs.clear();
	ppln2._pipelineLayout	= Default;

	VPipelineReflection		pr;
	TEST( pr.Reflect( INOUT ppln2 ));

	std::sort( ppln2._fragmentOutput.begin(), ppln2._fragmentOutput.end(), [](auto& lhs, auto& rhs) { return lhs.id < rhs.id; });
	std::sort( ppln._fragmentOutput.begin(),  ppln._fragmentOutput.end(),  [](auto& lhs, auto& rhs) { return lhs.id < rhs.id; });
	TEST( ppln2._fragmentOutput == ppln._fragmentOutput );

	std::sort( ppln2._vertexAttribs.begin(), ppln2._vertexAttribs.end(), [](auto& lhs, auto& rhs) { return lhs.id < rhs.id; });
	std::sort( ppln._vertexAttribs.begin(),  ppln._vertexAttribs.end(),  [](auto& lhs, auto& rhs) { return lhs.id < rhs.id; });
	TEST( ppln2._vertexAttribs == ppln._vertexAttribs );

	std::sort( ppln2._pipelineLayout.descriptorSets.begin(), ppln2._pipelineLayout.descriptorSets.end(), [](auto& lhs, auto& rhs) { return lhs.id < rhs.id; });
	std::sort( ppln._pipelineLayout.descriptorSets.begin(),  ppln._pipelineLayout.descriptorSets.end(),  [](auto& lhs, auto& rhs) { return lhs.id < rhs.id; });

	TEST( ppln2._pipelineLayout.descriptorSets.size() == ppln._pipelineLayout.descriptorSets.size() );
	
	for (size_t i = 0, cnt = ppln._pipelineLayout.descriptorSets.size(); i < cnt; ++i)
	{
		auto&	lhs = ppln._pipelineLayout.descriptorSets[i];
		auto&	rhs = ppln2._pipelineLayout.descriptorSets[i];

		TEST( lhs.bindingIndex == rhs.bindingIndex );
		//TEST( lhs.id == rhs.id );
		TEST( lhs.uniforms );
		TEST( rhs.uniforms );
		TEST( lhs.uniforms->size() == rhs.uniforms->size() );

		for (auto& [lkey, lun] : *lhs.uniforms)
		{
			auto	riter = rhs.uniforms->find( lkey );
			TEST( riter != rhs.uniforms->end() );

			TEST( riter->second == lun );
		}
	}
	
	TEST( ppln2._pipelineLayout.pushConstants.size() == ppln._pipelineLayout.pushConstants.size() );

	for (size_t i = 0, cnt = ppln._pipelineLayout.pushConstants.size(); i < cnt; ++i)
	{
		auto&	lhs = ppln._pipelineLayout.pushConstants[i];
		auto&	rhs = ppln2._pipelineLayout.pushConstants[i];

		TEST( lhs.first == rhs.first );
		TEST( lhs.second.offset == rhs.second.offset );
		TEST( lhs.second.size == rhs.second.size );
		TEST( lhs.second.stageFlags == rhs.second.stageFlags );
	}

	FG_LOGI( "Test1 - passed" );
}

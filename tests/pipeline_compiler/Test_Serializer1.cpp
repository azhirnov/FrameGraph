// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"
#include "pipeline_compiler/PipelineCppSerializer.h"


extern void Test_Serializer1 (VPipelineCompiler* compiler)
{
	GraphicsPipelineDesc	ppln;

	ppln.AddShader( EShader::Vertex,
				    EShaderLangFormat::GLSL_450,
				    "main",
R"#(
#version 450 core
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout (constant_id = 0) const float POS_Z = 0.5f;
layout (constant_id = 1) const float POS_W = 1.0f;

in  vec2	at_Position;
in  vec2	at_Texcoord;

out vec2	v_Texcoord;

void main() {
    gl_Position	= vec4( at_Position, POS_Z, POS_W );
	v_Texcoord	= at_Texcoord;
}
)#" );

	ppln.AddShader( EShader::Fragment,
				    EShaderLangFormat::GLSL_450,
				    "main",
R"#(
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


	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::Vulkan_100 | EShaderLangFormat::SPIRV ));

	for (auto& sh : ppln._shaders) {
		sh.second.data.clear();
	}


	PipelineCppSerializer	serializer;

	String	src;
	TEST( serializer.Serialize( ppln, "default", OUT src ));

	const String	serialized_ref = R"##(GPipelineID  Create_default (const FGThreadPtr &fg)
{
	GraphicsPipelineDesc  desc;

	desc.AddTopology( EPrimitive::Point );
	desc.AddTopology( EPrimitive::LineList );
	desc.AddTopology( EPrimitive::LineStrip );
	desc.AddTopology( EPrimitive::TriangleList );
	desc.AddTopology( EPrimitive::TriangleStrip );
	desc.AddTopology( EPrimitive::TriangleFan );
	desc.SetFragmentOutputs({
			{ RenderTargetID{"out_Color"}, 0, EFragOutput::Float4 } });

	desc.SetVertexAttribs({
			{ VertexID{"at_Position"}, 0, EVertexType::Float2 },
			{ VertexID{"at_Texcoord"}, 1, EVertexType::Float2 } });

	desc.SetEarlyFragmentTests( true );

	desc.AddDescriptorSet(
			DescriptorSetID{"0"},
			0,
			{{ UniformID{"un_ColorTexture"}, EImage::Tex2D, BindingIndex{~0u, 1u}, EShaderStages::Fragment }},
			{},
			{},
			{},
			{{ UniformID{"UB"}, 16_b, BindingIndex{~0u, 0u}, EShaderStages::Fragment }},
			{} );

	desc.SetSpecConstants( EShader::Vertex, {
			{SpecializationID{"POS_Z"}, 0u},
			{SpecializationID{"POS_W"}, 1u} });

	return fg->CreatePipeline( std::move(desc) );
}
)##";

	TEST( serialized_ref == src );

    FG_LOGI( "Test_Serializer1 - passed" );
}

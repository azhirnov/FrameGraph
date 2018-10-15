// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::_ImplTest_Scene1 ()
	{
		static const char ref_frame_dump[] = 
			""
			//#include "FGIApp_Scene1_ref.txt"
		;

		struct Vertex1
		{
			float3		position;
			short2		texcoord;
		};

		const uint2		view_size			{ 1024, 1024 };
		const BytesU	cbuf_size			= 256_b;
		const BytesU	cbuf_offset			= Max( cbuf_size, BytesU(_vulkan.GetDeviceProperties().limits.minUniformBufferOffsetAlignment) );
		const BytesU	cbuf_aligned_size	= AlignToLarger( cbuf_size, cbuf_offset );

		BufferPtr	const_buf1 = _CreateBuffer( cbuf_aligned_size,   "const_buf1" );
		BufferPtr	const_buf2 = _CreateBuffer( cbuf_aligned_size*2, "const_buf2" );
		BufferPtr	const_buf3 = _CreateBuffer( cbuf_aligned_size,   "const_buf3" );
	
		BufferPtr	vbuffer1 = _CreateBuffer( SizeOf<Vertex1> * 3*1000, "vbuffer1" );
		BufferPtr	vbuffer2 = _CreateBuffer( SizeOf<Vertex1> * 3*2000, "vbuffer2" );

		ImagePtr	texture1 = _CreateImage2D( uint2(512, 512), EPixelFormat::RGBA8_UNorm, "texture1" );
		ImagePtr	texture2 = _CreateImage2D( uint2(256, 512), EPixelFormat::RGBA8_UNorm, "texture2" );

		SamplerPtr	sampler1 = _frameGraph->CreateSampler( SamplerDesc() );

		const VertexInputState	vertex_input = VertexInputState{}.Bind( VertexBufferID(), SizeOf<Vertex1> )
														.Add( VertexID("at_Position"),	&Vertex1::position )
														.Add( VertexID("at_Texcoord"),	&Vertex1::texcoord, true );
		
		GraphicsPipelineDesc	ppln1;

		ppln1.AddShader( EShader::Vertex,
						EShaderLangFormat::GLSL_450,
						"main",
R"#(
#version 450 core
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout (binding=0, std140) uniform un_ConstBuf
{
	mat4	mvp;
	mat4	projection;
	mat4	view;
	vec4	color;
	vec4	color1;
	vec4	color2;
	vec4	color3;

} ub;

in  vec3	at_Position;
in  vec2	at_Texcoord;

out vec2	v_Texcoord;

void main() {
    gl_Position	= vec4( at_Position, 1.0 ) * ub.mvp;
	v_Texcoord	= at_Texcoord;
}
)#" );

		ppln1.AddShader( EShader::Fragment,
						EShaderLangFormat::GLSL_450,
						"main",
R"#(
#version 450 core
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout (binding=0, std140) uniform un_ConstBuf
{
	mat4	mvp;
	mat4	projection;
	mat4	view;
	vec4	color;
	vec4	color1;
	vec4	color2;
	vec4	color3;

} ub;

layout (binding=1) uniform sampler2D un_ColorTexture;

in  vec2	v_Texcoord;

out vec4	out_Color;

void main() {
    out_Color = texture(un_ColorTexture, v_Texcoord) * ub.color;
}
)#" );

		
		GraphicsPipelineDesc	ppln2;

		ppln2.AddShader( EShader::Vertex,
						EShaderLangFormat::GLSL_450,
						"main",
R"#(
#version 450 core
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout (binding=0, std140) uniform un_ConstBuf
{
	mat4	mvp;
	mat4	projection;
	mat4	view;
	vec4	color;
	vec4	color1;
	vec4	color2;
	vec4	color3;

} ub;

in  vec3	at_Position;
in  vec2	at_Texcoord;

out vec2	v_Texcoord;

void main() {
    gl_Position	= vec4( at_Position, 1.0 ) * ub.mvp;
	v_Texcoord	= at_Texcoord;
}
)#" );

		ppln2.AddShader( EShader::Fragment,
						EShaderLangFormat::GLSL_450,
						"main",
R"#(
#version 450 core
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout (binding=0, std140) uniform un_ConstBuf
{
	mat4	mvp;
	mat4	projection;
	mat4	view;
	vec4	color;
	vec4	color1;
	vec4	color2;
	vec4	color3;

} ub;

layout (binding=1) uniform sampler2D un_ColorTexture;

in  vec2	v_Texcoord;

void main() {
    if ( texture(un_ColorTexture, v_Texcoord).a * ub.color.a < 0.1f )
		discard;
}
)#" );

		PipelinePtr				pipeline1	= _frameGraph->CreatePipeline( std::move(ppln1) );
		PipelinePtr				pipeline2	= _frameGraph->CreatePipeline( std::move(ppln2) );
		CHECK_ERR( pipeline1->GetDescriptorSet(DescriptorSetID("0")) == pipeline2->GetDescriptorSet(DescriptorSetID("0")) );

		PipelineResourcesPtr	resources1	= pipeline1->CreateResources( DescriptorSetID("0") );
		resources1->BindTexture( UniformID("un_ColorTexture"), texture1, sampler1 )
				   .BindBuffer( UniformID("un_ConstBuf"), const_buf2, 0_b, cbuf_size );
		
		PipelineResourcesPtr	resources2	= pipeline1->CreateResources( DescriptorSetID("0") );
		resources2->BindTexture( UniformID("un_ColorTexture"), texture1, sampler1 )
				   .BindBuffer( UniformID("un_ConstBuf"), const_buf2, cbuf_offset, cbuf_size );

		PipelineResourcesPtr	resources3	= pipeline1->CreateResources( DescriptorSetID("0") );
		resources3->BindTexture( UniformID("un_ColorTexture"), texture1, sampler1 )
				   .BindBuffer( UniformID("un_ConstBuf"), const_buf3 );

		PipelineResourcesPtr	resources4	= pipeline1->CreateResources( DescriptorSetID("0") );
		resources4->BindTexture( UniformID("un_ColorTexture"), texture1, sampler1 )
				   .BindBuffer( UniformID("un_ConstBuf"), const_buf1 );

		PipelineResourcesPtr	resources5	= pipeline1->CreateResources( DescriptorSetID("0") );
		resources5->BindTexture( UniformID("un_ColorTexture"), texture1, sampler1 )
				   .BindBuffer( UniformID("un_ConstBuf"), const_buf2, cbuf_offset, cbuf_size );


		CHECK( _frameGraph->Begin() );
		
		ImagePtr	color_target = _frameGraph->CreateImage( EMemoryType::Default, EImage::Tex2D,
															 uint3(view_size.x, view_size.y, 0), EPixelFormat::RGBA8_UNorm,
															 EImageUsage::ColorAttachment );
		ImagePtr	depth_target = _frameGraph->CreateImage( EMemoryType::Default, EImage::Tex2D,
															 uint3(view_size.x, view_size.y, 0), EPixelFormat::Depth32F,
															 EImageUsage::DepthStencilAttachment );
		color_target->SetDebugName( "color_target" );
		depth_target->SetDebugName( "depth_target" );

		//ImagePtr	color_target = _CreateLogicalImage2D( view_size, EPixelFormat::RGBA8_UNorm, "color_target" );
		//ImagePtr	depth_target = _CreateLogicalImage2D( view_size, EPixelFormat::Depth32F, "depth_target" );
	
		RenderPass	depth_pass = _frameGraph->CreateRenderPass( RenderPassDesc{ view_size }
											.AddTarget( RenderTargetID(),				depth_target, DepthStencil(), EAttachmentStoreOp::Store ));

		RenderPass	opaque_pass = _frameGraph->CreateRenderPass( RenderPassDesc{ view_size }
											.AddTarget(  RenderTargetID("out_Color"),	color_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
											.AddTarget(  RenderTargetID(),				depth_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store ));
	
		RenderPass	transparent_pass = _frameGraph->CreateRenderPass( RenderPassDesc{ view_size }
											.AddTarget(  RenderTargetID("out_Color"),	color_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
											.AddTarget(  RenderTargetID(),				depth_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Invalidate ));


		{
			// depth pass
			Task	t_submit_depth = null;
			{
				_frameGraph->AddDrawTask( depth_pass,
					DrawTask{}.SetName( "Draw_Depth" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer1, 0_b ).SetVertices( 0, 3*1000 )
						.SetPipeline( pipeline2 ).AddResources( DescriptorSetID("0"), resources1 )
						.SetRenderState( RenderState().SetDepthTestEnabled(true).SetDepthWriteEnabled(true).SetTopology(EPrimitive::TriangleList) )
					 );
		
				Task	t_update_buf0 = _frameGraph->AddTask( UpdateBuffer{ const_buf2, 0_b, _CreateData( 256_b ) }.SetName( "update_buf0" ) );

				t_submit_depth = _frameGraph->AddTask( SubmitRenderPass{ depth_pass }.SetName( "DepthOnlyPass" ).DependsOn( t_update_buf0 ) );
			}

			// opaque pass
			Task	t_submit_opaque;
			{
				_frameGraph->AddDrawTask( opaque_pass,
					DrawTask{}.SetName( "Draw1_Opaque" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer1, 0_b ).SetVertices( 0, 3*1000 )
						.SetPipeline( pipeline1 ).AddResources( DescriptorSetID("0"), resources2 )
						.SetRenderState( RenderState().SetDepthTestEnabled(true).SetDepthWriteEnabled(false).SetTopology(EPrimitive::TriangleList) )
					 );

				_frameGraph->AddDrawTask( opaque_pass,
					DrawTask{}.SetName( "Draw2_Opaque" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer2, 0_b ).SetVertices( 0, 3*1000 )
						.SetPipeline( pipeline1 ).AddResources( DescriptorSetID("0"), resources3 )
						.SetRenderState( RenderState().SetDepthTestEnabled(true).SetDepthWriteEnabled(false).SetTopology(EPrimitive::TriangleList) )
					 );

				_frameGraph->AddDrawTask( opaque_pass,
					DrawTask{}.SetName( "Draw0_Opaque" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer1, 0_b ).SetVertices( 0, 3*2000 )
						.SetPipeline( pipeline1 ).AddResources( DescriptorSetID("0"), resources4 )
						.SetRenderState( RenderState().SetDepthTestEnabled(true).SetDepthWriteEnabled(false).SetTopology(EPrimitive::TriangleList) )
					 );
				
				Task	t_update_buf1 = _frameGraph->AddTask( UpdateBuffer{ const_buf1,   0_b, _CreateData( 256_b ) }.SetName( "update_buf1" ) );
				Task	t_update_buf2 = _frameGraph->AddTask( UpdateBuffer{ const_buf2, 256_b, _CreateData( 256_b ) }.SetName( "update_buf2" ) );
				
				t_submit_opaque = _frameGraph->AddTask( SubmitRenderPass{ opaque_pass }.SetName( "OpaquePass" ).DependsOn( t_submit_depth, t_update_buf1, t_update_buf2 ) );
			}

			// transparent pass
			Task	t_submit_transparent;
			{
				_frameGraph->AddDrawTask( transparent_pass,
					DrawTask{}.SetName( "Draw1_Transparent" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer2, 0_b ).SetVertices( 0, 3*1000 )
						.SetPipeline( pipeline1 ).AddResources( DescriptorSetID("0"), resources4 )
						.SetRenderState( RenderState().SetDepthTestEnabled(true).SetDepthWriteEnabled(false).SetTopology(EPrimitive::TriangleList)
											.AddColorBuffer( RenderTargetID(), EBlendFactor::SrcAlpha, EBlendFactor::OneMinusSrcAlpha, EBlendOp::Add ) )
					 );
				
				_frameGraph->AddDrawTask( transparent_pass,
					DrawTask{}.SetName( "Draw2_Transparent" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer1, 0_b ).SetVertices( 0, 3*1000 )
						.SetPipeline( pipeline1 ).AddResources( DescriptorSetID("0"), resources1 )
						.SetRenderState( RenderState().SetDepthTestEnabled(true).SetDepthWriteEnabled(false).SetTopology(EPrimitive::TriangleList)
											.AddColorBuffer( RenderTargetID(), EBlendFactor::SrcAlpha, EBlendFactor::OneMinusSrcAlpha, EBlendOp::Add ) )
					 );
		
				_frameGraph->AddDrawTask( transparent_pass,
					DrawTask{}.SetName( "Draw0_Transparent" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer2, 0_b ).SetVertices( 0, 3*2000 )
						.SetPipeline( pipeline1 ).AddResources( DescriptorSetID("0"), resources5 )
						.SetRenderState( RenderState().SetDepthTestEnabled(true).SetDepthWriteEnabled(false).SetTopology(EPrimitive::TriangleList)
											.AddColorBuffer( RenderTargetID(), EBlendFactor::SrcAlpha, EBlendFactor::OneMinusSrcAlpha, EBlendOp::Add ) )
					 );
				
				Task	update_buf3 = _frameGraph->AddTask( UpdateBuffer{ const_buf2, 0_b, _CreateData( 256_b ) }.SetName( "update_buf3" ) );

				t_submit_transparent = _frameGraph->AddTask( SubmitRenderPass{ transparent_pass }.SetName( "TransparentPass" ).DependsOn( t_submit_opaque, update_buf3 ) );
			}

			// present
			(void)(_frameGraph->AddTask( Present{ color_target }.DependsOn( t_submit_transparent ) ));
		}
		
		CHECK( _frameGraph->Compile() );		
		CHECK( _frameGraph->Execute() );
	
		FG_LOGI( "ImplTest_Scene1 - passed" );
		return true;
	}

}	// FG

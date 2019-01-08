// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::ImplTest_Scene1 ()
	{
		GraphicsPipelineDesc	ppln1;

		ppln1.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
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

		ppln1.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", R"#(
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

		ppln2.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
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

		ppln2.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", R"#(
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

		struct Vertex1
		{
			float3		position;
			short2		texcoord;
		};

		const uint2		view_size			{ 1024, 1024 };
		const BytesU	cbuf_size			= 256_b;
		const BytesU	cbuf_offset			= Max( cbuf_size, BytesU(_vulkan.GetDeviceProperties().limits.minUniformBufferOffsetAlignment) );
		const BytesU	cbuf_aligned_size	= AlignToLarger( cbuf_size, cbuf_offset );
		
		FGThreadPtr	frame_graph	= _fgGraphics1;

		BufferID	const_buf1 = frame_graph->CreateBuffer( BufferDesc{ cbuf_aligned_size,   EBufferUsage::Uniform | EBufferUsage::TransferDst }, Default, "const_buf1" );
		BufferID	const_buf2 = frame_graph->CreateBuffer( BufferDesc{ cbuf_aligned_size*2, EBufferUsage::Uniform | EBufferUsage::TransferDst }, Default, "const_buf2" );
		BufferID	const_buf3 = frame_graph->CreateBuffer( BufferDesc{ cbuf_aligned_size,   EBufferUsage::Uniform | EBufferUsage::TransferDst }, Default, "const_buf3" );
	
		BufferID	vbuffer1 = frame_graph->CreateBuffer( BufferDesc{ SizeOf<Vertex1> * 3*1000, EBufferUsage::Vertex | EBufferUsage::TransferDst }, Default, "vbuffer1" );
		BufferID	vbuffer2 = frame_graph->CreateBuffer( BufferDesc{ SizeOf<Vertex1> * 3*2000, EBufferUsage::Vertex | EBufferUsage::TransferDst }, Default, "vbuffer2" );

		ImageID		texture1 = frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3(512, 512, 1), EPixelFormat::RGBA8_UNorm,
																	EImageUsage::Sampled | EImageUsage::TransferDst }, Default, "texture1" );
		ImageID		texture2 = frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3(256, 512, 1), EPixelFormat::RGBA8_UNorm,
																	EImageUsage::Sampled | EImageUsage::TransferDst }, Default, "texture2" );

		SamplerID	sampler1 = frame_graph->CreateSampler( SamplerDesc() );

		const VertexInputState	vertex_input = VertexInputState{}.Bind( VertexBufferID(), SizeOf<Vertex1> )
														.Add( VertexID("at_Position"),	&Vertex1::position )
														.Add( VertexID("at_Texcoord"),	&Vertex1::texcoord, true );
		
		GPipelineID		pipeline1	= frame_graph->CreatePipeline( std::move(ppln1) );
		GPipelineID		pipeline2	= frame_graph->CreatePipeline( std::move(ppln2) );

		PipelineResources	resources;
		CHECK_ERR( frame_graph->InitPipelineResources( pipeline1, DescriptorSetID("0"), OUT resources ));


		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
		CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
		CHECK_ERR( frame_graph->Begin( batch_id, 0, EThreadUsage::Graphics ));
		
		ImageID		color_target = frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3(view_size.x, view_size.y, 0), EPixelFormat::RGBA8_UNorm,
																		EImageUsage::ColorAttachment | EImageUsage::TransferSrc }, Default, "color_target" );

		ImageID		depth_target = frame_graph->CreateImage( ImageDesc{ EImage::Tex2D, uint3(view_size.x, view_size.y, 0), EPixelFormat::Depth32F,
																		EImageUsage::DepthStencilAttachment }, Default, "depth_target" );

		//ImageID	color_target = _CreateLogicalImage2D( view_size, EPixelFormat::RGBA8_UNorm, "color_target" );
		//ImageID	depth_target = _CreateLogicalImage2D( view_size, EPixelFormat::Depth32F, "depth_target" );
	
		LogicalPassID	depth_pass = frame_graph->CreateRenderPass( RenderPassDesc{ view_size }
											.AddTarget( RenderTargetID(),				depth_target, DepthStencil(), EAttachmentStoreOp::Store )
											.SetDepthTestEnabled(true).SetDepthWriteEnabled(true) );

		LogicalPassID	opaque_pass = frame_graph->CreateRenderPass( RenderPassDesc{ view_size }
											.AddTarget(  RenderTargetID("out_Color"),	color_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
											.AddTarget(  RenderTargetID(),				depth_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
											.SetDepthTestEnabled(true).SetDepthWriteEnabled(false) );
	
		LogicalPassID	transparent_pass = frame_graph->CreateRenderPass( RenderPassDesc{ view_size }
											.AddTarget(  RenderTargetID("out_Color"),	color_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Store )
											.AddTarget(  RenderTargetID(),				depth_target, EAttachmentLoadOp::Load, EAttachmentStoreOp::Invalidate )
											.AddColorBuffer( RenderTargetID("out_Color"), EBlendFactor::SrcAlpha, EBlendFactor::OneMinusSrcAlpha, EBlendOp::Add )
											.SetDepthTestEnabled(true).SetDepthWriteEnabled(false) );

		{
			// depth pass
			Task	t_submit_depth = null;
			{
				resources.BindTexture( UniformID("un_ColorTexture"), texture1, sampler1 )
						 .BindBuffer( UniformID("un_ConstBuf"), const_buf2, 0_b, cbuf_size );

				frame_graph->AddTask( depth_pass,
					DrawVertices{}.SetName( "Draw_Depth" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer1, 0_b ).AddDrawCmd( 3*1000 )
						.SetPipeline( pipeline2 ).AddResources( DescriptorSetID("0"), &resources ).SetTopology(EPrimitive::TriangleList) );
		
				Task	t_update_buf0 = frame_graph->AddTask( UpdateBuffer{ const_buf2, 0_b, CreateData( 256_b ) }.SetName( "update_buf0" ));

				t_submit_depth = frame_graph->AddTask( SubmitRenderPass{ depth_pass }.SetName( "DepthOnlyPass" ).DependsOn( t_update_buf0 ));
			}

			// opaque pass
			Task	t_submit_opaque;
			{
				resources.BindTexture( UniformID("un_ColorTexture"), texture2, sampler1 )
						 .BindBuffer( UniformID("un_ConstBuf"), const_buf2, cbuf_offset, cbuf_size );

				frame_graph->AddTask( opaque_pass,
					DrawVertices{}.SetName( "Draw1_Opaque" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer1, 0_b ).AddDrawCmd( 3*1000 )
						.SetPipeline( pipeline1 ).AddResources( DescriptorSetID("0"), &resources ).SetTopology(EPrimitive::TriangleList) );
				
				resources.BindTexture( UniformID("un_ColorTexture"), texture1, sampler1 )
						 .BindBuffer( UniformID("un_ConstBuf"), const_buf2, 0_b, cbuf_size );

				frame_graph->AddTask( opaque_pass,
					DrawVertices{}.SetName( "Draw2_Opaque" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer2, 0_b ).AddDrawCmd( 3*1000 )
						.SetPipeline( pipeline1 ).AddResources( DescriptorSetID("0"), &resources ).SetTopology(EPrimitive::TriangleList) );
				
				resources.BindTexture( UniformID("un_ColorTexture"), texture1, sampler1 )
						 .BindBuffer( UniformID("un_ConstBuf"), const_buf1 );

				frame_graph->AddTask( opaque_pass,
					DrawVertices{}.SetName( "Draw0_Opaque" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer1, 0_b ).AddDrawCmd( 3*2000 )
						.SetPipeline( pipeline1 ).AddResources( DescriptorSetID("0"), &resources ).SetTopology(EPrimitive::TriangleList) );
				
				Task	t_update_buf1 = frame_graph->AddTask( UpdateBuffer{ const_buf1,   0_b, CreateData( 256_b ) }.SetName( "update_buf1" ));
				Task	t_update_buf2 = frame_graph->AddTask( UpdateBuffer{ const_buf2, 256_b, CreateData( 256_b ) }.SetName( "update_buf2" ));
				
				t_submit_opaque = frame_graph->AddTask( SubmitRenderPass{ opaque_pass }.SetName( "OpaquePass" ).DependsOn( t_submit_depth, t_update_buf1, t_update_buf2 ));
			}

			// transparent pass
			Task	t_submit_transparent;
			{
				resources.BindTexture( UniformID("un_ColorTexture"), texture2, sampler1 )
						 .BindBuffer( UniformID("un_ConstBuf"), const_buf3 );

				frame_graph->AddTask( transparent_pass,
					DrawVertices{}.SetName( "Draw1_Transparent" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer2, 0_b ).AddDrawCmd( 3*1000 )
						.SetPipeline( pipeline1 ).AddResources( DescriptorSetID("0"), &resources ).SetTopology(EPrimitive::TriangleList) );
				
				resources.BindTexture( UniformID("un_ColorTexture"), texture1, sampler1 )
						 .BindBuffer( UniformID("un_ConstBuf"), const_buf2, 0_b, cbuf_size );

				frame_graph->AddTask( transparent_pass,
					DrawVertices{}.SetName( "Draw2_Transparent" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer1, 0_b ).AddDrawCmd( 3*1000 )
						.SetPipeline( pipeline1 ).AddResources( DescriptorSetID("0"), &resources ).SetTopology(EPrimitive::TriangleList) );
		
				resources.BindTexture( UniformID("un_ColorTexture"), texture1, sampler1 )
						 .BindBuffer( UniformID("un_ConstBuf"), const_buf2, cbuf_offset, cbuf_size );

				frame_graph->AddTask( transparent_pass,
					DrawVertices{}.SetName( "Draw0_Transparent" )
						.SetVertexInput( vertex_input ).AddBuffer( VertexBufferID(), vbuffer2, 0_b ).AddDrawCmd( 3*2000 )
						.SetPipeline( pipeline1 ).AddResources( DescriptorSetID("0"), &resources ).SetTopology(EPrimitive::TriangleList) );
				
				Task	update_buf3 = frame_graph->AddTask( UpdateBuffer{ const_buf3, 0_b, CreateData( 256_b ) }.SetName( "update_buf3" ) );

				t_submit_transparent = frame_graph->AddTask( SubmitRenderPass{ transparent_pass }.SetName( "TransparentPass" ).DependsOn( t_submit_opaque, update_buf3 ));
			}

			// present
			Task	t_present = frame_graph->AddTask( Present{ color_target }.DependsOn( t_submit_transparent ));
			FG_UNUSED( t_present );
		}
		
		CHECK_ERR( frame_graph->Execute() );		
		CHECK_ERR( _fgInstance->EndFrame() );
	
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));
		
		CHECK_ERR( _fgInstance->WaitIdle() );
		
		DeleteResources( const_buf1, const_buf2, const_buf3, vbuffer1, vbuffer2, texture1, texture2,
						 color_target, depth_target, pipeline1, pipeline2, sampler1 );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

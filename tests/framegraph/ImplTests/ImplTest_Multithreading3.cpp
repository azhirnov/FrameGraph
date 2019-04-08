// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"
#include "stl/ThreadSafe/Barrier.h"
#include <thread>

namespace FG
{
	static constexpr uint	max_count		= 1000;
	static GPipelineID		pipeline;
	static Barrier			sync			{2};
	static CommandBuffer	cmdBuffers[2]	= {};
	static CommandBuffer	perFrame[2]		= {};


	static bool RenderThread1 (const FrameGraph &fg)
	{
		const uint2	view_size	= {800, 600};
		ImageID		image;

		for (uint i = 0; i < max_count; ++i)
		{
			fg->Wait({ perFrame[i&1] });

			CommandBuffer cmd = fg->Begin( CommandBufferDesc{ EQueueType::Graphics });
			CHECK_ERR( cmd );
			
			perFrame[i&1] = cmd;
			cmdBuffers[0] = cmd;

			// (1) wake up all render threads
			sync.wait();

			// create image in async mode to avoid manual synchronizations
			if ( i == 0 ) {
				image = fg->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA8_UNorm,
													EImageUsage::ColorAttachment | EImageUsage::TransferSrc }, Default, "RenderTarget1" );
			}
			
			LogicalPassID	render_pass	= cmd->CreateRenderPass( RenderPassDesc( view_size )
												.AddTarget( RenderTargetID(0), image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
												.AddViewport( view_size ) );
		
			cmd->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( pipeline ).SetTopology( EPrimitive::TriangleList ));

			Task	t_draw	= cmd->AddTask( SubmitRenderPass{ render_pass });
			FG_UNUSED( t_draw );

			// release image in async mode to avoid manual synchronizations
			if ( i+1 == max_count )
				fg->ReleaseResource( image );
			
			CHECK_ERR( fg->Execute( cmd ));
			
			// (2) wait until all threads complete command buffer recording
			sync.wait();

			CHECK_ERR( fg->Flush() );
		}

		return true;
	}


	static bool RenderThread2 (const FrameGraph &fg)
	{
		const uint2	view_size	= {1280, 960};
		ImageID		image;

		for (uint i = 0; i < max_count; ++i)
		{
			CommandBuffer cmd = fg->Begin( CommandBufferDesc{ EQueueType::Graphics });
			CHECK_ERR( cmd );
			
			cmdBuffers[1] = cmd;

			// (1) wait for first command buffer
			sync.wait();
			cmd->AddDependency( cmdBuffers[0] );
			
			// create image in async mode to avoid manual synchronizations
			if ( i == 0 ) {
				image = fg->CreateImage( ImageDesc{ EImage::Tex2D, uint3{view_size.x, view_size.y, 1}, EPixelFormat::RGBA16_UNorm,
													EImageUsage::ColorAttachment | EImageUsage::TransferSrc }, Default, "RenderTarget2" );
			}

			LogicalPassID	render_pass	= cmd->CreateRenderPass( RenderPassDesc( view_size )
												.AddTarget( RenderTargetID(0), image, RGBA32f(0.0f), EAttachmentStoreOp::Store )
												.AddViewport( view_size ) );
		
			cmd->AddTask( render_pass, DrawVertices().Draw( 3 ).SetPipeline( pipeline ).SetTopology( EPrimitive::TriangleList ));

			Task	t_draw	= cmd->AddTask( SubmitRenderPass{ render_pass });
			FG_UNUSED( t_draw );
			
			// release image in async mode to avoid manual synchronizations
			if ( i+1 == max_count )
				fg->ReleaseResource( image );
			
			CHECK_ERR( fg->Execute( cmd ));
			
			// (2) notify that thread has already finished recording the command buffer
			sync.wait();
		}

		return true;
	}


	bool FGApp::ImplTest_Multithreading3 ()
	{
		GraphicsPipelineDesc	ppln;
		ppln.AddShader( EShader::Vertex, EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

out vec3	v_Color;

const vec2	g_Positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

const vec3	g_Colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

void main() {
	gl_Position	= vec4( g_Positions[gl_VertexIndex], 0.0, 1.0 );
	v_Color		= g_Colors[gl_VertexIndex];
}
)#" );
		ppln.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_100, "main", R"#(
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

in  vec3	v_Color;
out vec4	out_Color;

void main() {
	out_Color = vec4(v_Color, 1.0);
}
)#" );
		
		pipeline = _frameGraph->CreatePipeline( ppln );
		
		bool			thread1_result;
		bool			thread2_result;

		std::thread		thread1( [this, &thread1_result]() { thread1_result = RenderThread1( _frameGraph ); });
		std::thread		thread2( [this, &thread2_result]() { thread2_result = RenderThread2( _frameGraph ); });

		thread1.join();
		thread2.join();

		CHECK_ERR( _frameGraph->WaitIdle() );
		CHECK_ERR( thread1_result and thread2_result );
		
		for (auto& cmd : cmdBuffers) { cmd = null; }
		for (auto& cmd : perFrame) { cmd = null; }

		DeleteResources( pipeline );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

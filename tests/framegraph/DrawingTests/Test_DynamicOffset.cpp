// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This test affects:
		...
*/

#include "../FGApp.h"
#include "pipeline_compiler/VPipelineCompiler.h"

namespace FG
{

	bool FGApp::Test_DynamicOffset ()
	{
		ComputePipelineDesc	ppln;

		ppln.AddShader( EShaderLangFormat::VKSL_100, "main", R"#(
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (std140) uniform UB {
	vec4	data[4];
} ub;

layout (std430) writeonly buffer SSB {
	vec4	data[4];
} ssb;

void main ()
{
	ssb.data[0] = ub.data[1];
	ssb.data[3] = ub.data[2];
	ssb.data[1] = ub.data[3];
	ssb.data[2] = ub.data[0];
}
)#" );

		const auto	old_flags = _pplnCompiler->GetCompilationFlags();

		_pplnCompiler->SetCompilationFlags( old_flags | EShaderCompilationFlags::AlwaysBufferDynamicOffset );
		
		FGThreadPtr		frame_graph		= _fgGraphics1;
		const BytesU	base_off		= 128_b;
		const BytesU	buf_off			= 128_b;
		const BytesU	src_size		= SizeOf<float4> * 4;
		const BytesU	dst_size		= SizeOf<float4> * 4;
		BufferID		buffer			= frame_graph->CreateBuffer( BufferDesc{ base_off + buf_off + src_size + dst_size,
																				 EBufferUsage::Uniform | EBufferUsage::Storage | EBufferUsage::Transfer },
																	 Default, "SharedBuffer" );
		const float		src_data[]		= { 1.0f, 2.0f, 3.0f, 4.0f,
											5.0f, 6.0f, 7.0f, 8.0f,
											1.1f, 2.2f, 2.3f, 2.4f,
											1.5f, 1.6f, 1.7f, 1.8f };
		CPipelineID		pipeline		= frame_graph->CreatePipeline( ppln );
		CHECK_ERR( pipeline );
		
		_pplnCompiler->SetCompilationFlags( old_flags );

		PipelineResources	resources;
		CHECK_ERR( frame_graph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources ));
		

		bool	cb_was_called	= false;
		bool	data_is_correct	= false;

		const auto	OnLoaded = [&src_data, dst_size, OUT &cb_was_called, OUT &data_is_correct] (BufferView data)
		{
			cb_was_called	= true;
			data_is_correct	= (data.size() == size_t(dst_size));

			const float*	dst_data = Cast<float>( data.Parts().front().data() );

			data_is_correct &= (src_data[1*4+0] == dst_data[0*4+0]);
			data_is_correct &= (src_data[1*4+1] == dst_data[0*4+1]);
			data_is_correct &= (src_data[1*4+2] == dst_data[0*4+2]);
			data_is_correct &= (src_data[1*4+3] == dst_data[0*4+3]);
			
			data_is_correct &= (src_data[2*4+0] == dst_data[3*4+0]);
			data_is_correct &= (src_data[2*4+1] == dst_data[3*4+1]);
			data_is_correct &= (src_data[2*4+2] == dst_data[3*4+2]);
			data_is_correct &= (src_data[2*4+3] == dst_data[3*4+3]);
			
			data_is_correct &= (src_data[3*4+0] == dst_data[1*4+0]);
			data_is_correct &= (src_data[3*4+1] == dst_data[1*4+1]);
			data_is_correct &= (src_data[3*4+2] == dst_data[1*4+2]);
			data_is_correct &= (src_data[3*4+3] == dst_data[1*4+3]);
			
			data_is_correct &= (src_data[0*4+0] == dst_data[2*4+0]);
			data_is_correct &= (src_data[0*4+1] == dst_data[2*4+1]);
			data_is_correct &= (src_data[0*4+2] == dst_data[2*4+2]);
			data_is_correct &= (src_data[0*4+3] == dst_data[2*4+3]);
		};

		
		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
		CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
		CHECK_ERR( frame_graph->Begin( batch_id, 0, EThreadUsage::Graphics ));
		
		resources.SetBufferBase( UniformID("UB"),  0_b );
		resources.SetBufferBase( UniformID("SSB"), base_off );

		resources.BindBuffer( UniformID("UB"),  buffer, base_off + buf_off,  src_size );
		resources.BindBuffer( UniformID("SSB"), buffer, base_off + buf_off + src_size,  dst_size );

		Task	t_write		= frame_graph->AddTask( UpdateBuffer{}.SetBuffer( buffer ).AddData( src_data, CountOf(src_data), base_off + buf_off ));
		Task	t_dispatch	= frame_graph->AddTask( DispatchCompute{}.SetPipeline( pipeline ).AddResources( DescriptorSetID("0"), &resources ).Dispatch({ 1, 1 }).DependsOn( t_write ));
		Task	t_read		= frame_graph->AddTask( ReadBuffer{}.SetBuffer( buffer, base_off + buf_off + src_size, dst_size ).SetCallback( OnLoaded ).DependsOn( t_dispatch ));
		FG_UNUSED( t_read );

		CHECK_ERR( frame_graph->Execute() );		
		CHECK_ERR( _fgInstance->EndFrame() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));
		
		CHECK_ERR( not cb_was_called );
		
		CHECK_ERR( _fgInstance->WaitIdle() );
		CHECK_ERR( cb_was_called );
		CHECK_ERR( data_is_correct );
		
		DeleteResources( pipeline, buffer );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

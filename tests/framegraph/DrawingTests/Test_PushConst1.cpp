// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This test affects:
		...
*/

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_PushConst1 ()
	{
		ComputePipelineDesc	ppln;

		ppln.AddShader( EShaderLangFormat::VKSL_100, "main", R"#(
#extension GL_ARB_shading_language_420pack : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (push_constant, std140) uniform PushConst {
						vec3	f3;
						ivec2	i2;
	layout(offset = 32) float	f1;
} pc;

layout (std430) writeonly buffer SSB {
	vec4	data[2];
} ssb;

void main ()
{
	ssb.data[0] = vec4(pc.f3.x, pc.f3.y, pc.f3.z, pc.f1);
	ssb.data[1] = vec4(float(pc.i2.x), float(pc.i2.y), 0.0f, 1.0f);
}
)#" );

		struct PushConst {
			float3		f3;			// offset:  0
			float		_padding1;	// offset: 12
			int2		i2;			// offset: 16
			float2		_padding2;	// offset: 24
			float		f1;			// offset: 32
		};							// size:   36
		
		FGThreadPtr		frame_graph		= _fgThreads[0];
		const BytesU	dst_buffer_size	= 32_b;
		BufferID		dst_buffer		= frame_graph->CreateBuffer( BufferDesc{ dst_buffer_size, EBufferUsage::Storage | EBufferUsage::TransferSrc }, Default, "DstBuffer" );

		CPipelineID		pipeline		= frame_graph->CreatePipeline( ppln );
		CHECK_ERR( pipeline );

		PushConst	pc;
		pc.f3 = { 10.1f, 11.2f, 18.5f };
		pc.i2 = { 11, 22 };
		pc.f1 = 33.0f;
		
		PipelineResources	resources;
		CHECK_ERR( frame_graph->InitPipelineResources( pipeline, DescriptorSetID("0"), OUT resources ));
		
		bool	cb_was_called	= false;
		bool	data_is_correct	= false;

		const auto	OnLoaded = [&pc, dst_buffer_size, OUT &cb_was_called, OUT &data_is_correct] (BufferView data)
		{
			cb_was_called	= true;
			data_is_correct	= (data.size() == size_t(dst_buffer_size));
			ASSERT( data.Parts().size() == 1 );

			const float*	ptr = Cast<float>( data.Parts().front().data() );
			float			rhs;
			rhs = *(ptr +  0_b);	data_is_correct &= (pc.f3.x == rhs);
			rhs = *(ptr +  4_b);	data_is_correct &= (pc.f3.y == rhs);
			rhs = *(ptr +  8_b);	data_is_correct &= (pc.f3.z == rhs);
			rhs = *(ptr + 12_b);	data_is_correct &= (pc.f1   == rhs);
			rhs = *(ptr + 16_b);	data_is_correct &= (float(pc.i2.x) == rhs);
			rhs = *(ptr + 20_b);	data_is_correct &= (float(pc.i2.y) == rhs);
			rhs = *(ptr + 24_b);	data_is_correct &= (0.0f == rhs);
			rhs = *(ptr + 28_b);	data_is_correct &= (1.0f == rhs);
		};

		
		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
		CHECK_ERR( _fgInstance->BeginFrame( submission_graph ));
		CHECK_ERR( frame_graph->Begin( batch_id, 0, EQueueUsage::Graphics ));
		
		resources.BindBuffer( UniformID("SSB"), dst_buffer );

		Task	t_dispatch	= frame_graph->AddTask( DispatchCompute{}.Dispatch({ 1, 1 }).SetPipeline( pipeline )
														.AddResources( DescriptorSetID("0"), &resources )
														.AddPushConstant( PushConstantID("PushConst"), pc ));
		Task	t_read		= frame_graph->AddTask( ReadBuffer{}.SetBuffer( dst_buffer, 0_b, dst_buffer_size ).SetCallback( OnLoaded ).DependsOn( t_dispatch ));

		FG_UNUSED( t_read );

		CHECK_ERR( frame_graph->Execute() );		
		CHECK_ERR( _fgInstance->EndFrame() );
		
		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));
		
		CHECK_ERR( not cb_was_called );
		
		CHECK_ERR( _fgInstance->WaitIdle() );
		CHECK_ERR( cb_was_called );
		CHECK_ERR( data_is_correct );
		
		DeleteResources( pipeline, dst_buffer );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

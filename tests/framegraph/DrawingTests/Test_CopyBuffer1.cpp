// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	This test affects:
		- frame graph building and execution
		- tasks: UpdateBuffer, CopyBuffer, ReadBuffer
		- resources: buffer
		- staging buffers
		- memory managment
*/

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_CopyBuffer1 ()
	{
		const BytesU	src_buffer_size = 256_b;
		const BytesU	dst_buffer_size = 512_b;
		
		FGThreadPtr		frame_graph	= _frameGraph1;
		BufferID		src_buffer = CreateBuffer( src_buffer_size, "SrcBuffer" );
		BufferID		dst_buffer = CreateBuffer( dst_buffer_size, "DstBuffer" );

		Array<uint8_t>	src_data;	src_data.resize( size_t(src_buffer_size) );

		for (size_t i = 0; i < src_data.size(); ++i) {
			src_data[i] = uint8_t(i);
		}

		bool	cb_was_called	= false;
		bool	data_is_correct	= false;

		const auto	OnLoaded = [&src_data, dst_buffer_size, OUT &cb_was_called, OUT &data_is_correct] (BufferView data)
		{
			cb_was_called	= true;
			data_is_correct	= (data.size() == size_t(dst_buffer_size));

			for (size_t i = 0; i < src_data.size(); ++i)
			{
				bool	is_equal = (src_data[i] == data[i+128]);
				ASSERT( is_equal );

				data_is_correct &= is_equal;
			}
		};
		
		CommandBatchID		batch_id {"main"};
		SubmissionGraph		submission_graph;
		submission_graph.AddBatch( batch_id );
		
		CHECK_ERR( _frameGraphInst->Begin( submission_graph ));
		CHECK_ERR( frame_graph->Begin( batch_id, 0, EThreadUsage::Graphics ));

		Task	t_update	= frame_graph->AddTask( UpdateBuffer().SetBuffer( src_buffer, 0_b ).SetData( src_data ) );
		Task	t_copy		= frame_graph->AddTask( CopyBuffer().From( src_buffer ).To( dst_buffer ).AddRegion( 0_b, 128_b, 256_b ).DependsOn( t_update ) );
		Task	t_read		= frame_graph->AddTask( ReadBuffer().SetBuffer( dst_buffer, 0_b, 512_b ).SetCallback( OnLoaded ).DependsOn( t_copy ) );
		FG_UNUSED( t_read );

		CHECK_ERR( frame_graph->Compile() );
		CHECK_ERR( _frameGraphInst->Execute() );

		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));

		// after execution 'src_data' was copied to 'src_buffer', 'src_buffer' copied to 'dst_buffer', 'dst_buffer' copied to staging buffer...
		CHECK_ERR( not cb_was_called );
		
		// all staging buffers will be synchronized, all 'ReadBuffer' callbacks will be called.
		CHECK_ERR( _frameGraphInst->WaitIdle() );
		CHECK_ERR( cb_was_called );
		CHECK_ERR( data_is_correct );

		DeleteResources( src_buffer, dst_buffer );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

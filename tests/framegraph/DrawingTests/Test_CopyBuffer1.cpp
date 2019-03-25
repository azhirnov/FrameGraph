// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "../FGApp.h"

namespace FG
{

	bool FGApp::Test_CopyBuffer1 ()
	{
		const BytesU	src_buffer_size = 256_b;
		const BytesU	dst_buffer_size = 512_b;
		
		BufferID		src_buffer	= _frameGraph->CreateBuffer( BufferDesc{ src_buffer_size, EBufferUsage::Transfer }, Default, "SrcBuffer" );
		BufferID		dst_buffer	= _frameGraph->CreateBuffer( BufferDesc{ dst_buffer_size, EBufferUsage::Transfer }, Default, "DstBuffer" );

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

		CommandBuffer	cmd = _frameGraph->Begin( CommandBufferDesc{}.SetDebugFlags( ECompilationDebugFlags::Default ));
		CHECK_ERR( cmd );

		Task	t_update	= cmd->AddTask( UpdateBuffer().SetBuffer( src_buffer ).AddData( src_data ));
		Task	t_copy		= cmd->AddTask( CopyBuffer().From( src_buffer ).To( dst_buffer ).AddRegion( 0_b, 128_b, 256_b ).DependsOn( t_update ));
		Task	t_read		= cmd->AddTask( ReadBuffer().SetBuffer( dst_buffer, 0_b, dst_buffer_size ).SetCallback( OnLoaded ).DependsOn( t_copy ));
		FG_UNUSED( t_read );

		CHECK_ERR( _frameGraph->Execute( cmd ));
		
		// after execution 'src_data' was copied to 'src_buffer', 'src_buffer' copied to 'dst_buffer', 'dst_buffer' copied to staging buffer...
		CHECK_ERR( not cb_was_called );

		// all staging buffers will be synchronized, all 'ReadBuffer' callbacks will be called.
		CHECK_ERR( _frameGraph->WaitIdle() );

		CHECK_ERR( CompareDumps( TEST_NAME ));
		CHECK_ERR( Visualize( TEST_NAME ));
		
		CHECK_ERR( cb_was_called );
		CHECK_ERR( data_is_correct );

		DeleteResources( src_buffer, dst_buffer );

		FG_LOGI( TEST_NAME << " - passed" );
		return true;
	}

}	// FG

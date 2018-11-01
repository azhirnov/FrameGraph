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

	bool FGApp::_DrawTest_CopyBuffer ()
	{
		const BytesU	src_buffer_size = 256_b;
		const BytesU	dst_buffer_size = 512_b;

		BufferID		src_buffer = _CreateBuffer( src_buffer_size, "SrcBuffer" );
		BufferID		dst_buffer = _CreateBuffer( dst_buffer_size, "DstBuffer" );

		Array<uint8_t>	src_data;	src_data.resize( size_t(src_buffer_size) );

		for (size_t i = 0; i < src_data.size(); ++i) {
			src_data[i] = uint8_t(i);
		}

		bool	cb_was_called	= false;
		bool	data_is_correct	= false;

		const auto	OnLoaded = [&src_data, &dst_buffer, dst_buffer_size, OUT &cb_was_called, OUT &data_is_correct] (BufferView data)
		{
			cb_was_called	= true;
			data_is_correct	= (data.size() == size_t(dst_buffer_size));

			for (size_t i = 0; i < src_data.size(); ++i)
			{
				data_is_correct &= (src_data[i] == data[i+128]);
			}
		};

		CHECK( _frameGraphInst->Begin() );
		CHECK( _frameGraph->Begin() );

		Task	t_update	= _frameGraph->AddTask( UpdateBuffer().SetBuffer( src_buffer, 0_b ).SetData( src_data ) );
		Task	t_copy		= _frameGraph->AddTask( CopyBuffer().From( src_buffer ).To( dst_buffer ).AddRegion( 0_b, 128_b, 256_b ).DependsOn( t_update ) );
		Task	t_read		= _frameGraph->AddTask( ReadBuffer().SetBuffer( dst_buffer, 0_b, 512_b ).SetCallback( OnLoaded ).DependsOn( t_copy ) );
		(void)(t_read);

		CHECK( _frameGraph->Compile() );
		CHECK( _frameGraphInst->Execute() );

		// after execution 'src_data' was copied to 'src_buffer', 'src_buffer' copied to 'dst_buffer', 'dst_buffer' copied to staging buffer...
		CHECK_ERR( not cb_was_called );
		
		// all staging buffers will be synchronized, all 'ReadBuffer' callbacks will be called.
		_frameGraphInst->WaitIdle();

		CHECK_ERR( cb_was_called );
		CHECK_ERR( data_is_correct );

		_DeleteResources( src_buffer, dst_buffer );

        FG_LOGI( "DrawTest_CopyBuffer - passed" );
		return true;
	}

}	// FG

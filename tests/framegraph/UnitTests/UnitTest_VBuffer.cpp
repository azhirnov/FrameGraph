// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VLocalBuffer.h"
#include "VBarrierManager.h"
#include "framegraph/Public/FrameGraph.h"
#include "UnitTest_Common.h"
#include "DummyTask.h"


namespace FG
{
	class VBufferUnitTest
	{
	public:
		using Barrier = VLocalBuffer::BufferAccess;

		static bool Create (VBuffer &buf, const BufferDesc &desc)
		{
			buf._desc = desc;
			return true;
		}

		static ArrayView<Barrier>  GetReadBarriers (const VLocalBuffer *buf) {
			return buf->_accessForRead;
		}
		
		static ArrayView<Barrier>  GetWriteBarriers (const VLocalBuffer *buf) {
			return buf->_accessForWrite;
		}
	};

	using BufferState = VLocalBuffer::BufferState;
}	// FG


static void VBuffer_Test1 ()
{
	VBarrierManager		barrier_mngr;

	const auto			tasks		= GenDummyTasks( 30 );
	auto				task_iter	= tasks.begin();

	VBuffer				global_buffer;
	VLocalBuffer		local_buffer;
	VLocalBuffer const*	buf			= &local_buffer;

	TEST( VBufferUnitTest::Create( global_buffer, BufferDesc{ 1024_b, EBufferUsage::All } ));

	TEST( local_buffer.Create( &global_buffer ));


	// pass 1
	{
		buf->AddPendingState(BufferState{ EResourceState::TransferDst, 0, 512, (task_iter++)->get() });
		buf->CommitBarrier( barrier_mngr, null );

		auto	w_barriers = VBufferUnitTest::GetWriteBarriers( buf );

		TEST( w_barriers.size() == 1 );
		TEST( w_barriers.back().range.begin == 0 );
		TEST( w_barriers.back().range.end == 512 );
		TEST( w_barriers.back().stages == VK_PIPELINE_STAGE_TRANSFER_BIT );
		TEST( w_barriers.back().access == VK_ACCESS_TRANSFER_WRITE_BIT );
		TEST( w_barriers.back().isReadable == false );
		TEST( w_barriers.back().isWritable == true );
	}

	// pass 2
	{
		buf->AddPendingState(BufferState{ EResourceState::TransferSrc, 0, 64, (task_iter++)->get() });
		buf->CommitBarrier( barrier_mngr, null );

		auto	r_barriers = VBufferUnitTest::GetReadBarriers( buf );

		TEST( r_barriers.size() == 1 );
		TEST( r_barriers.back().range.begin == 0 );
		TEST( r_barriers.back().range.end == 64 );
		TEST( r_barriers.back().stages == VK_PIPELINE_STAGE_TRANSFER_BIT );
		TEST( r_barriers.back().access == VK_ACCESS_TRANSFER_READ_BIT );
		TEST( r_barriers.back().isReadable == true );
		TEST( r_barriers.back().isWritable == false );
	}
	
	// pass 3
	{
		buf->AddPendingState(BufferState{ EResourceState::UniformRead | EResourceState::_VertexShader, 64, 64+64, (task_iter++)->get() });
		buf->CommitBarrier( barrier_mngr, null );

		auto	r_barriers = VBufferUnitTest::GetReadBarriers( buf );

		TEST( r_barriers.size() == 2 );
		TEST( r_barriers.back().range.begin == 64 );
		TEST( r_barriers.back().range.end == (64+64) );
		TEST( r_barriers.back().stages == VK_PIPELINE_STAGE_VERTEX_SHADER_BIT );
		TEST( r_barriers.back().access == VK_ACCESS_UNIFORM_READ_BIT );
		TEST( r_barriers.back().isReadable == true );
		TEST( r_barriers.back().isWritable == false );
	}
	
	// pass 4
	{
		buf->AddPendingState(BufferState{ EResourceState::UniformRead | EResourceState::_FragmentShader, 256, 256+64, (task_iter++)->get() });
		buf->CommitBarrier( barrier_mngr, null );

		auto	r_barriers = VBufferUnitTest::GetReadBarriers( buf );

		TEST( r_barriers.size() == 3 );
		TEST( r_barriers.back().range.begin == 256 );
		TEST( r_barriers.back().range.end == (256+64) );
		TEST( r_barriers.back().stages == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT );
		TEST( r_barriers.back().access == VK_ACCESS_UNIFORM_READ_BIT );
		TEST( r_barriers.back().isReadable == true );
		TEST( r_barriers.back().isWritable == false );
	}
	
	// pass 5
	{
		buf->AddPendingState(BufferState{ EResourceState::ShaderWrite | EResourceState::_ComputeShader, 512, 512+64, (task_iter++)->get() });
		buf->CommitBarrier( barrier_mngr, null );
		
		auto	r_barriers = VBufferUnitTest::GetReadBarriers( buf );
		auto	w_barriers = VBufferUnitTest::GetWriteBarriers( buf );
		
		TEST( r_barriers.size() == 3 );
		TEST( w_barriers.size() == 2 );

		TEST( w_barriers.back().range.begin == 512 );
		TEST( w_barriers.back().range.end == (512+64) );
		TEST( w_barriers.back().stages == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
		TEST( w_barriers.back().access == VK_ACCESS_SHADER_WRITE_BIT );
		TEST( w_barriers.back().isReadable == false );
		TEST( w_barriers.back().isWritable == true );
	}
	
	// pass 6
	{
		buf->AddPendingState(BufferState{ EResourceState::UniformRead | EResourceState::_VertexShader, 256+32, 256+64, (task_iter++)->get() });
		buf->CommitBarrier( barrier_mngr, null );

		auto	r_barriers = VBufferUnitTest::GetReadBarriers( buf );

		TEST( r_barriers.size() == 4 );

		TEST( r_barriers[2].range.begin == 256 );
		TEST( r_barriers[2].range.end == (256+32) );
		TEST( r_barriers[2].stages == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT );
		TEST( r_barriers[2].access == VK_ACCESS_UNIFORM_READ_BIT );
		TEST( r_barriers[2].isReadable == true );
		TEST( r_barriers[2].isWritable == false );
		TEST( r_barriers[2].index == ExeOrderIndex(4) );
		
		TEST( r_barriers[3].range.begin == (256+32) );
		TEST( r_barriers[3].range.end == (256+64) );
		TEST( r_barriers[3].stages == VK_PIPELINE_STAGE_VERTEX_SHADER_BIT );
		TEST( r_barriers[3].access == VK_ACCESS_UNIFORM_READ_BIT );
		TEST( r_barriers[3].isReadable == true );
		TEST( r_barriers[3].isWritable == false );
		TEST( r_barriers[3].index == ExeOrderIndex(6) );
	}
	
	// pass 7
	{
		buf->AddPendingState(BufferState{ EResourceState::UniformRead | EResourceState::_GeometryShader, 256+16, 256+16+32, (task_iter++)->get() });
		buf->CommitBarrier( barrier_mngr, null );

		auto	r_barriers = VBufferUnitTest::GetReadBarriers( buf );

		TEST( r_barriers.size() == 5 );

		TEST( r_barriers[2].range.begin == 256 );
		TEST( r_barriers[2].range.end == (256+16) );
		TEST( r_barriers[2].stages == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT );
		TEST( r_barriers[2].access == VK_ACCESS_UNIFORM_READ_BIT );
		TEST( r_barriers[2].isReadable == true );
		TEST( r_barriers[2].isWritable == false );
		TEST( r_barriers[2].index == ExeOrderIndex(4) );
		
		TEST( r_barriers[3].range.begin == (256+16) );
		TEST( r_barriers[3].range.end == (256+16+32) );
		TEST( r_barriers[3].stages == VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT );
		TEST( r_barriers[3].access == VK_ACCESS_UNIFORM_READ_BIT );
		TEST( r_barriers[3].isReadable == true );
		TEST( r_barriers[3].isWritable == false );
		TEST( r_barriers[3].index == ExeOrderIndex(7) );
		
		TEST( r_barriers[4].range.begin == (256+16+32) );
		TEST( r_barriers[4].range.end == (256+64) );
		TEST( r_barriers[4].stages == VK_PIPELINE_STAGE_VERTEX_SHADER_BIT );
		TEST( r_barriers[4].access == VK_ACCESS_UNIFORM_READ_BIT );
		TEST( r_barriers[4].isReadable == true );
		TEST( r_barriers[4].isWritable == false );
		TEST( r_barriers[4].index == ExeOrderIndex(6) );
	}

	// pass 8
	{
		buf->AddPendingState(BufferState{ EResourceState::UniformRead | EResourceState::_GeometryShader, 16, 32, (task_iter++)->get() });
		buf->CommitBarrier( barrier_mngr, null );

		auto	r_barriers = VBufferUnitTest::GetReadBarriers( buf );

		TEST( r_barriers.size() == 7 );

		TEST( r_barriers[0].range.begin == 0 );
		TEST( r_barriers[0].range.end == 16 );
		TEST( r_barriers[0].stages == VK_PIPELINE_STAGE_TRANSFER_BIT );
		TEST( r_barriers[0].access == VK_ACCESS_TRANSFER_READ_BIT );
		TEST( r_barriers[0].isReadable == true );
		TEST( r_barriers[0].isWritable == false );
		TEST( r_barriers[0].index == ExeOrderIndex(2) );
		
		TEST( r_barriers[1].range.begin == 16 );
		TEST( r_barriers[1].range.end == 32 );
		TEST( r_barriers[1].stages == VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT );
		TEST( r_barriers[1].access == VK_ACCESS_UNIFORM_READ_BIT );
		TEST( r_barriers[1].isReadable == true );
		TEST( r_barriers[1].isWritable == false );
		TEST( r_barriers[1].index == ExeOrderIndex(8) );
		
		TEST( r_barriers[2].range.begin == 32 );
		TEST( r_barriers[2].range.end == 64 );
		TEST( r_barriers[2].stages == VK_PIPELINE_STAGE_TRANSFER_BIT );
		TEST( r_barriers[2].access == VK_ACCESS_TRANSFER_READ_BIT );
		TEST( r_barriers[2].isReadable == true );
		TEST( r_barriers[2].isWritable == false );
		TEST( r_barriers[2].index == ExeOrderIndex(2) );

		TEST( r_barriers[3].range.begin == 64 );
		TEST( r_barriers[3].range.end == (64+64) );
		TEST( r_barriers[3].stages == VK_PIPELINE_STAGE_VERTEX_SHADER_BIT );
		TEST( r_barriers[3].access == VK_ACCESS_UNIFORM_READ_BIT );
		TEST( r_barriers[3].isReadable == true );
		TEST( r_barriers[3].isWritable == false );
		TEST( r_barriers[3].index == ExeOrderIndex(3) );
	}

	// pass 9
	{
		buf->AddPendingState(BufferState{ EResourceState::ShaderRead | EResourceState::_ComputeShader, 0, 256+32, (task_iter++)->get() });
		buf->CommitBarrier( barrier_mngr, null );

		auto	r_barriers = VBufferUnitTest::GetReadBarriers( buf );

		TEST( r_barriers.size() == 3 );
		
		TEST( r_barriers[0].range.begin == 0 );
		TEST( r_barriers[0].range.end == (256+32) );
		TEST( r_barriers[0].stages == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
		TEST( r_barriers[0].access == VK_ACCESS_SHADER_READ_BIT );
		TEST( r_barriers[0].isReadable == true );
		TEST( r_barriers[0].isWritable == false );
		TEST( r_barriers[0].index == ExeOrderIndex(9) );
		
		TEST( r_barriers[1].range.begin == (256+32) );
		TEST( r_barriers[1].range.end == (256+16+32) );
		TEST( r_barriers[1].stages == VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT );
		TEST( r_barriers[1].access == VK_ACCESS_UNIFORM_READ_BIT );
		TEST( r_barriers[1].isReadable == true );
		TEST( r_barriers[1].isWritable == false );
		TEST( r_barriers[1].index == ExeOrderIndex(7) );
		
		TEST( r_barriers[2].range.begin == (256+16+32) );
		TEST( r_barriers[2].range.end == (256+64) );
		TEST( r_barriers[2].stages == VK_PIPELINE_STAGE_VERTEX_SHADER_BIT );
		TEST( r_barriers[2].access == VK_ACCESS_UNIFORM_READ_BIT );
		TEST( r_barriers[2].isReadable == true );
		TEST( r_barriers[2].isWritable == false );
		TEST( r_barriers[2].index == ExeOrderIndex(6) );
	}

	
	// pass 10
	{
		buf->AddPendingState(BufferState{ EResourceState::TransferDst, 64, 512, (task_iter++)->get() });
		buf->CommitBarrier( barrier_mngr, null );
		
		auto	r_barriers = VBufferUnitTest::GetReadBarriers( buf );
		auto	w_barriers = VBufferUnitTest::GetWriteBarriers( buf );

		TEST( r_barriers.size() == 1 );
		TEST( w_barriers.size() == 3 );
		
		TEST( r_barriers[0].range.begin == 0 );
		TEST( r_barriers[0].range.end == 64 );
		TEST( r_barriers[0].stages == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
		TEST( r_barriers[0].access == VK_ACCESS_SHADER_READ_BIT );
		TEST( r_barriers[0].isReadable == true );
		TEST( r_barriers[0].isWritable == false );
		TEST( r_barriers[0].index == ExeOrderIndex(9) );
		
		TEST( w_barriers[0].range.begin == 0 );
		TEST( w_barriers[0].range.end == 64 );
		TEST( w_barriers[0].stages == VK_PIPELINE_STAGE_TRANSFER_BIT );
		TEST( w_barriers[0].access == VK_ACCESS_TRANSFER_WRITE_BIT );
		TEST( w_barriers[0].isReadable == false );
		TEST( w_barriers[0].isWritable == true );
		TEST( w_barriers[0].index == ExeOrderIndex(1) );

		TEST( w_barriers[1].range.begin == 64 );
		TEST( w_barriers[1].range.end == 512 );
		TEST( w_barriers[1].stages == VK_PIPELINE_STAGE_TRANSFER_BIT );
		TEST( w_barriers[1].access == VK_ACCESS_TRANSFER_WRITE_BIT );
		TEST( w_barriers[1].isReadable == false );
		TEST( w_barriers[1].isWritable == true );
		TEST( w_barriers[1].index == ExeOrderIndex(10) );

		TEST( w_barriers[2].range.begin == 512 );
		TEST( w_barriers[2].range.end == (512+64) );
		TEST( w_barriers[2].stages == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
		TEST( w_barriers[2].access == VK_ACCESS_SHADER_WRITE_BIT );
		TEST( w_barriers[2].isReadable == false );
		TEST( w_barriers[2].isWritable == true );
		TEST( w_barriers[2].index == ExeOrderIndex(5) );
	}

	local_buffer.ResetState( ExeOrderIndex::Final, barrier_mngr, null );

	local_buffer.Destroy();
	//global_buffer.Destroy();
}


extern void UnitTest_VBuffer ()
{
	VBuffer_Test1();
	FG_LOGI( "UnitTest_VBuffer - passed" );
}

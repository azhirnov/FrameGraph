// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VCommandBuffer.h"
#include "VTaskGraph.hpp"

namespace FG
{
namespace {
	static constexpr auto	GraphicsBit		= EQueueUsage::Graphics;
	static constexpr auto	ComputeBit		= EQueueUsage::Graphics | EQueueUsage::AsyncCompute;
	static constexpr auto	RayTracingBit	= EQueueUsage::Graphics | EQueueUsage::AsyncCompute;
	static constexpr auto	TransferBit		= EQueueUsage::Graphics | EQueueUsage::AsyncCompute | EQueueUsage::AsyncTransfer;

	static constexpr auto	CmdDebugFlags	= EDebugFlags::FullBarrier | EDebugFlags::QueueSync;
}
	
/*
=================================================
	constructor
=================================================
*/
	VCommandBuffer::VCommandBuffer (VFrameGraph &fg, uint index) :
		_state{ EState::Initial },
		_queueIndex{ Default },
		_instance{ fg },
		_indexInPool{ index }
	{
		_mainAllocator.SetBlockSize( 16_Mb );

		_ResetLocalRemaping();
	}
	
/*
=================================================
	destructor
=================================================
*/
	VCommandBuffer::~VCommandBuffer ()
	{
		EXLOCK( _drCheck );
		CHECK( _state == EState::Initial );

		for (auto& q : _perQueue) {
			q.Destroy( GetDevice() );
		}
		_perQueue.clear();
	}

/*
=================================================
	Begin
=================================================
*/
	bool  VCommandBuffer::Begin (const CommandBufferDesc &desc, const VCmdBatchPtr &batch, VDeviceQueueInfoPtr queue)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( batch );
		CHECK_ERR( _state == EState::Initial );

		_batch			= batch;
		_dbgName		= desc.name;
		_dbgFullBarriers= EnumEq( desc.debugFlags, EDebugFlags::FullBarrier );
		_dbgQueueSync	= EnumEq( desc.debugFlags, EDebugFlags::QueueSync );
		_state			= EState::Recording;
		_queueIndex		= queue->familyIndex;
		
		// create command pool
		{
			const uint	index = uint(_queueIndex);

			_perQueue.resize( Max( _perQueue.size(), index+1 ));
			
			auto&	pool = _perQueue[index];

			if ( not pool.IsCreated() )
			{
				CHECK_ERR( pool.Create( GetDevice(), queue ));
			}
		}
		
		_batch->OnBegin( desc );
		
		// setup local debugger
		const EDebugFlags	debugger_flags = desc.debugFlags & ~CmdDebugFlags;

		if ( debugger_flags != Default )
		{
			if ( not _debugger )
				_debugger.reset( new VLocalDebugger{} );
			
			_debugger->Begin( debugger_flags );
		}
		else
			_debugger.reset();

		_taskGraph.OnStart( GetAllocator() );
		return true;
	}
	
/*
=================================================
	Execute
=================================================
*/
	bool  VCommandBuffer::Execute ()
	{
		using TimePoint_t = std::chrono::high_resolution_clock::time_point;

		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		
		const auto	start_time = TimePoint_t::clock::now();

		_state = EState::Compiling;

		CHECK_ERR( _BuildCommandBuffers() );
		
		if_unlikely( _debugger )
			_debugger->End( GetName(), _indexInPool, OUT &_batch->_debugDump, OUT &_batch->_debugGraph );

		CHECK_ERR( _batch->OnBaked( INOUT _rm.resourceMap ));
		
		_taskGraph.OnDiscardMemory();
		_AfterCompilation();
		_mainAllocator.Discard();
		
		EditStatistic().renderer.cpuTime += TimePoint_t::clock::now() - start_time;
		_batch = null;

		_state = EState::Initial;
		return true;
	}
	
/*
=================================================
	_AfterCompilation
=================================================
*/
	void  VCommandBuffer::_AfterCompilation ()
	{
		// reset global shader debugger
		{
			_shaderDbg.timemapIndex		= Default;
			_shaderDbg.timemapStages	= Default;
		}

		// destroy logical render passes
		for (uint i = 0; i < _rm.logicalRenderPassCount; ++i)
		{
			auto&	rp = _rm.logicalRenderPasses[ Index_t(i) ];

			if ( not rp.IsDestroyed() )
			{
				rp.Destroy( GetResourceManager() );
				_rm.logicalRenderPasses.Unassign( Index_t(i) );
			}
		}
		_rm.logicalRenderPassCount = 0;
	}

/*
=================================================
	SignalSemaphore
=================================================
*/
	void  VCommandBuffer::SignalSemaphore (VkSemaphore sem)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _state == EState::Recording or _state == EState::Compiling, void());

		_batch->SignalSemaphore( sem );
	}
	
/*
=================================================
	WaitSemaphore
=================================================
*/
	void  VCommandBuffer::WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _state == EState::Recording or _state == EState::Compiling, void());

		_batch->WaitSemaphore( sem, stage );
	}

/*
=================================================
	_BuildCommandBuffers
=================================================
*/
	bool  VCommandBuffer::_BuildCommandBuffers ()
	{
		//if ( _taskGraph.Empty() )
		//	return true;

		VkCommandBuffer		cmd;
		VDevice const&		dev	= GetDevice();
		
		// create command buffer
		{
			auto&	pool = _perQueue[ uint(_queueIndex) ];
			
			cmd = pool.AllocPrimary( dev );
			_batch->PushBackCommandBuffer( cmd, &pool );
		}

		// begin
		{
			VkCommandBufferBeginInfo	info = {};
			info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CALL( dev.vkBeginCommandBuffer( cmd, &info ));
			_batch->OnBeginRecording( cmd );

			VkMemoryBarrier	barrier = {};
			barrier.sType			= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask	= VK_ACCESS_HOST_WRITE_BIT;
			barrier.dstAccessMask	= VK_ACCESS_TRANSFER_READ_BIT;
			_barrierMngr.AddMemoryBarrier( VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, barrier );
		}

		// commit image layout transition and other
		_barrierMngr.Commit( dev, cmd );

		CHECK( _ProcessTasks( cmd ));

		// transit image layout to default state
		// add memory dependency to flush caches
		{
			VkMemoryBarrier	barrier = {};
			barrier.sType			= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask	= VK_ACCESS_HOST_READ_BIT;
			_barrierMngr.AddMemoryBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, barrier );

			_FlushLocalResourceStates( ExeOrderIndex::Final, _barrierMngr, GetDebugger() );
			_barrierMngr.ForceCommit( dev, cmd, dev.GetAllWritableStages(), dev.GetAllReadableStages() );
		}

		// end
		{
			_batch->OnEndRecording( cmd );
			VK_CALL( dev.vkEndCommandBuffer( cmd ));
		}
		return true;
	}
	
/*
=================================================
	VTaskProcessor::Run
=================================================
*/
	forceinline void  VTaskProcessor::Run (VTask node)
	{
		// reset states
		_currTask = node;
		
		if_unlikely( _fgThread.GetDebugger() )
			_fgThread.GetDebugger()->AddTask( _currTask );

		node->Process( this );
	}

/*
=================================================
	_ProcessTasks
=================================================
*/
	bool  VCommandBuffer::_ProcessTasks (VkCommandBuffer cmd)
	{
		VTaskProcessor	processor{ *this, cmd };
		uint			visitor_id		= 1;
		ExeOrderIndex	exe_order_index	= ExeOrderIndex::First;

		TempTaskArray_t		pending{ GetAllocator() };
		pending.reserve( 128 );
		pending.assign( _taskGraph.Entries().begin(), _taskGraph.Entries().end() );

		for (uint k = 0; k < 10 and not pending.empty(); ++k)
		{
			for (size_t i = 0; i < pending.size();)
			{
				auto	node = pending[i];
				
				if ( node->VisitorID() == visitor_id ) {
					++i;
					continue;
				}

				// wait for input
				bool	input_processed = true;

				for (auto in_node : node->Inputs())
				{
					if ( in_node->VisitorID() != visitor_id ) {
						input_processed = false;
						break;
					}
				}

				if ( not input_processed ) {
					++i;
					continue;
				}

				node->SetVisitorID( visitor_id );
				node->SetExecutionOrder( ++exe_order_index );
				
				processor.Run( node );

				for (auto out_node : node->Outputs())
				{
					pending.push_back( out_node );
				}

				pending.erase( pending.begin()+i );
			}
		}
		return true;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	GetSwapchainImage
=================================================
*/
	RawImageID	VCommandBuffer::GetSwapchainImage (RawSwapchainID swapchainId, ESwapchainImage type)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );

		auto*	swapchain = AcquireTemporary( swapchainId );
		CHECK_ERR( swapchain );

		RawImageID	id;
		CHECK_ERR( swapchain->Acquire( *this, type, _dbgQueueSync, OUT id ));

		// transit to undefined layout
		AcquireImage( id, true, true );

		bool	found = false;

		for (auto& sw : _batch->_swapchains)
		{
			if ( sw == swapchain )
			{
				found = true;
				break;
			}
		}

		if ( not found )
			_batch->_swapchains.push_back( swapchain );

		return id;
	}

/*
=================================================
	AddExternalCommands
=================================================
*/
	bool  VCommandBuffer::AddExternalCommands (const ExternalCmdBatch_t &desc)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );

		auto*	info = UnionGetIf< VulkanCommandBatch >( &desc );
		CHECK_ERR( info );
		CHECK_ERR( info->queueFamilyIndex == uint(_queueIndex) );

		for (auto& cmd : info->commands) {
			_batch->PushBackCommandBuffer( BitCast<VkCommandBuffer>(cmd), null );
		}
		for (auto& sem : info->signalSemaphores) {
			_batch->SignalSemaphore( BitCast<VkSemaphore>(sem) );
		}
		for (size_t i = 0; i < info->waitSemaphores.size(); ++i) {
			_batch->WaitSemaphore( BitCast<VkSemaphore>(info->waitSemaphores[i].first), info->waitSemaphores[i].second );
		}

		return true;
	}
	
/*
=================================================
	AddDependency
=================================================
*/
	bool  VCommandBuffer::AddDependency (const CommandBuffer &cmd)
	{
		if ( not cmd.GetBatch() or cmd.GetCommandBuffer() == this )
			return false;

		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );

		_batch->AddDependency( Cast<VCmdBatch>(cmd.GetBatch()) );
		return true;
	}
	
/*
=================================================
	AllocBuffer
=================================================
*/
	bool  VCommandBuffer::AllocBuffer (BytesU size, BytesU align, OUT RawBufferID &buffer, OUT BytesU &offset, OUT void* &mapped)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _state == EState::Recording or _state == EState::Compiling );

		BytesU	buf_size;
		return _batch->GetWritable( size, 1_b, align, size, OUT buffer, OUT offset, OUT buf_size, OUT mapped );
	}

/*
=================================================
	AcquireImage
=================================================
*/
	void  VCommandBuffer::AcquireImage (RawImageID id, bool makeMutable, bool invalidate)
	{
		EXLOCK( _drCheck );
		CHECK( _IsRecording() );

		auto*	image = ToLocal( id );
		if ( image ) {
			image->SetInitialState( not makeMutable, invalidate );
		}
	}
	
/*
=================================================
	AcquireBuffer
=================================================
*/
	void  VCommandBuffer::AcquireBuffer (RawBufferID id, bool makeMutable)
	{
		EXLOCK( _drCheck );
		CHECK( _IsRecording() );

		auto*	buffer = ToLocal( id );
		if ( buffer ) {
			buffer->SetInitialState( not makeMutable );
		}
	}

/*
=================================================
	AddTask (SubmitRenderPass)
=================================================
*/
	Task  VCommandBuffer::AddTask (const SubmitRenderPass &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( GraphicsBit, _GetQueueUsage() ));

		// TODO: add scale to shader timemap

		auto	rp_task = _taskGraph.Add( *this, task );
		
		if ( EnumEq( _shaderDbg.timemapStages, EShaderStages::Fragment ) and _shaderDbg.timemapIndex != Default )
		{
			rp_task->GetLogicalPass()->_SetShaderDebugIndex( _shaderDbg.timemapIndex );
		}

		// TODO
		//_renderPassGraph.Add( rp_task );

		return rp_task;
	}
	
/*
=================================================
	AddTask (DispatchCompute)
=================================================
*/
	Task  VCommandBuffer::AddTask (const DispatchCompute &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( ComputeBit, _GetQueueUsage() ));
		ASSERT( task.pipeline );

		auto	result = _taskGraph.Add( *this, task );
		
		if ( EnumEq( _shaderDbg.timemapStages, EShaderStages::Compute ) and _shaderDbg.timemapIndex != Default )
		{
			ASSERT( result->debugModeIndex == Default );
			result->debugModeIndex = _shaderDbg.timemapIndex;
		}

		return result;
	}
	
/*
=================================================
	AddTask (DispatchComputeIndirect)
=================================================
*/
	Task  VCommandBuffer::AddTask (const DispatchComputeIndirect &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( ComputeBit, _GetQueueUsage() ));
		ASSERT( task.pipeline );
		
		auto	result = _taskGraph.Add( *this, task );
		
		if ( EnumEq( _shaderDbg.timemapStages, EShaderStages::Compute ) and _shaderDbg.timemapIndex != Default )
		{
			ASSERT( result->debugModeIndex == Default );
			result->debugModeIndex = _shaderDbg.timemapIndex;
		}

		return result;
	}
	
/*
=================================================
	AddTask (CopyBuffer)
=================================================
*/
	Task  VCommandBuffer::AddTask (const CopyBuffer &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( TransferBit, _GetQueueUsage() ));
		
		if ( task.regions.empty() )
			return null;	// TODO: is it an error?

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (CopyImage)
=================================================
*/
	Task  VCommandBuffer::AddTask (const CopyImage &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( TransferBit, _GetQueueUsage() ));
		
		if ( task.regions.empty() )
			return null;	// TODO: is it an error?

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (CopyBufferToImage)
=================================================
*/
	Task  VCommandBuffer::AddTask (const CopyBufferToImage &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( TransferBit, _GetQueueUsage() ));
		
		if ( task.regions.empty() )
			return null;	// TODO: is it an error?

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (CopyImageToBuffer)
=================================================
*/
	Task  VCommandBuffer::AddTask (const CopyImageToBuffer &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( TransferBit, _GetQueueUsage() ));
		
		if ( task.regions.empty() )
			return null;	// TODO: is it an error?

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (BlitImage)
=================================================
*/
	Task  VCommandBuffer::AddTask (const BlitImage &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( GraphicsBit, _GetQueueUsage() ));
		
		if ( task.regions.empty() )
			return null;	// TODO: is it an error?

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (ResolveImage)
=================================================
*/
	Task  VCommandBuffer::AddTask (const ResolveImage &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( GraphicsBit, _GetQueueUsage() ));
		
		if ( task.regions.empty() )
			return null;	// TODO: is it an error?

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (GenerateMipmaps)
=================================================
*/
	Task  VCommandBuffer::AddTask (const GenerateMipmaps &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( GraphicsBit, _GetQueueUsage() ));

		return _taskGraph.Add( *this, task );
	}

/*
=================================================
	AddTask (FillBuffer)
=================================================
*/
	Task  VCommandBuffer::AddTask (const FillBuffer &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (ClearColorImage)
=================================================
*/
	Task  VCommandBuffer::AddTask (const ClearColorImage &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( ComputeBit, _GetQueueUsage() ));

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (ClearDepthStencilImage)
=================================================
*/
	Task  VCommandBuffer::AddTask (const ClearDepthStencilImage &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( ComputeBit, _GetQueueUsage() ));

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (UpdateBuffer)
=================================================
*/
	Task  VCommandBuffer::AddTask (const UpdateBuffer &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( TransferBit, _GetQueueUsage() ));

		if ( task.regions.empty() )
			return null;	// TODO: is it an error?

		return _AddUpdateBufferTask( task );
	}
	
/*
=================================================
	_AddUpdateBufferTask
=================================================
*/
	Task  VCommandBuffer::_AddUpdateBufferTask (const UpdateBuffer &task)
	{
		CopyBuffer		copy;
		copy.taskName	= task.taskName;
		copy.debugColor	= task.debugColor;
		copy.depends	= task.depends;
		copy.dstBuffer	= task.dstBuffer;

		// copy to staging buffer
		for (auto& reg : task.regions)
		{
			for (BytesU readn; readn < ArraySizeOf(reg.data);)
			{
				RawBufferID		src_buffer;
				BytesU			off, size;
				CHECK_ERR( _StorePartialData( reg.data, readn, OUT src_buffer, OUT off, OUT size ));
			
				if ( copy.srcBuffer and src_buffer != copy.srcBuffer )
				{
					Task	last_task = AddTask( copy );
					copy.regions.clear();
					copy.depends.clear();
					copy.depends.push_back( last_task );
				}

				copy.AddRegion( off, reg.offset + readn, size );

				readn		  += size;
				copy.srcBuffer = src_buffer;
			}
		}

		return AddTask( copy );
	}
	
/*
=================================================
	AddTask (UpdateImage)
=================================================
*/
	Task  VCommandBuffer::AddTask (const UpdateImage &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( TransferBit, _GetQueueUsage() ));
		
		if ( All( task.imageSize == Zero ))
			return null;	// TODO: is it an error?

		return _AddUpdateImageTask( task );
	}
	
/*
=================================================
	_AddUpdateImageTask
=================================================
*/
	Task  VCommandBuffer::_AddUpdateImageTask (const UpdateImage &task)
	{
		CHECK_ERR( task.dstImage );
		ImageDesc const&	img_desc = AcquireTemporary( task.dstImage )->Description();

		ASSERT( task.mipmapLevel < img_desc.maxLevel );
		ASSERT( task.arrayLayer < img_desc.arrayLayers );
		ASSERT(Any( task.imageSize > Zero ));

		const uint3		image_size		= Max( task.imageSize, 1u );
		const auto&		fmt_info		= EPixelFormat_GetInfo( img_desc.format );
		const auto&		block_dim		= fmt_info.blockSize;
		const uint		block_size		= task.aspectMask != EImageAspect::Stencil ? fmt_info.bitsPerBlock : fmt_info.bitsPerBlock2;
		const BytesU	row_pitch		= Max( task.dataRowPitch, BytesU(image_size.x * block_size + block_dim.x-1) / (block_dim.x * 8) );
		const BytesU	min_slice_pitch	= (image_size.y * row_pitch + block_dim.y-1) / block_dim.y;
		const BytesU	slice_pitch		= Max( task.dataSlicePitch, min_slice_pitch );
		const BytesU	total_size		= image_size.z > 1 ? slice_pitch * image_size.z : min_slice_pitch;

		CHECK_ERR( total_size == ArraySizeOf(task.data) );

		const BytesU		min_size	= _instance.GetResourceManager().GetHostWriteBufferSize() / 4;
		const uint			row_length	= CheckCast<uint>((row_pitch * block_dim.x * 8) / block_size);
		const uint			img_height	= CheckCast<uint>((slice_pitch * block_dim.y) / row_pitch);
		CopyBufferToImage	copy;

		copy.taskName	= task.taskName;
		copy.debugColor	= task.debugColor;
		copy.depends	= task.depends;
		copy.dstImage	= task.dstImage;

		ASSERT( task.imageOffset.x % block_dim.x == 0 );
		ASSERT( task.imageOffset.y % block_dim.y == 0 );

		// copy to staging buffer slice by slice
		if ( total_size < min_size )
		{
			uint	z_offset = 0;
			for (BytesU readn; readn < total_size;)
			{
				RawBufferID		src_buffer;
				BytesU			off, size;
				CHECK_ERR( _StoreImageData( task.data, readn, slice_pitch, total_size, OUT src_buffer, OUT off, OUT size ));
				
				if ( copy.srcBuffer and src_buffer != copy.srcBuffer )
				{
					Task	last_task = AddTask( copy );
					copy.regions.clear();
					copy.depends.clear();
					copy.depends.push_back( last_task );
				}

				const uint	z_size = CheckCast<uint>(size / slice_pitch);

				ASSERT( image_size.x % block_dim.x == 0 );
				ASSERT( image_size.y % block_dim.y == 0 );

				copy.AddRegion( off, row_length, img_height,
								ImageSubresourceRange{ task.mipmapLevel, task.arrayLayer, 1, task.aspectMask },
								task.imageOffset + int3(0, 0, z_offset), uint3(image_size.x, image_size.y, z_size) );

				readn		  += size;
				z_offset	  += z_size;
				copy.srcBuffer = src_buffer;
			}
			CHECK( z_offset == image_size.z );
		}
		else

		// copy to staging buffer row by row
		for (uint slice = 0; slice < image_size.z; ++slice)
		{
			uint				y_offset	= 0;
			ArrayView<uint8_t>	data		= task.data.section( size_t(slice * slice_pitch), size_t(slice_pitch) );

			for (BytesU readn; readn < ArraySizeOf(data);)
			{
				RawBufferID		src_buffer;
				BytesU			off, size;
				CHECK_ERR( _StoreImageData( data, readn, row_pitch * block_dim.y, total_size, OUT src_buffer, OUT off, OUT size ));
				
				if ( copy.srcBuffer and src_buffer != copy.srcBuffer )
				{
					Task	last_task = AddTask( copy );
					copy.regions.clear();
					copy.depends.clear();
					copy.depends.push_back( last_task );
				}

				const uint	y_size = CheckCast<uint>((size * block_dim.y) / row_pitch);

				ASSERT( (task.imageOffset.y + y_offset) % block_dim.y == 0 );
				ASSERT( image_size.x % block_dim.x == 0 );
				ASSERT( y_size % block_dim.y == 0 );

				copy.AddRegion( off, row_length, y_size,
								ImageSubresourceRange{ task.mipmapLevel, task.arrayLayer, 1, task.aspectMask },
								task.imageOffset + int3(0, y_offset, slice), uint3(image_size.x, y_size, 1) );

				readn		  += size;
				y_offset	  += y_size;
				copy.srcBuffer = src_buffer;
			}
			CHECK( y_offset == image_size.y );
		}

		return AddTask( copy );
	}
	
/*
=================================================
	AddTask (ReadBuffer)
=================================================
*/
	Task  VCommandBuffer::AddTask (const ReadBuffer &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( TransferBit, _GetQueueUsage() ));
		
		if ( task.size == 0 )
			return null;	// TODO: is it an error?

		return _AddReadBufferTask( task );
	}
	
/*
=================================================
	_AddReadBufferTask
=================================================
*/
	Task  VCommandBuffer::_AddReadBufferTask (const ReadBuffer &task)
	{
		using OnDataLoadedEvent = VCmdBatch::OnBufferDataLoadedEvent;

		CHECK_ERR( task.srcBuffer and task.callback );

		OnDataLoadedEvent	load_event{ task.callback, task.size };
		CopyBuffer			copy;

		copy.taskName	= task.taskName;
		copy.debugColor	= task.debugColor;
		copy.depends	= task.depends;
		copy.srcBuffer	= task.srcBuffer;

		// copy to staging buffer
		for (BytesU written; written < task.size;)
		{
			RawBufferID					dst_buffer;
			OnDataLoadedEvent::Range	range;
			CHECK_ERR( _batch->AddPendingLoad( written, task.size, OUT dst_buffer, OUT range ));
			
			if ( copy.dstBuffer and dst_buffer != copy.dstBuffer )
			{
				Task	last_task = AddTask( copy );
				copy.regions.clear();
				copy.depends.clear();
				copy.depends.push_back( last_task );
			}
			
			copy.AddRegion( task.offset + written, range.offset, range.size );
			load_event.parts.push_back( range );

			written		  += range.size;
			copy.dstBuffer = dst_buffer;
		}

		_batch->AddDataLoadedEvent( std::move(load_event) );
		
		return AddTask( copy );
	}
	
/*
=================================================
	AddTask (ReadImage)
=================================================
*/
	Task  VCommandBuffer::AddTask (const ReadImage &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( TransferBit, _GetQueueUsage() ));
		
		if ( All( task.imageSize == Zero ))
			return null;	// TODO: is it an error?

		return _AddReadImageTask( task );
	}
	
/*
=================================================
	_AddReadImageTask
=================================================
*/
	Task  VCommandBuffer::_AddReadImageTask (const ReadImage &task)
	{
		using OnDataLoadedEvent = VCmdBatch::OnImageDataLoadedEvent;

		CHECK_ERR( task.srcImage and task.callback );
		ImageDesc const&	img_desc = AcquireTemporary( task.srcImage )->Description();

		ASSERT( task.mipmapLevel < img_desc.maxLevel );
		ASSERT( task.arrayLayer < img_desc.arrayLayers );
		ASSERT(Any( task.imageSize > Zero ));
		
		const uint3			image_size		= Max( task.imageSize, 1u );
		const BytesU		min_size		= _instance.GetResourceManager().GetHostReadBufferSize() / 4;
		const auto&			fmt_info		= EPixelFormat_GetInfo( img_desc.format );
		const auto&			block_dim		= fmt_info.blockSize;
		const uint			block_size		= task.aspectMask != EImageAspect::Stencil ? fmt_info.bitsPerBlock : fmt_info.bitsPerBlock2;
		const BytesU		row_pitch		= BytesU(image_size.x * block_size + block_dim.x-1) / (block_dim.x * 8);
		const BytesU		slice_pitch		= (image_size.y * row_pitch + block_dim.y-1) / block_dim.y;
		const BytesU		total_size		= slice_pitch * image_size.z;
		const uint			row_length		= image_size.x;
		const uint			img_height		= image_size.y;

		OnDataLoadedEvent	load_event{ task.callback, total_size, image_size, row_pitch, slice_pitch, img_desc.format, task.aspectMask };
		CopyImageToBuffer	copy;

		copy.taskName	= task.taskName;
		copy.debugColor	= task.debugColor;
		copy.depends	= task.depends;
		copy.srcImage	= task.srcImage;
		
		// copy to staging buffer slice by slice
		if ( total_size < min_size )
		{
			uint	z_offset = 0;
			for (BytesU written; written < total_size;)
			{
				RawBufferID					dst_buffer;
				OnDataLoadedEvent::Range	range;
				CHECK_ERR( _batch->AddPendingLoad( written, total_size, slice_pitch, OUT dst_buffer, OUT range ));
			
				if ( copy.dstBuffer and dst_buffer != copy.dstBuffer )
				{
					Task	last_task = AddTask( copy );
					copy.regions.clear();
					copy.depends.clear();
					copy.depends.push_back( last_task );
				}

				const uint	z_size = CheckCast<uint>(range.size / slice_pitch);

				copy.AddRegion( ImageSubresourceRange{ task.mipmapLevel, task.arrayLayer, 1, task.aspectMask },
								task.imageOffset + int3(0, 0, z_offset), uint3(image_size.x, image_size.y, z_size),
								range.offset, row_length, img_height );
				
				load_event.parts.push_back( range );

				written		  += range.size;
				z_offset	  += z_size;
				copy.dstBuffer = dst_buffer;
			}
			ASSERT( z_offset == image_size.z );
		}
		else

		// copy to staging buffer row by row
		for (uint slice = 0; slice < image_size.z; ++slice)
		{
			uint	y_offset = 0;

			for (BytesU written; written < slice_pitch;)
			{
				RawBufferID					dst_buffer;
				OnDataLoadedEvent::Range	range;
				CHECK_ERR( _batch->AddPendingLoad( written, total_size, row_pitch * block_dim.y, OUT dst_buffer, OUT range ));
				
				if ( copy.dstBuffer and dst_buffer != copy.dstBuffer )
				{
					Task	last_task = AddTask( copy );
					copy.regions.clear();
					copy.depends.clear();
					copy.depends.push_back( last_task );
				}

				const uint	y_size = CheckCast<uint>((range.size * block_dim.y) / row_pitch);

				copy.AddRegion( ImageSubresourceRange{ task.mipmapLevel, task.arrayLayer, 1, task.aspectMask },
								task.imageOffset + int3(0, y_offset, slice), uint3(image_size.x, y_size, 1),
								range.offset, row_length, img_height );
				
				load_event.parts.push_back( range );

				written		  += range.size;
				y_offset	  += y_size;
				copy.dstBuffer = dst_buffer;
			}
			ASSERT( y_offset == image_size.y );
		}

		_batch->AddDataLoadedEvent( std::move(load_event) );

		return AddTask( copy );
	}

/*
=================================================
	AddTask (Present)
=================================================
*/
	Task  VCommandBuffer::AddTask (const Present &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );

		auto	vtask = _taskGraph.Add( *this, task );

		if ( vtask )
			_batch->_swapchains.push_back( vtask->swapchain );

		return vtask;
	}
	
/*
=================================================
	_AllocStorage
=================================================
*/
	template <typename T>
	inline bool  VCommandBuffer::_AllocStorage (size_t count, OUT const VLocalBuffer* &outBuffer, OUT VkDeviceSize &outOffset, OUT T* &outPtr)
	{
		RawBufferID		buffer;
		BytesU			buf_offset, buf_size;
		void*			ptr = null;

		CHECK_ERR( _batch->GetWritable( SizeOf<T> * count, 1_b, 16_b, SizeOf<T> * count,
										OUT buffer, OUT buf_offset, OUT buf_size, OUT ptr ));

		outPtr		= Cast<T>(ptr);
		outBuffer	= ToLocal( buffer );
		outOffset	= VkDeviceSize(buf_offset);
		return true;
	}
	
/*
=================================================
	_StoreData
=================================================
*/
	inline bool  VCommandBuffer::_StoreData (const void *dataPtr, BytesU dataSize, BytesU offsetAlign, OUT const VLocalBuffer* &outBuffer, OUT VkDeviceSize &outOffset)
	{
		RawBufferID		buffer;
		BytesU			buf_offset, buf_size;
		void *			ptr = null;

		if ( _batch->GetWritable( dataSize, 1_b, offsetAlign, dataSize, OUT buffer, OUT buf_offset, OUT buf_size, OUT ptr ))
		{
			outBuffer	= ToLocal( buffer );
			outOffset	= VkDeviceSize(buf_offset);

			MemCopy( ptr, buf_size, dataPtr, buf_size );
			return true;
		}

		return false;
	}
	
/*
=================================================
	AddTask (UpdateRayTracingShaderTable)
=================================================
*/
	Task  VCommandBuffer::AddTask (const UpdateRayTracingShaderTable &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( RayTracingBit, _GetQueueUsage() ));

		return _taskGraph.Add( *this, task );
	}

/*
=================================================
	AddTask (BuildRayTracingGeometry)
=================================================
*/
	Task  VCommandBuffer::AddTask (const BuildRayTracingGeometry &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( RayTracingBit, _GetQueueUsage() ));
		
		auto*	result	= _taskGraph.Add( *this, task );
		auto*	geom	= ToLocal( task.rtGeometry );

		CHECK_ERR( result and geom );
		CHECK_ERR( task.triangles.size() <= geom->GetTriangles().size() );
		CHECK_ERR( task.aabbs.size() <= geom->GetAABBs().size() );

		result->_rtGeometry = geom;

		VkMemoryRequirements2								mem_req	= {};
		VkAccelerationStructureMemoryRequirementsInfoNV		as_info	= {};
		as_info.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		as_info.type					= VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
		as_info.accelerationStructure	= geom->Handle();
		GetDevice().vkGetAccelerationStructureMemoryRequirementsNV( GetDevice().GetVkDevice(), &as_info, OUT &mem_req );

		// TODO: virtual buffer or buffer cache
		BufferID	buf = _instance.CreateBuffer( BufferDesc{ BytesU(mem_req.memoryRequirements.size), EBufferUsage::RayTracing }, Default, "ScratchBuffer" );
		CHECK_ERR( buf );

		result->_scratchBuffer = ToLocal( buf.Get() );
		ReleaseResource( buf.Release() );
		
		ASSERT( EnumEq( result->_scratchBuffer->Description().usage, EBufferUsage::RayTracing ));
		
		result->_geometryCount	= task.triangles.size() + task.aabbs.size();
		result->_geometry		= _mainAllocator.Alloc<VkGeometryNV>( result->_geometryCount );

		// initialize geometry
		for (uint i = 0; i < result->_geometryCount; ++i)
		{
			auto&	dst = result->_geometry[i];
			dst							= {};
			dst.sType					= VK_STRUCTURE_TYPE_GEOMETRY_NV;
			dst.geometry.aabbs.sType	= VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
			dst.geometry.triangles.sType= VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
			dst.geometryType			= (i < task.triangles.size() ? VK_GEOMETRY_TYPE_TRIANGLES_NV : VK_GEOMETRY_TYPE_AABBS_NV);
		}

		// add triangles
		for (size_t i = 0; i < task.triangles.size(); ++i)
		{
			auto&	src		= task.triangles[i];
			size_t	pos		= BinarySearch( geom->GetTriangles(), src.geometryId );
			CHECK_ERR( pos < geom->GetTriangles().size() );

			auto&	dst		= result->_geometry[pos];
			auto&	ref		= geom->GetTriangles()[pos];
			dst.flags = VEnumCast( ref.flags );

			ASSERT( src.vertexBuffer or src.vertexData.size() );
			ASSERT( src.vertexCount > 0 );
			ASSERT( src.vertexCount <= ref.maxVertexCount );
			ASSERT( src.indexCount <= ref.maxIndexCount );
			ASSERT( src.vertexFormat == ref.vertexFormat );
			ASSERT( src.indexType == ref.indexType );

			// vertices
			dst.geometry.triangles.vertexCount	= src.vertexCount;
			dst.geometry.triangles.vertexStride	= VkDeviceSize(src.vertexStride);
			dst.geometry.triangles.vertexFormat	= VEnumCast( src.vertexFormat );

			if ( src.vertexData.size() )
			{
				VLocalBuffer const*	vb = null;
				CHECK_ERR( _StoreData( src.vertexData.data(), ArraySizeOf(src.vertexData), BytesU{ref.vertexSize}, OUT vb, OUT dst.geometry.triangles.vertexOffset ));
				dst.geometry.triangles.vertexData = vb->Handle();
				//result->_usableBuffers.insert( vb );	// staging buffer is already immutable
			}
			else
			{
				auto*	vb = ToLocal( src.vertexBuffer );
				CHECK_ERR( vb );
				dst.geometry.triangles.vertexData	= vb->Handle();
				dst.geometry.triangles.vertexOffset	= VkDeviceSize(src.vertexOffset);
				result->_usableBuffers.insert( vb );
			}
			
			// indices
			if ( src.indexCount > 0 )
			{
				ASSERT( src.indexBuffer or src.indexData.size() );
				dst.geometry.triangles.indexCount	= src.indexCount;
				dst.geometry.triangles.indexType	= VEnumCast( src.indexType );
			}
			else
			{
				ASSERT( not src.indexBuffer or src.indexData.empty() );
				ASSERT( src.indexType == EIndex::Unknown );
				dst.geometry.triangles.indexType	= VK_INDEX_TYPE_NONE_NV;
			}

			if ( src.indexData.size() )
			{
				VLocalBuffer const*	ib = null;
				CHECK_ERR( _StoreData( src.indexData.data(), ArraySizeOf(src.indexData), BytesU{ref.indexSize}, OUT ib, OUT dst.geometry.triangles.indexOffset ));
				dst.geometry.triangles.indexData = ib->Handle();
				//result->_usableBuffers.insert( ib );	// staging buffer is already immutable
			}
			else
			if ( src.indexBuffer )
			{
				auto*	ib = ToLocal( src.indexBuffer );
				CHECK_ERR( ib );
				dst.geometry.triangles.indexData	= ib->Handle();
				dst.geometry.triangles.indexOffset	= VkDeviceSize(src.indexOffset);
				result->_usableBuffers.insert( ib );
			}

			// transformation
			if ( src.transformBuffer )
			{
				auto*	tb = ToLocal( src.transformBuffer );
				CHECK_ERR( tb );
				dst.geometry.triangles.transformData	= tb->Handle();
				dst.geometry.triangles.transformOffset	= VkDeviceSize(src.transformOffset);
				result->_usableBuffers.insert( tb );
			}
			else
			if ( src.transformData.has_value() )
			{
				VLocalBuffer const*	tb = null;
				CHECK_ERR( _StoreData( &src.transformData.value(), BytesU::SizeOf(src.transformData.value()), 16_b, OUT tb, OUT dst.geometry.triangles.transformOffset ));
				dst.geometry.triangles.transformData = tb->Handle();
				//result->_usableBuffers.insert( tb );	// staging buffer is already immutable
			}
		}

		// add AABBs
		for (size_t i = 0; i < task.aabbs.size(); ++i)
		{
			auto&	src		= task.aabbs[i];
			size_t	pos		= BinarySearch( geom->GetAABBs(), src.geometryId );
			CHECK_ERR( pos < geom->GetAABBs().size() );

			auto&	dst		= result->_geometry[pos + task.triangles.size()];
			auto&	ref		= geom->GetAABBs()[pos];
			
			ASSERT( src.aabbBuffer or src.aabbData.size() );
			ASSERT( src.aabbCount > 0 );
			ASSERT( src.aabbCount <= ref.maxAabbCount );
			ASSERT( src.aabbStride % 8 == 0 );

			dst.flags					= VEnumCast( ref.flags );
			dst.geometry.aabbs.sType	= VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
			dst.geometry.aabbs.numAABBs	= src.aabbCount;
			dst.geometry.aabbs.stride	= CheckCast<uint>(src.aabbStride);

			if ( src.aabbData.size() )
			{
				VLocalBuffer const*	ab = null;
				CHECK_ERR( _StoreData( src.aabbData.data(), ArraySizeOf(src.aabbData), 8_b, OUT ab, OUT dst.geometry.aabbs.offset ));
				dst.geometry.aabbs.aabbData = ab->Handle();
				//result->_usableBuffers.insert( ab );	// staging buffer is already immutable
			}
			else
			{
				auto*	ab = ToLocal( src.aabbBuffer );
				CHECK_ERR( ab );
				dst.geometry.aabbs.aabbData	= ab->Handle();
				dst.geometry.aabbs.offset	= VkDeviceSize(src.aabbOffset);
				result->_usableBuffers.insert( ab );
			}
		}

		return result;
	}

/*
=================================================
	AddTask (BuildRayTracingScene)
=================================================
*/
	Task  VCommandBuffer::AddTask (const BuildRayTracingScene &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( RayTracingBit, _GetQueueUsage() ));
		
		auto*	result	= _taskGraph.Add( *this, task );
		auto*	scene	= ToLocal( task.rtScene );

		CHECK_ERR( result and scene );
		CHECK_ERR( task.instances.size() <= scene->MaxInstanceCount() );

		result->_rtScene = scene;

		VkMemoryRequirements2								mem_req	= {};
		VkAccelerationStructureMemoryRequirementsInfoNV		as_info	= {};
		as_info.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		as_info.type					= VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
		as_info.accelerationStructure	= scene->Handle();
		GetDevice().vkGetAccelerationStructureMemoryRequirementsNV( GetDevice().GetVkDevice(), &as_info, OUT &mem_req );
		
		MemoryDesc	mem;
		mem.type	= EMemoryType::Default;
		mem.req		= VulkanMemRequirements{ mem_req.memoryRequirements.memoryTypeBits, CheckCast<uint>(mem_req.memoryRequirements.alignment) };

		// TODO: virtual buffer or buffer cache
		BufferID	scratch_buf = _instance.CreateBuffer( BufferDesc{ BytesU(mem_req.memoryRequirements.size), EBufferUsage::RayTracing }, mem, "ScratchBuffer" );
		CHECK_ERR( scratch_buf );

		result->_scratchBuffer = ToLocal( scratch_buf.Get() );
		ReleaseResource( scratch_buf.Release() );
		
		// TODO: virtual buffer or buffer cache
		BufferID	instance_buf = _instance.CreateBuffer( BufferDesc{ ArraySizeOf(task.instances), EBufferUsage::TransferDst | EBufferUsage::RayTracing }, mem, "InstanceBuffer" );
		CHECK_ERR( instance_buf );

		result->_instanceBuffer = ToLocal( instance_buf.Get() );
		ReleaseResource( instance_buf.Release() );

		VkGeometryInstance*  vk_instances;
		CHECK_ERR( _AllocStorage<VkGeometryInstance>( task.instances.size(), OUT result->_instanceStagingBuffer, OUT result->_instanceStagingBufferOffset, OUT vk_instances ));
		
		ASSERT( EnumEq( result->_scratchBuffer->Description().usage, EBufferUsage::RayTracing ));
		ASSERT( EnumEq( result->_instanceBuffer->Description().usage, EBufferUsage::RayTracing ));

		// sort instances by ID
		Array<uint>	sorted;		// TODO: use temporary allocator
		sorted.resize( task.instances.size() );
		for (size_t i = 0; i < sorted.size(); ++i) { sorted[i] = uint(i); }
		std::sort( sorted.begin(), sorted.end(), [inst = &task.instances] (auto lhs, auto rhs) { return (*inst)[lhs].instanceId < (*inst)[rhs].instanceId; });

		result->_rtGeometries			= _mainAllocator.Alloc< VLocalRTGeometry const *>( task.instances.size() );
		result->_instances				= _mainAllocator.Alloc< VFgTask<BuildRayTracingScene>::Instance >( task.instances.size() );
		result->_instanceCount			= CheckCast<uint>(task.instances.size());
		result->_hitShadersPerInstance	= Max( 1u, task.hitShadersPerInstance );

		for (size_t i = 0; i < task.instances.size(); ++i)
		{
			uint	idx  = sorted[i];
			auto&	src  = task.instances[i];
			auto&	dst  = vk_instances[idx];
			auto&	blas = result->_rtGeometries[idx];

			ASSERT( src.instanceId.IsDefined() );
			ASSERT( src.geometryId );
			ASSERT( (src.customId >> 24) == 0 );

			blas = ToLocal( src.geometryId );
			CHECK_ERR( blas );

			dst.blasHandle		= blas->BLASHandle();
			dst.transformRow0	= src.transform[0];
			dst.transformRow1	= src.transform[1];
			dst.transformRow2	= src.transform[2];
			dst.customIndex		= src.customId;
			dst.mask			= src.mask;
			dst.instanceOffset	= result->_maxHitShaderCount;
			dst.flags			= VEnumCast( src.flags );
			
			PlacementNew<VFgTask<BuildRayTracingScene>::Instance>( result->_instances + idx, src.instanceId, src.geometryId, uint(dst.instanceOffset) );

			result->_maxHitShaderCount += (blas->MaxGeometryCount() * result->_hitShadersPerInstance);
		}
		
		GetResourceManager().CheckTask( task );

		return result;
	}
	
/*
=================================================
	AddTask (TraceRays)
=================================================
*/
	Task  VCommandBuffer::AddTask (const TraceRays &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( RayTracingBit, _GetQueueUsage() ));
		ASSERT( task.shaderTable );

		auto	result = _taskGraph.Add( *this, task );

		if ( EnumAny( _shaderDbg.timemapStages, EShaderStages::AllRayTracing ) and _shaderDbg.timemapIndex != Default )
		{
			ASSERT( result->debugModeIndex == Default );
			result->debugModeIndex = _shaderDbg.timemapIndex;
		}

		return result;
	}
	
/*
=================================================
	AddTask (CustomTask)
=================================================
*/
	Task  VCommandBuffer::AddTask (const CustomTask &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( task.callback );

		return _taskGraph.Add( *this, task );
	}

/*
=================================================
	AddTask (DrawVertices)
=================================================
*/
	void  VCommandBuffer::AddTask (LogicalPassID renderPass, const DrawVertices &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording(), void());
		ASSERT( task.commands.size() );
		ASSERT( task.pipeline );
		
		auto *	rp  = ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<DrawVertices> >( 
						*this, task,
						VTaskProcessor::Visit1_DrawVertices,
						VTaskProcessor::Visit2_DrawVertices );
	}
	
/*
=================================================
	AddTask (DrawIndexed)
=================================================
*/
	void  VCommandBuffer::AddTask (LogicalPassID renderPass, const DrawIndexed &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording(), void());
		ASSERT( task.commands.size() );
		ASSERT( task.pipeline );
		
		auto *	rp  = ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<DrawIndexed> >(
						*this, task,
						VTaskProcessor::Visit1_DrawIndexed,
						VTaskProcessor::Visit2_DrawIndexed );
	}
	
/*
=================================================
	AddTask (DrawMeshes)
=================================================
*/
	void  VCommandBuffer::AddTask (LogicalPassID renderPass, const DrawMeshes &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording(), void());
		ASSERT( task.commands.size() );
		ASSERT( task.pipeline );
		
		auto *	rp  = ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<DrawMeshes> >(
						*this, task,
						VTaskProcessor::Visit1_DrawMeshes,
						VTaskProcessor::Visit2_DrawMeshes );
	}
	
/*
=================================================
	AddTask (DrawVerticesIndirect)
=================================================
*/
	void  VCommandBuffer::AddTask (LogicalPassID renderPass, const DrawVerticesIndirect &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording(), void());
		ASSERT( task.commands.size() );
		ASSERT( task.pipeline );
		
		auto *	rp  = ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<DrawVerticesIndirect> >(
						*this, task,
						VTaskProcessor::Visit1_DrawVerticesIndirect,
						VTaskProcessor::Visit2_DrawVerticesIndirect );
	}
	
/*
=================================================
	AddTask (DrawIndexedIndirect)
=================================================
*/
	void  VCommandBuffer::AddTask (LogicalPassID renderPass, const DrawIndexedIndirect &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording(), void());
		ASSERT( task.commands.size() );
		ASSERT( task.pipeline );
		
		auto *	rp  = ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<DrawIndexedIndirect> >(
						*this, task,
						VTaskProcessor::Visit1_DrawIndexedIndirect,
						VTaskProcessor::Visit2_DrawIndexedIndirect );
	}
	
/*
=================================================
	AddTask (DrawMeshesIndirect)
=================================================
*/
	void  VCommandBuffer::AddTask (LogicalPassID renderPass, const DrawMeshesIndirect &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording(), void());
		ASSERT( task.commands.size() );
		ASSERT( task.pipeline );
		
		auto *	rp  = ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<DrawMeshesIndirect> >(
						*this, task,
						VTaskProcessor::Visit1_DrawMeshesIndirect,
						VTaskProcessor::Visit2_DrawMeshesIndirect );
	}
	
/*
=================================================
	AddTask (CustomDraw)
=================================================
*/
	void  VCommandBuffer::AddTask (LogicalPassID renderPass, const CustomDraw &task)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording(), void());
		ASSERT( task.callback );
		
		auto *	rp  = ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<CustomDraw> >(
						*this, task,
						VTaskProcessor::Visit1_CustomDraw,
						VTaskProcessor::Visit2_CustomDraw );
	}
	
/*
=================================================
	Replace
----
	destroy previous resource instance and construct new instance
=================================================
*/
	template <typename ResType, typename ...Args>
	inline void  Replace (INOUT ResourceBase<ResType> &target, Args&& ...args)
	{
		target.Data().~ResType();
		new (&target.Data()) ResType{ std::forward<Args &&>(args)... };
	}

/*
=================================================
	CreateRenderPass
=================================================
*/
	LogicalPassID  VCommandBuffer::CreateRenderPass (const RenderPassDesc &desc)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( GraphicsBit, _GetQueueUsage() ));

		Index_t		index = 0;
		CHECK_ERR( _rm.logicalRenderPasses.Assign( OUT index ));
		
		auto&		data = _rm.logicalRenderPasses[ index ];
		Replace( data );

		if ( not data.Create( *this, desc ) )
		{
			_rm.logicalRenderPasses.Unassign( index );
			RETURN_ERR( "failed when creating logical render pass" );
		}

		_rm.logicalRenderPassCount = Max( uint(index)+1, _rm.logicalRenderPassCount );

		return LogicalPassID( index, 0 );
	}
	
/*
=================================================
	BeginShaderTimeMap
=================================================
*/
	bool  VCommandBuffer::BeginShaderTimeMap (const uint2 &dim, EShaderStages stages)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		CHECK_ERR( _shaderDbg.timemapIndex == Default );	// already started
		ASSERT( EnumEq( ComputeBit, _GetQueueUsage() ));

		_shaderDbg.timemapStages	= stages;
		_shaderDbg.timemapIndex		= _batch->AppendTimemap( dim, stages );
		CHECK_ERR( _shaderDbg.timemapIndex != Default );

		return true;
	}
	
/*
=================================================
	EndShaderTimeMap
=================================================
*/
	Task  VCommandBuffer::EndShaderTimeMap (RawImageID dstImage, ImageLayer layer, MipmapLevel level, ArrayView<Task> dependsOn)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _IsRecording() );
		CHECK_ERR( _shaderDbg.timemapIndex != Default );	// not started
		ASSERT( EnumEq( ComputeBit, _GetQueueUsage() ));

		auto&	rm		= GetResourceManager();
		auto	desc	= rm.GetDescription( dstImage );
		auto	pplns	= rm.GetShaderTimemapPipelines();

		CHECK_ERR( desc.imageType == EImage::Tex2D or desc.imageType == EImage::Tex2DArray );
		//CHECK_ERR( desc.format == EPixelFormat::RGBA8_UNorm );
		CHECK_ERR( EnumEq( desc.usage, EImageUsage::Storage ));
		CHECK_ERR( std::get<0>(pplns) and std::get<1>(pplns) and std::get<2>(pplns) );

		PipelineResources	res;
		RawBufferID			ssb;
		BytesU				ssb_offset, ssb_size, ssb_offset2, ssb_size2;
		BytesU				ssb_align { _instance.GetDevice().GetDeviceProperties().limits.minStorageBufferOffsetAlignment };
		uint2				ssb_dim;
		Task				task;

		CHECK_ERR( _batch->GetShaderTimemap( _shaderDbg.timemapIndex, OUT ssb, OUT ssb_offset, OUT ssb_size, OUT ssb_dim ));
		_shaderDbg.timemapIndex = Default;

		ssb_size2	= ssb_dim.y * SizeOf<uint64_t>;
		ssb_size	-= ssb_size2 + ssb_align;
		ssb_offset2	= AlignToLarger( ssb_offset + ssb_size, ssb_align );

		// pass 1
		{
			RawCPipelineID	ppln = std::get<0>(pplns);
			CHECK_ERR( GetInstance().InitPipelineResources( ppln, DescriptorSetID{"0"}, OUT res ));

			res.BindBuffer( UniformID{"un_Timemap"},   ssb, ssb_offset, ssb_size );
			res.BindBuffer( UniformID{"un_MaxValues"}, ssb, ssb_offset2, ssb_size2 );

			DispatchCompute		comp;
			comp.SetPipeline( ppln );
			comp.AddResources( DescriptorSetID{"0"}, &res );
			comp.SetLocalSize({ 32, 1, 1 });
			comp.Dispatch({ (ssb_dim.y + 31) / 32, 1, 1 });
			comp.depends = dependsOn;

			task = AddTask( comp );
		}

		// pass 2
		{
			RawCPipelineID	ppln = std::get<1>(pplns);
			CHECK_ERR( GetInstance().InitPipelineResources( ppln, DescriptorSetID{"0"}, OUT res ));
			
			res.BindBuffer( UniformID{"un_Timemap"},   ssb, ssb_offset, ssb_size );
			res.BindBuffer( UniformID{"un_MaxValues"}, ssb, ssb_offset2, ssb_size2 );

			DispatchCompute		comp;
			comp.SetPipeline( ppln );
			comp.AddResources( DescriptorSetID{"0"}, &res );
			comp.SetLocalSize({ 1, 1, 1 });
			comp.Dispatch({ 1, 1, 1 });
			comp.DependsOn( task );

			task = AddTask( comp );
		}

		// pass 3
		{
			RawCPipelineID	ppln = std::get<2>(pplns);
			CHECK_ERR( GetInstance().InitPipelineResources( ppln, DescriptorSetID{"0"}, OUT res ));
			
			res.BindBuffer( UniformID{"un_Timemap"}, ssb, ssb_offset, ssb_size );
			res.BindImage( UniformID{"un_OutImage"}, dstImage, ImageViewDesc{}.SetViewType( EImage::Tex2D ).SetBaseLayer( layer.Get() ).SetBaseLevel( level.Get() ));

			DispatchCompute		comp;
			comp.SetPipeline( ppln );
			comp.AddResources( DescriptorSetID{"0"}, &res );
			comp.SetLocalSize({ 8, 8, 1 });
			comp.Dispatch( (desc.dimension.xy() + 7) / 8 );
			comp.DependsOn( task );

			task = AddTask( comp );
		}
		return task;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	_FlushLocalResourceStates
=================================================
*/
	void  VCommandBuffer::_FlushLocalResourceStates (ExeOrderIndex index, VBarrierManager &barrierMngr, Ptr<VLocalDebugger> debugger)
	{
		// reset state & destroy local images
		for (uint i = 0; i < _rm.images.maxLocalIndex; ++i)
		{
			auto&	image = _rm.images.pool[ Index_t(i) ];

			if ( not image.IsDestroyed() )
			{
				image.Data().ResetState( index, barrierMngr, debugger );
				image.Destroy();
				_rm.images.pool.Unassign( Index_t(i) );
			}
		}
		_rm.images.maxLocalIndex = 0;
		
		// reset state & destroy local buffers
		for (uint i = 0; i < _rm.buffers.maxLocalIndex; ++i)
		{
			auto&	buffer = _rm.buffers.pool[ Index_t(i) ];

			if ( not buffer.IsDestroyed() )
			{
				buffer.Data().ResetState( index, barrierMngr, debugger );
				buffer.Destroy();
				_rm.buffers.pool.Unassign( Index_t(i) );
			}
		}
		_rm.buffers.maxLocalIndex = 0;
	
		// reset state & destroy local ray tracing geometries
		for (uint i = 0; i < _rm.rtGeometries.maxLocalIndex; ++i)
		{
			auto&	geometry = _rm.rtGeometries.pool[ Index_t(i) ];

			if ( not geometry.IsDestroyed() )
			{
				geometry.Data().ResetState( index, barrierMngr, debugger );
				geometry.Destroy();
				_rm.rtGeometries.pool.Unassign( Index_t(i) );
			}
		}
		_rm.rtGeometries.maxLocalIndex = 0;

		// merge & destroy ray tracing scenes
		for (uint i = 0; i < _rm.rtScenes.maxLocalIndex; ++i)
		{
			auto&	scene = _rm.rtScenes.pool[ Index_t(i) ];

			if ( not scene.IsDestroyed() )
			{
				scene.Data().ResetState( index, barrierMngr, debugger );
				scene.Destroy();
				_rm.rtScenes.pool.Unassign( Index_t(i) );
			}
		}
		_rm.rtScenes.maxLocalIndex = 0;
		
		_ResetLocalRemaping();
	}

/*
=================================================
	_ToLocal
=================================================
*/
	template <typename ID, typename Res, typename MainPool, size_t MC>
	inline Res const*  VCommandBuffer::_ToLocal (ID id, INOUT LocalResPool<Res,MainPool,MC> &localRes, StringView msg)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _state == EState::Recording or _state == EState::Compiling );

		if ( id.Index() >= localRes.toLocal.size() )
			return null;

		Index_t&	local = localRes.toLocal[ id.Index() ];

		if ( local != UMax )
		{
			Res const*  result = &(localRes.pool[ local ].Data());
			ASSERT( result->ToGlobal() );
			return result;
		}

		auto*	res  = AcquireTemporary( id );
		if ( not res )
			return null;

		CHECK_ERR( localRes.pool.Assign( OUT local ));

		auto&	data = localRes.pool[ local ];
		Replace( data );
		
		if ( not data.Create( res ) )
		{
			localRes.pool.Unassign( local );
			RETURN_ERR( msg );
		}

		localRes.maxLocalIndex  = Max( uint(local)+1, localRes.maxLocalIndex );
		localRes.maxGlobalIndex = Max( uint(id.Index())+1, localRes.maxGlobalIndex );

		return &(data.Data());
	}
	
/*
=================================================
	ToLocal
=================================================
*/
	VLocalBuffer const*  VCommandBuffer::ToLocal (RawBufferID id)
	{
		return _ToLocal( id, _rm.buffers, "failed when creating local buffer" );
	}

	VLocalImage const*  VCommandBuffer::ToLocal (RawImageID id)
	{
		return _ToLocal( id, _rm.images, "failed when creating local image" );
	}

	VLocalRTGeometry const*  VCommandBuffer::ToLocal (RawRTGeometryID id)
	{
		return _ToLocal( id, _rm.rtGeometries, "failed when creating local ray tracing geometry" );
	}

	VLocalRTScene const*  VCommandBuffer::ToLocal (RawRTSceneID id)
	{
		return _ToLocal( id, _rm.rtScenes, "failed when creating local ray tracing scene" );
	}
	
	VLogicalRenderPass*  VCommandBuffer::ToLocal (LogicalPassID id)
	{
		ASSERT( id );
		EXLOCK( _drCheck );

		auto&	data = _rm.logicalRenderPasses[ id.Index() ];
		ASSERT( data.IsCreated() );

		return &data.Data();
	}

/*
=================================================
	_ResetLocalRemaping
=================================================
*/
	void  VCommandBuffer::_ResetLocalRemaping ()
	{
		memset( _rm.images.toLocal.data(), ~0u, sizeof(Index_t)*_rm.images.maxGlobalIndex );
		memset( _rm.buffers.toLocal.data(), ~0u, sizeof(Index_t)*_rm.buffers.maxGlobalIndex );
		memset( _rm.rtScenes.toLocal.data(), ~0u, sizeof(Index_t)*_rm.rtScenes.maxGlobalIndex );
		memset( _rm.rtGeometries.toLocal.data(), ~0u, sizeof(Index_t)*_rm.rtGeometries.maxGlobalIndex );

		_rm.images.maxGlobalIndex		= 0;
		_rm.buffers.maxGlobalIndex		= 0;
		_rm.rtScenes.maxGlobalIndex		= 0;
		_rm.rtGeometries.maxGlobalIndex	= 0;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	_StorePartialData
=================================================
*/
	bool  VCommandBuffer::_StorePartialData (ArrayView<uint8_t> srcData, const BytesU srcOffset, OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size)
	{
		// skip blocks less than 1/N of data size
		const BytesU	src_size	= ArraySizeOf(srcData);
		const BytesU	min_size	= Min( (src_size + MaxBufferParts-1) / MaxBufferParts, Min( src_size, MinBufferPart ));
		void *			ptr			= null;

		if ( _batch->GetWritable( src_size - srcOffset, 1_b, 16_b, min_size, OUT dstBuffer, OUT dstOffset, OUT size, OUT ptr ))
		{
			MemCopy( ptr, size, srcData.data() + srcOffset, size );
			return true;
		}
		return false;
	}

/*
=================================================
	_StoreImageData
=================================================
*/
	bool  VCommandBuffer::_StoreImageData (ArrayView<uint8_t> srcData, const BytesU srcOffset, const BytesU srcPitch, const BytesU srcTotalSize,
										   OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size)
	{
		// skip blocks less than 1/N of total data size
		const BytesU	src_size	= ArraySizeOf(srcData);
		const BytesU	min_size	= Max( (srcTotalSize + MaxImageParts-1) / MaxImageParts, srcPitch );
		void *			ptr			= null;

		if ( _batch->GetWritable( src_size - srcOffset, srcPitch, 16_b, min_size, OUT dstBuffer, OUT dstOffset, OUT size, OUT ptr ))
		{
			MemCopy( ptr, size, srcData.data() + srcOffset, size );
			return true;
		}
		return false;
	}
	

}	// FG

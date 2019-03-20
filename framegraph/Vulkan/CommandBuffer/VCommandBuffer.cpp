// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VCommandBuffer.h"
#include "VTaskGraph.hpp"

namespace FG
{
namespace {
	static constexpr auto	GraphicsBit		= EQueueUsageBits::Graphics;
	static constexpr auto	ComputeBit		= EQueueUsageBits::Graphics | EQueueUsageBits::AsyncCompute;
	static constexpr auto	RayTracingBit	= EQueueUsageBits::Graphics | EQueueUsageBits::AsyncCompute;
	static constexpr auto	TransferBit		= EQueueUsageBits::Graphics | EQueueUsageBits::AsyncCompute | EQueueUsageBits::AsyncTransfer;
}
	
/*
=================================================
	constructor
=================================================
*/
	VCommandBuffer::VCommandBuffer (VFrameGraph &fg) :
		_instance{ fg },
		_shaderDebugger{ *this }
	{
		_mainAllocator.SetBlockSize( 16_Mb );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VCommandBuffer::~VCommandBuffer ()
	{
		CHECK( not IsRecording() );
		CHECK( _perQueue.empty() );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool VCommandBuffer::Initialize ()
	{
		EXLOCK( _rcCheck );

		// TODO
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VCommandBuffer::Deinitialize ()
	{
		EXLOCK( _rcCheck );
		CHECK( not IsRecording() );

		auto&	dev = GetDevice();

		for (auto& q : _perQueue)
		{
			if ( q.cmdPool ) {
				dev.vkDestroyCommandPool( dev.GetVkDevice(), q.cmdPool, null );
			}
		}
		_perQueue.clear();
	}

/*
=================================================
	Begin
=================================================
*/
	bool VCommandBuffer::Begin (const CommandBufferDesc &desc, const VCmdBatchPtr &batch)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( batch );

		_currUsage	= EQueueUsageBits(0) | desc.queue;
		_batch		= batch;
		
		// create command pool
		{
			uint	index = uint(_batch->GetQueue()->familyIndex);

			_perQueue.resize( Max( _perQueue.size(), index+1 ));

			auto&	q	= _perQueue[index];
			auto&	dev	= GetDevice();

			if ( not q.cmdPool )
			{
				VkCommandPoolCreateInfo	info = {};
				info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				info.flags				= 0; //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				info.queueFamilyIndex	= index;

				VK_CHECK( dev.vkCreateCommandPool( dev.GetVkDevice(), &info, null, OUT &q.cmdPool ));
			}
		}
		
		_hostWritableBufferSize		= desc.hostWritableBufferPageSize;
		_hostReadableBufferSize		= desc.hostWritableBufferPageSize;
		_hostWritebleBufferUsage	= desc.hostWritebleBufferUsage | EBufferUsage::TransferSrc;
		
		_taskGraph.OnStart( GetAllocator() );
		return true;
	}
	
/*
=================================================
	Submit
=================================================
*/
	bool VCommandBuffer::Submit ()
	{
		EXLOCK( _rcCheck );

		CHECK_ERR( _BuildCommandBuffers() );
		
		CHECK_ERR( _batch->OnBacked( _rm.resourceMap ));
		_batch = null;
		
		_taskGraph.OnDiscardMemory();
		return true;
	}
	
/*
=================================================
	SignalSemaphore
=================================================
*/
	void VCommandBuffer::SignalSemaphore (VkSemaphore sem)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording(), void());

		_batch->SignalSemaphore( sem );
	}
	
/*
=================================================
	WaitSemaphore
=================================================
*/
	void VCommandBuffer::WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording(), void());

		_batch->WaitSemaphore( sem, stage );
	}

/*
=================================================
	_BuildCommandBuffers
=================================================
*/
	bool VCommandBuffer::_BuildCommandBuffers ()
	{
		//if ( _taskGraph.Empty() )
		//	return true;

		VkCommandBuffer		cmd;
		VDevice const&		dev	= GetDevice();
		
		// create command buffer
		{
			auto&	q = _perQueue[ uint(_batch->GetQueue()->familyIndex) ];
			
			VkCommandBufferAllocateInfo	info = {};
			info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.pNext				= null;
			info.commandPool		= q.cmdPool;
			info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			info.commandBufferCount	= 1;
			
			VK_CHECK( dev.vkAllocateCommandBuffers( dev.GetVkDevice(), &info, OUT &cmd ));
			_batch->AddCommandBuffer( cmd );
		}

		// begin
		{
			VkCommandBufferBeginInfo	info = {};
			info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CALL( dev.vkBeginCommandBuffer( cmd, &info ));
			//dev.vkCmdWriteTimestamp( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, _query.pool, _currQueue->frames[_frameId].queryIndex );
		}

		_shaderDebugger.OnBeginRecording( cmd );

		// commit image layout transition and other
		_barrierMngr.Commit( dev, cmd );

		CHECK( _ProcessTasks( cmd ));

		// transit image layout to default state
		// add memory dependency to flush caches
		/*{
			auto	batch_exe_order = _submissionGraph->GetExecutionOrder( _cmdBatchId, _indexInBatch );

			_resourceMngr.FlushLocalResourceStates( ExeOrderIndex::Final, batch_exe_order, _barrierMngr, GetDebugger() );
			_barrierMngr.Commit( dev, cmd );
		}*/

		_shaderDebugger.OnEndRecording( cmd );
		
		// end
		{
			//dev.vkCmdWriteTimestamp( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, _query.pool, _currQueue->frames[_frameId].queryIndex + 1 );
			VK_CALL( dev.vkEndCommandBuffer( cmd ));
		}
		return true;
	}
	
/*
=================================================
	VTaskProcessor::Run
=================================================
*/
	forceinline void VTaskProcessor::Run (VTask node)
	{
		// reset states
		_currTask	= node;
		
		//if ( _frameGraph.GetDebugger() )
		//	_frameGraph.GetDebugger()->AddTask( _currTask );

		Cast<VFrameGraphTask>( node )->Process( this );
	}

/*
=================================================
	_ProcessTasks
=================================================
*/
	bool VCommandBuffer::_ProcessTasks (VkCommandBuffer cmd)
	{
		VTaskProcessor	processor{ *this, cmd };

		uint			visitor_id		= 0;
		ExeOrderIndex	exe_order_index	= ExeOrderIndex::First;

		TempTaskArray_t		pending{ GetAllocator() };
		pending.reserve( 128 );
		pending.assign( _taskGraph.Entries().begin(), _taskGraph.Entries().end() );

		for (uint k = 0; k < 10 and not pending.empty(); ++k)
		{
			for (auto iter = pending.begin(); iter != pending.end();)
			{
				auto	node = Cast<VFrameGraphTask>( *iter );
				
				if ( node->VisitorID() == visitor_id ) {
					++iter;
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
					++iter;
					continue;
				}

				node->SetVisitorID( visitor_id );
				node->SetExecutionOrder( ++exe_order_index );
				
				processor.Run( node );

				for (auto out_node : node->Outputs())
				{
					pending.push_back( out_node );
				}

				iter = pending.erase( iter );
			}
		}
		return true;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	AddExternalCommands
=================================================
*/
	bool  VCommandBuffer::AddExternalCommands (const ExternalCmdBatch_t &desc)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );

		auto*	info = UnionGetIf< VulkanCommandBatch >( &desc );
		CHECK_ERR( info );
		CHECK_ERR( info->queueFamilyIndex == uint(_batch->GetQueue()->familyIndex) );

		for (auto& cmd : info->commands) {
			_batch->AddCommandBuffer( BitCast<VkCommandBuffer>(cmd) );
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
		EXLOCK( _rcCheck );
		CHECK_ERR( cmd.GetBatch() );
		CHECK_ERR( IsRecording() );

		_batch->AddDependency( Cast<VCmdBatch>(cmd.GetBatch()) );
		return true;
	}

/*
=================================================
	AddTask (SubmitRenderPass)
=================================================
*/
	Task  VCommandBuffer::AddTask (const SubmitRenderPass &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, GraphicsBit ));

		auto*	rp_task = _taskGraph.Add( *this, task );

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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, ComputeBit ));

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (DispatchComputeIndirect)
=================================================
*/
	Task  VCommandBuffer::AddTask (const DispatchComputeIndirect &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, ComputeBit ));

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (CopyBuffer)
=================================================
*/
	Task  VCommandBuffer::AddTask (const CopyBuffer &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, TransferBit ));
		
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, TransferBit ));
		
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, TransferBit ));
		
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, TransferBit ));
		
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, GraphicsBit ));
		
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, GraphicsBit ));
		
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, GraphicsBit ));

		return _taskGraph.Add( *this, task );
	}

/*
=================================================
	AddTask (FillBuffer)
=================================================
*/
	Task  VCommandBuffer::AddTask (const FillBuffer &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (ClearColorImage)
=================================================
*/
	Task  VCommandBuffer::AddTask (const ClearColorImage &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, ComputeBit ));

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (ClearDepthStencilImage)
=================================================
*/
	Task  VCommandBuffer::AddTask (const ClearDepthStencilImage &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, ComputeBit ));

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (UpdateBuffer)
=================================================
*/
	Task  VCommandBuffer::AddTask (const UpdateBuffer &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, TransferBit ));

		if ( task.regions.empty() )
			return null;	// TODO: is it an error?

		return _AddUpdateBufferTask( task );
		//return _taskGraph.Add( *this, task );
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, TransferBit ));
		
		if ( All( task.imageSize == uint3(0) ) )
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
		ASSERT(Any( task.imageSize > uint3(0) ));

		const uint3		image_size		= Max( task.imageSize, 1u );
		const auto&		fmt_info		= EPixelFormat_GetInfo( img_desc.format );
		const auto&		block_dim		= fmt_info.blockSize;
		const uint		block_size		= task.aspectMask != EImageAspect::Stencil ? fmt_info.bitsPerBlock : fmt_info.bitsPerBlock2;
		const BytesU	row_pitch		= Max( task.dataRowPitch, BytesU(image_size.x * block_size + block_dim.x-1) / (block_dim.x * 8) );
		const BytesU	min_slice_pitch	= (image_size.y * row_pitch + block_dim.y-1) / block_dim.y;
		const BytesU	slice_pitch		= Max( task.dataSlicePitch, min_slice_pitch );
		const BytesU	total_size		= image_size.z > 1 ? slice_pitch * image_size.z : min_slice_pitch;

		CHECK_ERR( total_size == ArraySizeOf(task.data) );

		const BytesU		min_size	= _GetMaxWritableStoregeSize();
		const uint			row_length	= uint((row_pitch * block_dim.x * 8) / block_size);
		const uint			img_height	= uint((slice_pitch * block_dim.y) / row_pitch);
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

				const uint	z_size = uint(size / slice_pitch);

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

				const uint	y_size = uint((size * block_dim.y) / row_pitch);

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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, TransferBit ));
		
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
			CHECK_ERR( _AddPendingLoad( written, task.size, OUT dst_buffer, OUT range ));
			
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

		_AddDataLoadedEvent( std::move(load_event) );
		
		return AddTask( copy );
	}
	
/*
=================================================
	AddTask (ReadImage)
=================================================
*/
	Task  VCommandBuffer::AddTask (const ReadImage &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, TransferBit ));
		
		if ( All( task.imageSize == uint3(0) ) )
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
		ASSERT(Any( task.imageSize > uint3(0) ));
		
		const uint3			image_size		= Max( task.imageSize, 1u );
		const BytesU		min_size		= _GetMaxReadableStorageSize();
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
				CHECK_ERR( _AddPendingLoad( written, total_size, slice_pitch, OUT dst_buffer, OUT range ));
			
				if ( copy.dstBuffer and dst_buffer != copy.dstBuffer )
				{
					Task	last_task = AddTask( copy );
					copy.regions.clear();
					copy.depends.clear();
					copy.depends.push_back( last_task );
				}

				const uint	z_size = uint(range.size / slice_pitch);

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
				CHECK_ERR( _AddPendingLoad( written, total_size, row_pitch * block_dim.y, OUT dst_buffer, OUT range ));
				
				if ( copy.dstBuffer and dst_buffer != copy.dstBuffer )
				{
					Task	last_task = AddTask( copy );
					copy.regions.clear();
					copy.depends.clear();
					copy.depends.push_back( last_task );
				}

				const uint	y_size = uint((range.size * block_dim.y) / row_pitch);

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

		_AddDataLoadedEvent( std::move(load_event) );

		return AddTask( copy );
	}

/*
=================================================
	AddTask (Present)
=================================================
*/
	Task  VCommandBuffer::AddTask (const FG::Present &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );

		//if ( _swapchain )
		//	return _AddPresentTask( task );
		
		RETURN_ERR( "not supported", Task(null) );
	}

/*
=================================================
	_AddPresentTask
=================================================
*
	Task  VCommandBuffer::_AddPresentTask (const FG::Present &task)
	{
		/*CHECK_ERR( task.srcImage );
	
		RawImageID	swapchain_image;
		CHECK_ERR( _swapchain->Acquire( ESwapchainImage::Primary, OUT swapchain_image ));

		auto*	src_data = AcquireTemporary( task.srcImage );
		auto*	dst_data = AcquireTemporary( swapchain_image );

		BlitImage	blit;
		blit.debugColor	= task.debugColor;
		blit.depends	= task.depends;
		blit.taskName	= task.taskName;

		blit.srcImage	= task.srcImage;
		blit.dstImage	= swapchain_image;
		blit.filter		= EFilter::Nearest;
		blit.AddRegion( {}, int3{}, int3{src_data->Dimension()}, {}, int3{}, int3{dst_data->Dimension()} );

		//CHECK_ERR( _swapchain->Present( swapchain_image ));
		* /
		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	AddTask (PresentVR)
=================================================
*
	Task  VCommandBuffer::AddTask (const PresentVR &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );

		return _taskGraph.Add( *this, task );
	}
	
/*
=================================================
	_AllocStorage
=================================================
*/
	template <typename T>
	inline bool VCommandBuffer::_AllocStorage (size_t count, OUT const VLocalBuffer* &outBuffer, OUT VkDeviceSize &outOffset, OUT T* &outPtr)
	{
		RawBufferID		buffer;
		BytesU			buf_offset, buf_size;
		
		CHECK_ERR( _GetWritable( SizeOf<T> * count, 1_b, 16_b, SizeOf<T> * count, OUT buffer, OUT buf_offset, OUT buf_size, OUT BitCast<void *>(outPtr) ));

		outBuffer	= ToLocal( buffer );
		outOffset	= VkDeviceSize(buf_offset);
		return true;
	}
	
/*
=================================================
	_StoreData
=================================================
*/
	inline bool VCommandBuffer::_StoreData (const void *dataPtr, BytesU dataSize, BytesU offsetAlign, OUT const VLocalBuffer* &outBuffer, OUT VkDeviceSize &outOffset)
	{
		RawBufferID		buffer;
		BytesU			buf_offset, buf_size;
		void *			ptr = null;

		if ( _GetWritable( dataSize, 1_b, offsetAlign, dataSize, OUT buffer, OUT buf_offset, OUT buf_size, OUT ptr ) )
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, RayTracingBit ));

		return _taskGraph.Add( *this, task );
	}

/*
=================================================
	AddTask (BuildRayTracingGeometry)
=================================================
*/
	Task  VCommandBuffer::AddTask (const BuildRayTracingGeometry &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, RayTracingBit ));
		
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
		BufferID	buf = _instance.CreateBuffer( BufferDesc{ BytesU(mem_req.memoryRequirements.size), EBufferUsage::RayTracing }, Default, Default );
		result->_scratchBuffer = ToLocal( buf.Get() );
		ReleaseResource( buf.Release() );
		
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
			dst.geometry.aabbs.stride	= uint(src.aabbStride);

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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, RayTracingBit ));
		
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
		
		// TODO: virtual buffer or buffer cache
		BufferID	buf = _instance.CreateBuffer( BufferDesc{ BytesU(mem_req.memoryRequirements.size), EBufferUsage::RayTracing }, Default, Default );
		result->_scratchBuffer = ToLocal( buf.Get() );
		ReleaseResource( buf.Release() );

		VkGeometryInstance*  vk_instances;
		CHECK_ERR( _AllocStorage<VkGeometryInstance>( task.instances.size(), OUT result->_instanceBuffer, OUT result->_instanceBufferOffset, OUT vk_instances ));

		// sort instances by ID
		Array<uint>	sorted;		// TODO: use temporary allocator
		sorted.resize( task.instances.size() );
		for (size_t i = 0; i < sorted.size(); ++i) { sorted[i] = uint(i); }
		std::sort( sorted.begin(), sorted.end(), [inst = &task.instances] (auto lhs, auto rhs) { return (*inst)[lhs].instanceId < (*inst)[rhs].instanceId; });

		result->_rtGeometries			= _mainAllocator.Alloc< VLocalRTGeometry const *>( task.instances.size() );
		result->_instances				= _mainAllocator.Alloc< VFgTask<BuildRayTracingScene>::Instance >( task.instances.size() );
		result->_instanceCount			= uint(task.instances.size());
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

			blas = ToLocal( src.geometryId, /*acquire ref*/true );
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
		
		return result;
	}
	
/*
=================================================
	AddTask (TraceRays)
=================================================
*/
	Task  VCommandBuffer::AddTask (const TraceRays &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, RayTracingBit ));

		return _taskGraph.Add( *this, task );
	}

/*
=================================================
	AddTask (DrawVertices)
=================================================
*/
	void  VCommandBuffer::AddTask (LogicalPassID renderPass, const DrawVertices &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording(), void());
		
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording(), void());
		
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording(), void());
		
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording(), void());
		
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording(), void());
		
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording(), void());
		
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording(), void());
		
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
	inline void Replace (INOUT ResourceBase<ResType> &target, Args&& ...args)
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
		EXLOCK( _rcCheck );
		CHECK_ERR( IsRecording() );
		ASSERT( EnumAny( _currUsage, GraphicsBit ));

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
//-----------------------------------------------------------------------------

	
/*
=================================================
	_FlushLocalResourceStates
=================================================
*/
	void  VCommandBuffer::_FlushLocalResourceStates (ExeOrderIndex index, ExeOrderIndex batchExeOrder,
													 VBarrierManager &barrierMngr, Ptr<VFrameGraphDebugger> debugger)
	{
		// reset state & destroy local images
		for (uint i = 0; i < _rm.localImagesCount; ++i)
		{
			auto&	image = _rm.localImages[ Index_t(i) ];

			if ( not image.IsDestroyed() )
			{
				image.Data().ResetState( index, barrierMngr, debugger );
				//image.Destroy( OUT GetReadyToDeleteQueue(), OUT GetUnassignIDs() );
				_rm.localImages.Unassign( Index_t(i) );
			}
		}
		
		// reset state & destroy local buffers
		for (uint i = 0; i < _rm.localBuffersCount; ++i)
		{
			auto&	buffer = _rm.localBuffers[ Index_t(i) ];

			if ( not buffer.IsDestroyed() )
			{
				buffer.Data().ResetState( index, barrierMngr, debugger );
				//buffer.Destroy( OUT GetReadyToDeleteQueue(), OUT GetUnassignIDs() );
				_rm.localBuffers.Unassign( Index_t(i) );
			}
		}
	
		// reset state & destroy local ray tracing geometries
		for (uint i = 0; i < _rm.localRTGeometryCount; ++i)
		{
			auto&	geometry = _rm.localRTGeometries[ Index_t(i) ];

			if ( not geometry.IsDestroyed() )
			{
				geometry.Data().ResetState( index, barrierMngr, debugger );
				//geometry.Destroy( OUT GetReadyToDeleteQueue(), OUT GetUnassignIDs() );
				_rm.localRTGeometries.Unassign( Index_t(i) );
			}
		}

		// merge & destroy ray tracing scenes
		for (uint i = 0; i < _rm.localRTSceneCount; ++i)
		{
			auto&	scene = _rm.localRTScenes[ Index_t(i) ];

			if ( not scene.IsDestroyed() )
			{
				// TODO
				/*if ( scene.Data().HasUncommitedChanges() )
				{
					_syncTasks.emplace_back( [this, obj = scene.Data().ToGlobal()] () {
												obj->CommitChanges( this->GetFrameIndex() );
											});
				}*/
				scene.Data().ResetState( index, barrierMngr, debugger );
				//scene.Destroy( OUT GetReadyToDeleteQueue(), OUT GetUnassignIDs(), batchExeOrder, GetFrameIndex() );
				_rm.localRTScenes.Unassign( Index_t(i) );
			}
		}
		
		_ResetLocalRemaping();
	}

/*
=================================================
	_ToLocal
=================================================
*/
	template <typename ID, typename T, typename GtoL>
	inline T const*  VCommandBuffer::_ToLocal (ID id, INOUT PoolTmpl<T> &localPool, INOUT GtoL &globalToLocal, INOUT uint &counter, bool incRef, StringView msg)
	{
		EXLOCK( _rcCheck );

		if ( id.Index() >= globalToLocal.size() )
			return null;

		Index_t&	local = globalToLocal[ id.Index() ];

		if ( local != UMax )
		{
			//if ( incRef )
			//	_mainRM._GetResourcePool( id )[ id.Index() ].AddRef();

			return &(localPool[ local ].Data());
		}

		auto*	res  = AcquireTemporary( id );
		if ( not res )
			return null;

		CHECK_ERR( localPool.Assign( OUT local ));

		auto&	data = localPool[ local ];
		Replace( data );
		
		if ( not data.Create( res ) )
		{
			localPool.Unassign( local );
			RETURN_ERR( msg );
		}

		// TODO
		//if ( incRef )
		//	_mainRM._GetResourcePool( id )[ id.Index() ].AddRef();

		counter = Max( uint(local)+1, counter );
		return &(data.Data());
	}
	
/*
=================================================
	ToLocal
=================================================
*/
	VLocalBuffer const*  VCommandBuffer::ToLocal (RawBufferID id, bool acquireRef)
	{
		return _ToLocal( id, _rm.localBuffers, _rm.bufferToLocal, _rm.localBuffersCount, acquireRef, "failed when creating local buffer" );
	}

	VLocalImage const*  VCommandBuffer::ToLocal (RawImageID id, bool acquireRef)
	{
		return _ToLocal( id, _rm.localImages, _rm.imageToLocal, _rm.localImagesCount, acquireRef, "failed when creating local image" );
	}

	VLocalRTGeometry const*  VCommandBuffer::ToLocal (RawRTGeometryID id, bool acquireRef)
	{
		return _ToLocal( id, _rm.localRTGeometries, _rm.rtGeometryToLocal, _rm.localRTGeometryCount, acquireRef, "failed when creating local ray tracing geometry" );
	}

	VLocalRTScene const*  VCommandBuffer::ToLocal (RawRTSceneID id, bool acquireRef)
	{
		return _ToLocal( id, _rm.localRTScenes, _rm.rtSceneToLocal, _rm.localRTSceneCount, acquireRef, "failed when creating local ray tracing scene" );
	}
	
	VLogicalRenderPass*  VCommandBuffer::ToLocal (LogicalPassID id)
	{
		ASSERT( id );
		EXLOCK( _rcCheck );

		auto&	data = _rm.logicalRenderPasses[ id.Index() ];
		ASSERT( data.IsCreated() );

		return &data.Data();
	}

/*
=================================================
	_ResetLocalRemaping
=================================================
*/
	void VCommandBuffer::_ResetLocalRemaping ()
	{
		memset( _rm.imageToLocal.data(), ~0u, size_t(ArraySizeOf(_rm.imageToLocal)) );
		memset( _rm.bufferToLocal.data(), ~0u, size_t(ArraySizeOf(_rm.bufferToLocal)) );
		memset( _rm.rtSceneToLocal.data(), ~0u, size_t(ArraySizeOf(_rm.rtSceneToLocal)) );
		memset( _rm.rtGeometryToLocal.data(), ~0u, size_t(ArraySizeOf(_rm.rtGeometryToLocal)) );

		_rm.localImagesCount		= 0;
		_rm.localBuffersCount		= 0;
		_rm.localRTGeometryCount	= 0;
		_rm.localRTSceneCount		= 0;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	_StorePartialData
=================================================
*/
	bool VCommandBuffer::_StorePartialData (ArrayView<uint8_t> srcData, const BytesU srcOffset, OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size)
	{
		// skip blocks less than 1/N of data size
		const BytesU	src_size	= ArraySizeOf(srcData);
		const BytesU	min_size	= Min( (src_size + MaxBufferParts-1) / MaxBufferParts, Min( src_size, MinBufferPart ));
		void *			ptr			= null;

		if ( _GetWritable( src_size - srcOffset, 1_b, 16_b, min_size, OUT dstBuffer, OUT dstOffset, OUT size, OUT ptr ) )
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
	bool VCommandBuffer::_StoreImageData (ArrayView<uint8_t> srcData, const BytesU srcOffset, const BytesU srcPitch, const BytesU srcTotalSize,
										  OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size)
	{
		// skip blocks less than 1/N of total data size
		const BytesU	src_size	= ArraySizeOf(srcData);
		const BytesU	min_size	= Max( (srcTotalSize + MaxImageParts-1) / MaxImageParts, srcPitch );
		void *			ptr			= null;

		if ( _GetWritable( src_size - srcOffset, srcPitch, 16_b, min_size, OUT dstBuffer, OUT dstOffset, OUT size, OUT ptr ) )
		{
			MemCopy( ptr, size, srcData.data() + srcOffset, size );
			return true;
		}
		return false;
	}

/*
=================================================
	_GetWritable
=================================================
*/
	bool VCommandBuffer::_GetWritable (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
											  OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &outSize, OUT void* &mappedPtr)
	{
		ASSERT( blockAlign > 0_b and offsetAlign > 0_b );
		ASSERT( dstMinSize == AlignToSmaller( dstMinSize, blockAlign ));

		auto&	staging_buffers = _batch->_hostToDevice;


		// search in existing
		StagingBuffer*	suitable		= null;
		StagingBuffer*	max_available	= null;
		BytesU			max_size;

		for (auto& buf : staging_buffers)
		{
			const BytesU	off	= AlignToLarger( buf.size, offsetAlign );
			const BytesU	av	= AlignToSmaller( buf.capacity - off, blockAlign );

			if ( av >= srcRequiredSize )
			{
				suitable = &buf;
				break;
			}

			if ( not max_available or av > max_size )
			{
				max_available	= &buf;
				max_size		= av;
			}
		}

		// no suitable space, try to use max available block
		if ( not suitable and max_available and max_size >= dstMinSize )
		{
			suitable = max_available;
		}

		// allocate new buffer
		if ( not suitable )
		{
			ASSERT( dstMinSize < _hostWritableBufferSize );

			BufferID	buf_id = _instance.CreateBuffer( BufferDesc{ _hostWritableBufferSize, _hostWritebleBufferUsage }, 
														 MemoryDesc{ EMemoryType::HostWrite }, "HostWriteBuffer" );
			CHECK_ERR( buf_id );

			RawMemoryID	mem_id = GetResourceManager().GetResource( buf_id.Get() )->GetMemoryID();
			CHECK_ERR( mem_id );

			staging_buffers.push_back({ std::move(buf_id), mem_id, _hostWritableBufferSize });

			suitable = &staging_buffers.back();
			CHECK( _batch->_MapMemory( GetResourceManager(), *suitable ));
		}

		// write data to buffer
		dstOffset	= AlignToLarger( suitable->size, offsetAlign );
		outSize		= Min( AlignToSmaller( suitable->capacity - dstOffset, blockAlign ), srcRequiredSize );
		dstBuffer	= suitable->bufferId.Get();
		mappedPtr	= suitable->mappedPtr + dstOffset;

		suitable->size = dstOffset + outSize;
		return true;
	}
	
/*
=================================================
	_AddPendingLoad
=================================================
*/
	bool VCommandBuffer::_AddPendingLoad (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
										  OUT RawBufferID &dstBuffer, OUT VCmdBatch::OnBufferDataLoadedEvent::Range &range)
	{
		ASSERT( blockAlign > 0_b and offsetAlign > 0_b );
		ASSERT( dstMinSize == AlignToSmaller( dstMinSize, blockAlign ));

		auto&	staging_buffers = _batch->_deviceToHost;
		

		// search in existing
		StagingBuffer*	suitable		= null;
		StagingBuffer*	max_available	= null;
		BytesU			max_size;

		for (auto& buf : staging_buffers)
		{
			const BytesU	off	= AlignToLarger( buf.size, offsetAlign );
			const BytesU	av	= AlignToSmaller( buf.capacity - off, blockAlign );

			if ( av >= srcRequiredSize )
			{
				suitable = &buf;
				break;
			}
			
			if ( not max_available or av > max_size )
			{
				max_available	= &buf;
				max_size		= av;
			}
		}

		// no suitable space, try to use max available block
		if ( not suitable and max_available and max_size >= dstMinSize )
		{
			suitable = max_available;
		}

		// allocate new buffer
		if ( not suitable )
		{
			ASSERT( dstMinSize < _hostReadableBufferSize );

			BufferID	buf_id = _instance.CreateBuffer( BufferDesc{ _hostReadableBufferSize, EBufferUsage::TransferDst },
														 MemoryDesc{ EMemoryType::HostRead }, "HostReadBuffer" );
			CHECK_ERR( buf_id );
			
			RawMemoryID	mem_id = GetResourceManager().GetResource( buf_id.Get() )->GetMemoryID();
			CHECK_ERR( mem_id );

			staging_buffers.push_back({ std::move(buf_id), mem_id, _hostReadableBufferSize });

			suitable = &staging_buffers.back();
			CHECK( _batch->_MapMemory( GetResourceManager(), *suitable ));
		}
		
		// write data to buffer
		range.buffer	= suitable;
		range.offset	= AlignToLarger( suitable->size, offsetAlign );
		range.size		= Min( AlignToSmaller( suitable->capacity - range.offset, blockAlign ), srcRequiredSize );
		dstBuffer		= suitable->bufferId.Get();

		suitable->size = range.offset + range.size;
		return true;
	}
	
/*
=================================================
	_AddPendingLoad
=================================================
*/
	bool VCommandBuffer::_AddPendingLoad (const BytesU srcOffset, const BytesU srcTotalSize,
										  OUT RawBufferID &dstBuffer, OUT VCmdBatch::OnBufferDataLoadedEvent::Range &range)
	{
		// skip blocks less than 1/N of data size
		const BytesU	min_size = (srcTotalSize + MaxBufferParts-1) / MaxBufferParts;

		return _AddPendingLoad( srcTotalSize - srcOffset, 1_b, 16_b, min_size, OUT dstBuffer, OUT range );
	}

/*
=================================================
	_AddDataLoadedEvent
=================================================
*/
	bool VCommandBuffer::_AddDataLoadedEvent (VCmdBatch::OnBufferDataLoadedEvent &&ev)
	{
		CHECK_ERR( ev.callback and not ev.parts.empty() );

		_batch->_onBufferLoadedEvents.push_back( std::move(ev) );
		return true;
	}
	
/*
=================================================
	_AddPendingLoad
=================================================
*/
	bool VCommandBuffer::_AddPendingLoad (const BytesU srcOffset, const BytesU srcTotalSize, const BytesU srcPitch,
										  OUT RawBufferID &dstBuffer, OUT VCmdBatch::OnImageDataLoadedEvent::Range &range)
	{
		// skip blocks less than 1/N of total data size
		const BytesU	min_size = Max( (srcTotalSize + MaxImageParts-1) / MaxImageParts, srcPitch );

		return _AddPendingLoad( srcTotalSize - srcOffset, srcPitch, 16_b, min_size, OUT dstBuffer, OUT range );
	}

/*
=================================================
	_AddDataLoadedEvent
=================================================
*/
	bool VCommandBuffer::_AddDataLoadedEvent (VCmdBatch::OnImageDataLoadedEvent &&ev)
	{
		CHECK_ERR( ev.callback and not ev.parts.empty() );

		_batch->_onImageLoadedEvents.push_back( std::move(ev) );
		return true;
	}


}	// FG

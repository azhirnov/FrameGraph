// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraphThread.h"
#include "VStagingBufferManager.h"
#include "VTaskGraph.hpp"

namespace FG
{
namespace {
	static constexpr EThreadUsage	GraphicsBit		= EThreadUsage::Graphics;
	static constexpr EThreadUsage	ComputeBit		= EThreadUsage::Graphics | EThreadUsage::AsyncCompute;
	static constexpr EThreadUsage	TransferBit		= EThreadUsage::Graphics | EThreadUsage::AsyncCompute | EThreadUsage::AsyncStreaming;
}
	
/*
=================================================
	operator ++ (ExeOrderIndex)
=================================================
*/
	ExeOrderIndex&  operator ++ (ExeOrderIndex &value)
	{
		value = BitCast<ExeOrderIndex>( BitCast<uint>( value ) + 1 );
		return value;
	}

/*
=================================================
	_BuildCommandBuffers
=================================================
*/
	bool VFrameGraphThread::_BuildCommandBuffers ()
	{
		if ( _taskGraph.Empty() )
			return true;

		VkCommandBuffer		cmd	= _CreateCommandBuffer( _currUsage );
		VDevice const&		dev	= GetDevice();
		
		// begin
		{
			VkCommandBufferBeginInfo	info = {};
			info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CALL( dev.vkBeginCommandBuffer( cmd, &info ));
		}
		
		// TODO: is it realy needed?
		// add global memory and execution barrier between command buffers in same batch
		/*if ( _indexInBatch > 0 )
		{
			VkMemoryBarrier	barrier = {};
			barrier.sType			= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask	= VK_ACCESS_MEMORY_WRITE_BIT;
			barrier.dstAccessMask	= VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

			_barrierMngr.AddMemoryBarrier( VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, barrier );
		}*/

		// commit image layout transition and other
		_barrierMngr.Commit( dev, cmd );

		CHECK( _ProcessTasks( cmd ));

		// transit image layout to default state
		// add memory dependency to flush caches
		{
			_resourceMngr.FlushLocalResourceStates( ExeOrderIndex::Final, _barrierMngr, GetDebugger() );
			_barrierMngr.Commit( dev, cmd );
		}

		VK_CALL( dev.vkEndCommandBuffer( cmd ));
		return true;
	}
	
/*
=================================================
	_ProcessTasks
=================================================
*/
	bool VFrameGraphThread::_ProcessTasks (VkCommandBuffer cmd)
	{
		VTaskProcessor	processor{ *this, _barrierMngr, cmd, _cmdBatchId, _indexInBatch };

		++_visitorID;
		_exeOrderIndex = ExeOrderIndex::First;

		TempTaskArray_t		pending{ GetAllocator() };
		pending.reserve( 128 );
		pending.assign( _taskGraph.Entries().begin(), _taskGraph.Entries().end() );

		for (uint k = 0; k < 10 and not pending.empty(); ++k)
		{
			for (size_t i = 0; i < pending.size(); ++i)
			{
				auto&	node = pending[i];
				
				if ( node->VisitorID() == _visitorID )
					continue;

				// wait for input
				bool	input_processed = true;

				for (auto in_node : node->Inputs())
				{
					if ( in_node->VisitorID() != _visitorID ) {
						input_processed = false;
						break;
					}
				}

				if ( not input_processed )
					continue;
			
				node->SetVisitorID( _visitorID );
				node->SetExecutionOrder( ++_exeOrderIndex );
				
				processor.Run( node );

				for (auto out_node : node->Outputs())
				{
					pending.push_back( out_node );
				}

				pending.erase( pending.begin() + i );
				--i;
			}
		}
		return true;
	}

/*
=================================================
	CompareRenderTargets
=================================================
*
	ND_ static bool CompareRenderTargets (const VLogicalRenderPass::ColorTarget &lhs, const VLogicalRenderPass::ColorTarget &rhs)
	{
		if ( lhs._hash != rhs._hash )
			return false;

		return	lhs.image	== rhs.image	and
				lhs.desc	== rhs.desc;
	}

/*
=================================================
	CompareRenderPasses
=================================================
*
	ND_ static uint CompareRenderPasses (const VLogicalRenderPass *prev, const VLogicalRenderPass *curr)
	{
		uint	counter = 0;

		counter += uint(CompareRenderTargets( curr->GetDepthStencilTarget(), prev->GetDepthStencilTarget() ));

		for (auto& ct : prev->GetColorTargets())
		{
			auto	iter = curr->GetColorTargets().find( ct.first );

			if ( iter == curr->GetColorTargets().end() )
				continue;
			
			counter += uint(CompareRenderTargets( ct.second, iter->second ));
		}

		return counter;
	}

/*
=================================================
	AddTask (SubmitRenderPass)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const SubmitRenderPass &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( _currUsage, GraphicsBit ));

		auto*	rp_task = static_cast< VFgTask<SubmitRenderPass> *>(_taskGraph.Add( this, task ));

		//_renderPassGraph.Add( rp_task );

		return rp_task;
	}
	
/*
=================================================
	AddTask (DispatchCompute)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const DispatchCompute &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _currUsage, ComputeBit ));

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (DispatchIndirectCompute)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const DispatchIndirectCompute &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _currUsage, ComputeBit ));

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (CopyBuffer)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const CopyBuffer &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		CHECK_ERR( task.srcBuffer and task.dstBuffer );
		ASSERT( EnumAny( _currUsage, TransferBit ));

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (CopyImage)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const CopyImage &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		CHECK_ERR( task.srcImage and task.dstImage );
		ASSERT( EnumAny( _currUsage, TransferBit ));

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (CopyBufferToImage)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const CopyBufferToImage &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		CHECK_ERR( task.srcBuffer and task.dstImage );
		ASSERT( EnumAny( _currUsage, TransferBit ));

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (CopyImageToBuffer)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const CopyImageToBuffer &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		CHECK_ERR( task.srcImage and task.dstBuffer );
		ASSERT( EnumAny( _currUsage, TransferBit ));

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (BlitImage)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const BlitImage &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		CHECK_ERR( task.srcImage and task.dstImage );
		ASSERT( EnumAny( _currUsage, GraphicsBit ));

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (ResolveImage)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const ResolveImage &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		CHECK_ERR( task.srcImage and task.dstImage );
		ASSERT( EnumAny( _currUsage, GraphicsBit ));

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (FillBuffer)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const FillBuffer &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		CHECK_ERR( task.dstBuffer );

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (ClearColorImage)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const ClearColorImage &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		CHECK_ERR( task.dstImage );
		ASSERT( EnumAny( _currUsage, ComputeBit ));

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (ClearDepthStencilImage)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const ClearDepthStencilImage &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		CHECK_ERR( task.dstImage );
		ASSERT( EnumAny( _currUsage, ComputeBit ));

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (UpdateBuffer)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const UpdateBuffer &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		CHECK_ERR( task.dstBuffer );
		ASSERT( EnumAny( _currUsage, TransferBit ));

		if ( _stagingMngr )
			return _AddUpdateBufferTask( task );
		
		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	_AddUpdateBufferTask
=================================================
*/
	Task  VFrameGraphThread::_AddUpdateBufferTask (const UpdateBuffer &task)
	{
		TaskGroupSync	sync;

		// copy to staging buffer
		for (BytesU readn; readn < ArraySizeOf(task.data);)
		{
			CopyBuffer	copy;
			copy.taskName	= task.taskName;
			copy.debugColor	= task.debugColor;
			copy.depends	= task.depends;
			copy.dstBuffer	= task.dstBuffer;

			BytesU		off, size;
			CHECK_ERR( _stagingMngr->StoreBufferData( task.data, readn, OUT copy.srcBuffer, OUT off, OUT size ));

			copy.AddRegion( off, task.offset + readn, size );
			readn += size;

			sync.depends.push_back( AddTask( copy ));
		}

		// return task handle
		if ( sync.depends.size() > 1 )
			return _taskGraph.Add( this, sync );
		else
			return sync.depends.front();
	}
	
/*
=================================================
	AddTask (UpdateImage)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const UpdateImage &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _currUsage, TransferBit ));

		if ( _stagingMngr )
			return _AddUpdateImageTask( task );

		RETURN_ERR( "not supported", Task(null) );
	}
	
/*
=================================================
	_AddUpdateImageTask
=================================================
*/
	Task  VFrameGraphThread::_AddUpdateImageTask (const UpdateImage &task)
	{
		CHECK_ERR( task.dstImage );
		ImageDesc const&	img_desc = _resourceMngr.GetResource( task.dstImage )->Description();

		ASSERT( task.mipmapLevel < img_desc.maxLevel );
		ASSERT( task.arrayLayer < img_desc.arrayLayers );
		ASSERT(Any( task.imageSize > uint3(0) ));

		const uint3		image_size		= Max( task.imageSize, 1u );
		const uint		bpp				= EPixelFormat_BitPerPixel( img_desc.format, task.aspectMask );
		const BytesU	row_pitch		= Max( task.dataRowPitch, (image_size.x * BytesU(bpp)) / 8 );
		const BytesU	slice_pitch		= Max( task.dataSlicePitch, (image_size.y * row_pitch) );

		ASSERT( ArraySizeOf(task.data) == slice_pitch * image_size.z );
		

		TaskGroupSync		sync;
		const BytesU		min_size	= 16_Mb;		// TODO
		const uint			row_length	= uint((row_pitch * 8) / bpp);
		const uint			img_height	= uint(slice_pitch / row_pitch); //task.dstImage->Height();
		const BytesU		total_size	= ArraySizeOf(task.data);

		ASSERT( (row_pitch * 8) % bpp == 0 );
		ASSERT( slice_pitch % row_pitch == 0 );


		// copy to staging buffer slice by slice
		if ( total_size < min_size )
		{
			uint	z_offset = 0;
			for (BytesU readn; readn < total_size;)
			{
				CopyBufferToImage	copy;
				copy.taskName	= task.taskName;
				copy.debugColor	= task.debugColor;
				copy.depends	= task.depends;
				copy.dstImage	= task.dstImage;
		
				BytesU		off, size;
				CHECK_ERR( _stagingMngr->StoreImageData( task.data, readn, slice_pitch, total_size,
														 OUT copy.srcBuffer, OUT off, OUT size ));

				const uint	z_size = uint(size / slice_pitch);

				copy.AddRegion( off, row_length, img_height,
							    ImageSubresourceRange{ task.mipmapLevel, task.arrayLayer, 1, task.aspectMask },
							    task.imageOffset + int3(0, 0, z_offset), uint3(image_size.x, image_size.y, z_size) );
				readn += size;
				z_offset += z_size;
			
				sync.depends.push_back( AddTask( copy ));
			}
			ASSERT( z_offset == image_size.z );
		}
		else

		// copy to staging buffer row by row
		for (uint slice = 0; slice < image_size.z; ++slice)
		{
			uint				y_offset	= 0;
			ArrayView<uint8_t>	data		= task.data.section( size_t(slice * slice_pitch), size_t(slice_pitch) );

			for (BytesU readn; readn < ArraySizeOf(data);)
			{
				CopyBufferToImage	copy;
				copy.taskName	= task.taskName;
				copy.debugColor	= task.debugColor;
				copy.depends	= task.depends;
				copy.dstImage	= task.dstImage;
		
				BytesU		off, size;
				CHECK_ERR( _stagingMngr->StoreImageData( data, readn, row_pitch, total_size,
														 OUT copy.srcBuffer, OUT off, OUT size ));

				const uint	y_size = uint(size / row_pitch);

				copy.AddRegion( off, row_length, img_height,
							    ImageSubresourceRange{ task.mipmapLevel, task.arrayLayer, 1, task.aspectMask },
							    task.imageOffset + int3(0, y_offset, slice), uint3(image_size.x, image_size.y, 1) );
				readn += size;
			
				sync.depends.push_back( AddTask( copy ));
			}
			ASSERT( y_offset == image_size.y );
		}


		// return task handle
		if ( sync.depends.size() > 1 )
			return _taskGraph.Add( this, sync );
		else
			return sync.depends.front();
	}
	
/*
=================================================
	AddTask (ReadBuffer)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const ReadBuffer &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _currUsage, TransferBit ));

		if ( _stagingMngr )
			return _AddReadBufferTask( task );
		
		RETURN_ERR( "not supported", Task(null) );
	}
	
/*
=================================================
	_AddReadBufferTask
=================================================
*/
	Task  VFrameGraphThread::_AddReadBufferTask (const ReadBuffer &task)
	{
		using OnDataLoadedEvent = VStagingBufferManager::OnBufferDataLoadedEvent;

		CHECK_ERR( task.srcBuffer and task.callback );

		OnDataLoadedEvent	load_event{ task.callback, task.size };
		TaskGroupSync		sync;

		// copy to staging buffer
		for (BytesU written; written < task.size;)
		{
			CopyBuffer	copy;
			copy.taskName	= task.taskName;
			copy.debugColor	= task.debugColor;
			copy.depends	= task.depends;
			copy.srcBuffer	= task.srcBuffer;

			OnDataLoadedEvent::Range	range;
			CHECK_ERR( _stagingMngr->AddPendingLoad( written, task.size, OUT copy.dstBuffer, OUT range ));
			
			copy.AddRegion( range.offset, task.offset + written, range.size );
			written += range.size;

			load_event.parts.push_back( range );

			sync.depends.push_back( AddTask( copy ));
		}

		_stagingMngr->AddDataLoadedEvent( std::move(load_event) );
		

		// return task handle
		if ( sync.depends.size() > 1 )
			return _taskGraph.Add( this, sync );
		else
			return sync.depends.front();
	}
	
/*
=================================================
	AddTask (ReadImage)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const ReadImage &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _currUsage, TransferBit ));

		if ( _stagingMngr )
			return _AddReadImageTask( task );
		
		RETURN_ERR( "not supported", Task(null) );
	}
	
/*
=================================================
	_AddReadImageTask
=================================================
*/
	Task  VFrameGraphThread::_AddReadImageTask (const ReadImage &task)
	{
		using OnDataLoadedEvent = VStagingBufferManager::OnImageDataLoadedEvent;

		CHECK_ERR( task.srcImage and task.callback );
		ImageDesc const&	img_desc = _resourceMngr.GetResource( task.srcImage )->Description();

		ASSERT( task.mipmapLevel < img_desc.maxLevel );
		ASSERT( task.arrayLayer < img_desc.arrayLayers );
		ASSERT(Any( task.imageSize > uint3(0) ));
		
		const uint3			image_size	= Max( task.imageSize, 1u );
		const BytesU		min_size	= 16_Mb;		// TODO
		const uint			bpp			= EPixelFormat_BitPerPixel( img_desc.format, task.aspectMask );
		const BytesU		row_pitch	= (image_size.x * BytesU(bpp)) / 8;
		const BytesU		slice_pitch	= row_pitch * image_size.y;
		const BytesU		total_size	= slice_pitch * image_size.z;
		const uint			row_length	= uint((row_pitch * 8) / bpp);
		const uint			img_height	= uint(slice_pitch / row_pitch);

		OnDataLoadedEvent	load_event{ task.callback, total_size, image_size, row_pitch, slice_pitch, img_desc.format, task.aspectMask };
		TaskGroupSync		sync;
		
		// copy to staging buffer slice by slice
		if ( total_size < min_size )
		{
			uint	z_offset = 0;
			for (BytesU written; written < total_size;)
			{
				CopyImageToBuffer	copy;
				copy.taskName	= task.taskName;
				copy.debugColor	= task.debugColor;
				copy.depends	= task.depends;
				copy.srcImage	= task.srcImage;
		
				OnDataLoadedEvent::Range	range;
				CHECK_ERR( _stagingMngr->AddPendingLoad( written, total_size, slice_pitch, OUT copy.dstBuffer, OUT range ));
			
				const uint	z_size = uint(range.size / slice_pitch);

				copy.AddRegion( ImageSubresourceRange{ task.mipmapLevel, task.arrayLayer, 1, task.aspectMask },
							    task.imageOffset + int3(0, 0, z_offset), uint3(image_size.x, image_size.y, z_size),
								range.offset, row_length, img_height );
				
				load_event.parts.push_back( range );

				written  += range.size;
				z_offset += z_size;
			
				sync.depends.push_back( AddTask( copy ));
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
				CopyImageToBuffer	copy;
				copy.taskName	= task.taskName;
				copy.debugColor	= task.debugColor;
				copy.depends	= task.depends;
				copy.srcImage	= task.srcImage;
		
				OnDataLoadedEvent::Range	range;
				CHECK_ERR( _stagingMngr->AddPendingLoad( written, total_size, row_pitch, OUT copy.dstBuffer, OUT range ));

				const uint	y_size = uint(range.size / row_pitch);

				copy.AddRegion( ImageSubresourceRange{ task.mipmapLevel, task.arrayLayer, 1, task.aspectMask },
							    task.imageOffset + int3(0, y_offset, slice), uint3(image_size.x, image_size.y, 1),
								range.offset, row_length, img_height );
				
				load_event.parts.push_back( range );

				written  += range.size;
				y_offset += y_size;
			
				sync.depends.push_back( AddTask( copy ));
			}
			ASSERT( y_offset == image_size.y );
		}

		_stagingMngr->AddDataLoadedEvent( std::move(load_event) );


		// return task handle
		if ( sync.depends.size() > 1 )
			return _taskGraph.Add( this, sync );
		else
			return sync.depends.front();
	}

/*
=================================================
	AddTask (Present)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const FG::Present &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );

		if ( _swapchain )
			return _AddPresentTask( task );
		
		RETURN_ERR( "not supported", Task(null) );
	}

/*
=================================================
	_AddPresentTask
=================================================
*/
	Task  VFrameGraphThread::_AddPresentTask (const FG::Present &task)
	{
		CHECK_ERR( task.srcImage );
		/*
		RawImageID	swapchain_image;
		CHECK_ERR( _swapchain->Acquire( ESwapchainImage::Primary, OUT swapchain_image ));

		auto*	src_data = _resourceMngr.GetResource( task.srcImage );
		auto*	dst_data = _resourceMngr.GetResource( swapchain_image );

		BlitImage	blit;
		blit.debugColor	= task.debugColor;
		blit.depends	= task.depends;
		blit.taskName	= task.taskName;

		blit.srcImage	= task.srcImage;
		blit.dstImage	= swapchain_image;
		blit.filter		= EFilter::Nearest;
		blit.AddRegion( {}, int3{}, int3{src_data->Dimension()}, {}, int3{}, int3{dst_data->Dimension()} );

		//CHECK_ERR( _swapchain->Present( swapchain_image ));
		*/
		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (PresentVR)
=================================================
*
	Task  VFrameGraphThread::AddTask (const PresentVR &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );

		return _taskGraph.Add( this, task );
	}

/*
=================================================
	AddTask (TaskGroupSync)
=================================================
*
	Task  VFrameGraphThread::AddTask (const TaskGroupSync &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );

		return _taskGraph.Add( this, task );
	}

/*
=================================================
	AddDrawTask (DrawTask)
=================================================
*/
	void  VFrameGraphThread::AddDrawTask (LogicalPassID renderPass, const DrawTask &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording(), void() );
		
		auto *	rp  = _resourceMngr.GetState( renderPass );
		void *	ptr = _mainAllocator.Alloc< VFgDrawTask<DrawTask> >();

		rp->AddTask( PlacementNew< VFgDrawTask<DrawTask> >(
						 ptr,
						 this, task,
						 VTaskProcessor::Visit1_DrawTask,
						 VTaskProcessor::Visit2_DrawTask ));
	}
	
/*
=================================================
	AddDrawTask (DrawIndexedTask)
=================================================
*/
	void  VFrameGraphThread::AddDrawTask (LogicalPassID renderPass, const DrawIndexedTask &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording(), void() );
		
		auto *	rp  = _resourceMngr.GetState( renderPass );
		void *	ptr = _mainAllocator.Alloc< VFgDrawTask<DrawIndexedTask> >();

		rp->AddTask( PlacementNew< VFgDrawTask<DrawIndexedTask> >(
						 ptr,
						 this, task,
						 VTaskProcessor::Visit1_DrawIndexedTask,
						 VTaskProcessor::Visit2_DrawIndexedTask ));
	}
	
/*
=================================================
	AddDrawTask (DrawMeshTask)
=================================================
*/
	void  VFrameGraphThread::AddDrawTask (LogicalPassID renderPass, const DrawMeshTask &task)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording(), void() );
		
		auto *	rp  = _resourceMngr.GetState( renderPass );
		void *	ptr = _mainAllocator.Alloc< VFgDrawTask<DrawMeshTask> >();

		rp->AddTask( PlacementNew< VFgDrawTask<DrawMeshTask> >(
						 ptr,
						 this, task,
						 VTaskProcessor::Visit1_DrawMeshTask,
						 VTaskProcessor::Visit2_DrawMeshTask ));
	}

/*
=================================================
	CreateRenderPass
=================================================
*/
	LogicalPassID  VFrameGraphThread::CreateRenderPass (const RenderPassDesc &desc)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _threadUsage, GraphicsBit ));

		return _resourceMngr.CreateLogicalRenderPass( desc );
	}
	
/*
=================================================
	Acquire
=================================================
*/
	bool  VFrameGraphThread::Acquire (const ImageID &id, bool immutable)
	{
		SCOPELOCK( _rcCheck );
		// TODO
		return false;
	}
	
/*
=================================================
	Acquire
=================================================
*/
	bool  VFrameGraphThread::Acquire (const ImageID &id, MipmapLevel baseLevel, uint levelCount, ImageLayer baseLayer, uint layerCount, bool immutable)
	{
		SCOPELOCK( _rcCheck );
		// TODO
		return false;
	}
	
/*
=================================================
	Acquire
=================================================
*/
	bool  VFrameGraphThread::Acquire (const BufferID &id, bool immutable)
	{
		SCOPELOCK( _rcCheck );
		// TODO
		return false;
	}
	
/*
=================================================
	Acquire
=================================================
*/
	bool  VFrameGraphThread::Acquire (const BufferID &id, BytesU offset, BytesU size, bool immutable)
	{
		SCOPELOCK( _rcCheck );
		// TODO
		return false;
	}


}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraphThread.h"
#include "VStagingBufferManager.h"
#include "VTaskGraph.hpp"
#include "VFrameGraphDebugger.h"

namespace FG
{
namespace {
	static constexpr EThreadUsage	GraphicsBit		= EThreadUsage::Graphics;
	static constexpr EThreadUsage	ComputeBit		= EThreadUsage::Graphics | EThreadUsage::AsyncCompute;
	static constexpr EThreadUsage	RayTracingBit	= EThreadUsage::Graphics | EThreadUsage::AsyncCompute;
	static constexpr EThreadUsage	TransferBit		= EThreadUsage::Graphics | EThreadUsage::AsyncCompute | EThreadUsage::AsyncStreaming;
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

		VkCommandBuffer		cmd	= _CreateCommandBuffer();
		VDevice const&		dev	= GetDevice();
		
		// begin
		{
			VkCommandBufferBeginInfo	info = {};
			info.sType	= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.flags	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CALL( dev.vkBeginCommandBuffer( cmd, &info ));
			dev.vkCmdWriteTimestamp( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, _queryPool, _currQueue->frames[_frameId].queryIndex );
		}

		if ( _shaderDebugger )
			_shaderDebugger->OnBeginRecording( cmd );

		// commit image layout transition and other
		_barrierMngr.Commit( dev, cmd );

		CHECK( _ProcessTasks( cmd ));

		// transit image layout to default state
		// add memory dependency to flush caches
		{
			auto	batch_exe_order = _submissionGraph->GetExecutionOrder( _cmdBatchId, _indexInBatch );

			_resourceMngr.FlushLocalResourceStates( ExeOrderIndex::Final, batch_exe_order, _barrierMngr, GetDebugger() );
			_barrierMngr.Commit( dev, cmd );
		}

		if ( _shaderDebugger )
			_shaderDebugger->OnEndRecording( cmd );
		
		// end
		{
			dev.vkCmdWriteTimestamp( cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, _queryPool, _currQueue->frames[_frameId].queryIndex + 1 );
			VK_CALL( dev.vkEndCommandBuffer( cmd ));
		}
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
		
		ExeOrderIndex	exe_order_index = ExeOrderIndex::First;

		TempTaskArray_t		pending{ GetAllocator() };
		pending.reserve( 128 );
		pending.assign( _taskGraph.Entries().begin(), _taskGraph.Entries().end() );

		for (uint k = 0; k < 10 and not pending.empty(); ++k)
		{
			for (auto iter = pending.begin(); iter != pending.end();)
			{
				auto&	node = (*iter);
				
				if ( node->VisitorID() == _visitorID ) {
					++iter;
					continue;
				}

				// wait for input
				bool	input_processed = true;

				for (auto in_node : node->Inputs())
				{
					if ( in_node->VisitorID() != _visitorID ) {
						input_processed = false;
						break;
					}
				}

				if ( not input_processed ) {
					++iter;
					continue;
				}

				node->SetVisitorID( _visitorID );
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

/*
=================================================
	AddTask (SubmitRenderPass)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const SubmitRenderPass &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumEq( _currUsage, GraphicsBit ));

		auto*	rp_task = _taskGraph.Add( this, task );

		// TODO
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
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _currUsage, ComputeBit ));

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (DispatchComputeIndirect)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const DispatchComputeIndirect &task)
	{
		EXLOCK( _rcCheck );
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
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
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
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
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
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
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
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
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
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
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
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _currUsage, GraphicsBit ));

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (GenerateMipmaps)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const GenerateMipmaps &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
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
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	AddTask (ClearColorImage)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const ClearColorImage &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
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
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
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
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
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
				CHECK_ERR( _stagingMngr->StorePartialData( reg.data, readn, OUT src_buffer, OUT off, OUT size ));
			
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
	Task  VFrameGraphThread::AddTask (const UpdateImage &task)
	{
		EXLOCK( _rcCheck );
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
		const auto&		fmt_info		= EPixelFormat_GetInfo( img_desc.format );
		const auto&		block_dim		= fmt_info.blockSize;
		const uint		block_size		= task.aspectMask != EImageAspect::Stencil ? fmt_info.bitsPerBlock : fmt_info.bitsPerBlock2;
		const BytesU	row_pitch		= Max( task.dataRowPitch, BytesU(image_size.x * block_size + block_dim.x-1) / (block_dim.x * 8) );
		const BytesU	min_slice_pitch	= (image_size.y * row_pitch + block_dim.y-1) / block_dim.y;
		const BytesU	slice_pitch		= Max( task.dataSlicePitch, min_slice_pitch );
		const BytesU	total_size		= image_size.z > 1 ? slice_pitch * image_size.z : min_slice_pitch;

		CHECK_ERR( total_size == ArraySizeOf(task.data) );

		const BytesU		min_size	= _stagingMngr->GetMaxWritableStoregeSize();
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
				CHECK_ERR( _stagingMngr->StoreImageData( task.data, readn, slice_pitch, total_size,
														 OUT src_buffer, OUT off, OUT size ));
				
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
				CHECK_ERR( _stagingMngr->StoreImageData( data, readn, row_pitch * block_dim.y, total_size,
														 OUT src_buffer, OUT off, OUT size ));
				
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
	Task  VFrameGraphThread::AddTask (const ReadBuffer &task)
	{
		EXLOCK( _rcCheck );
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
			CHECK_ERR( _stagingMngr->AddPendingLoad( written, task.size, OUT dst_buffer, OUT range ));
			
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

		_stagingMngr->AddDataLoadedEvent( std::move(load_event) );
		
		return AddTask( copy );
	}
	
/*
=================================================
	AddTask (ReadImage)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const ReadImage &task)
	{
		EXLOCK( _rcCheck );
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
		
		const uint3			image_size		= Max( task.imageSize, 1u );
		const BytesU		min_size		= _stagingMngr->GetMaxReadableStorageSize();
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
				CHECK_ERR( _stagingMngr->AddPendingLoad( written, total_size, slice_pitch, OUT dst_buffer, OUT range ));
			
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
				CHECK_ERR( _stagingMngr->AddPendingLoad( written, total_size, row_pitch * block_dim.y, OUT dst_buffer, OUT range ));
				
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

		_stagingMngr->AddDataLoadedEvent( std::move(load_event) );

		return AddTask( copy );
	}

/*
=================================================
	AddTask (Present)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const FG::Present &task)
	{
		EXLOCK( _rcCheck );
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
		/*CHECK_ERR( task.srcImage );
	
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
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );

		return _taskGraph.Add( this, task );
	}
	
/*
=================================================
	_AllocStorage
=================================================
*/
	template <typename T>
	inline bool VFrameGraphThread::_AllocStorage (size_t count, OUT const VLocalBuffer* &outBuffer, OUT VkDeviceSize &outOffset, OUT T* &outPtr)
	{
		CHECK_ERR( _stagingMngr );

		RawBufferID		buffer;
		BytesU			buf_offset, buf_size;
		CHECK_ERR( _stagingMngr->GetWritableBuffer( SizeOf<T> * count, SizeOf<T> * count, OUT buffer, OUT buf_offset, OUT buf_size, OUT BitCast<void *>(outPtr) ));
		
		outBuffer	= GetResourceManager()->ToLocal( buffer );
		outOffset	= VkDeviceSize(buf_offset);
		return true;
	}
	
/*
=================================================
	_StoreData
=================================================
*/
	inline bool VFrameGraphThread::_StoreData (const void *dataPtr, BytesU dataSize, BytesU offsetAlign, OUT const VLocalBuffer* &outBuffer, OUT VkDeviceSize &outOffset)
	{
		CHECK_ERR( _stagingMngr );
		
		RawBufferID		buffer;
		BytesU			buf_offset, buf_size;
		CHECK_ERR( _stagingMngr->StoreSolidData( dataPtr, dataSize, offsetAlign, OUT buffer, OUT buf_offset, OUT buf_size ));
		
		outBuffer	= GetResourceManager()->ToLocal( buffer );
		outOffset	= VkDeviceSize(buf_offset);
		return true;
	}
	
/*
=================================================
	AddTask (UpdateRayTracingShaderTable)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const UpdateRayTracingShaderTable &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _currUsage, RayTracingBit ));

		return _taskGraph.Add( this, task );
	}

/*
=================================================
	AddTask (BuildRayTracingGeometry)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const BuildRayTracingGeometry &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _currUsage, RayTracingBit ));
		
		auto*	result	= _taskGraph.Add( this, task );
		auto*	geom	= GetResourceManager()->ToLocal( task.rtGeometry );

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
		BufferID	buf = CreateBuffer( BufferDesc{ BytesU(mem_req.memoryRequirements.size), EBufferUsage::RayTracing }, Default, Default );
		result->_scratchBuffer = GetResourceManager()->ToLocal( buf.Get() );
		ReleaseResource( buf );
		
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
				auto*	vb = GetResourceManager()->ToLocal( src.vertexBuffer );
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
				auto*	ib = GetResourceManager()->ToLocal( src.indexBuffer );
				CHECK_ERR( ib );
				dst.geometry.triangles.indexData	= ib->Handle();
				dst.geometry.triangles.indexOffset	= VkDeviceSize(src.indexOffset);
				result->_usableBuffers.insert( ib );
			}

			// transformation
			if ( src.transformBuffer )
			{
				auto*	tb = GetResourceManager()->ToLocal( src.transformBuffer );
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
				auto*	ab = GetResourceManager()->ToLocal( src.aabbBuffer );
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
	Task  VFrameGraphThread::AddTask (const BuildRayTracingScene &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _currUsage, RayTracingBit ));
		
		auto*	result	= _taskGraph.Add( this, task );
		auto*	scene	= GetResourceManager()->ToLocal( task.rtScene );

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
		BufferID	buf = CreateBuffer( BufferDesc{ BytesU(mem_req.memoryRequirements.size), EBufferUsage::RayTracing }, Default, Default );
		result->_scratchBuffer = GetResourceManager()->ToLocal( buf.Get() );
		ReleaseResource( buf );

		VkGeometryInstance*  vk_instances;
		CHECK_ERR( _AllocStorage<VkGeometryInstance>( task.instances.size(), OUT result->_instanceBuffer, OUT result->_instanceBufferOffset, OUT vk_instances ));

		// sort instances by ID
		Array<uint>	sorted;		// TODO: use temporary allocator
		sorted.resize( task.instances.size() );
		for (size_t i = 0; i < sorted.size(); ++i) { sorted[i] = uint(i); }
		std::sort( sorted.begin(), sorted.end(), [inst = &task.instances] (auto lhs, auto rhs) { return (*inst)[lhs].instanceId < (*inst)[rhs].instanceId; });

		result->_rtGeometries			= _mainAllocator.Alloc< VLocalRTGeometry const *>( task.instances.size() );
		result->_instances				= _mainAllocator.Alloc< Pair<InstanceID, RTGeometryID> >( task.instances.size() );
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

			blas = GetResourceManager()->ToLocal( src.geometryId, /*acquire ref*/true );
			CHECK_ERR( blas );

			dst.blasHandle		= blas->BLASHandle();
			dst.transformRow0	= src.transform[0];
			dst.transformRow1	= src.transform[1];
			dst.transformRow2	= src.transform[2];
			dst.customIndex		= src.customId;
			dst.mask			= src.mask;
			dst.instanceOffset	= result->_maxHitShaderCount * result->_hitShadersPerInstance;
			dst.flags			= VEnumCast( src.flags );
			
			PlacementNew<Pair<InstanceID, RTGeometryID>>( result->_instances + idx, src.instanceId, src.geometryId );

			result->_maxHitShaderCount += (blas->MaxGeometryCount() * result->_hitShadersPerInstance);
		}
		
		return result;
	}
	
/*
=================================================
	AddTask (TraceRays)
=================================================
*/
	Task  VFrameGraphThread::AddTask (const TraceRays &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _currUsage, RayTracingBit ));

		return _taskGraph.Add( this, task );
	}

/*
=================================================
	AddTask (DrawVertices)
=================================================
*/
	void  VFrameGraphThread::AddTask (LogicalPassID renderPass, const DrawVertices &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording(), void());
		
		auto *	rp  = _resourceMngr.ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<DrawVertices> >( 
						this, task,
						VTaskProcessor::Visit1_DrawVertices,
						VTaskProcessor::Visit2_DrawVertices );
	}
	
/*
=================================================
	AddTask (DrawIndexed)
=================================================
*/
	void  VFrameGraphThread::AddTask (LogicalPassID renderPass, const DrawIndexed &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording(), void());
		
		auto *	rp  = _resourceMngr.ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<DrawIndexed> >(
						this, task,
						VTaskProcessor::Visit1_DrawIndexed,
						VTaskProcessor::Visit2_DrawIndexed );
	}
	
/*
=================================================
	AddTask (DrawMeshes)
=================================================
*/
	void  VFrameGraphThread::AddTask (LogicalPassID renderPass, const DrawMeshes &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording(), void());
		
		auto *	rp  = _resourceMngr.ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<DrawMeshes> >(
						this, task,
						VTaskProcessor::Visit1_DrawMeshes,
						VTaskProcessor::Visit2_DrawMeshes );
	}
	
/*
=================================================
	AddTask (DrawVerticesIndirect)
=================================================
*/
	void  VFrameGraphThread::AddTask (LogicalPassID renderPass, const DrawVerticesIndirect &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording(), void());
		
		auto *	rp  = _resourceMngr.ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<DrawVerticesIndirect> >(
						this, task,
						VTaskProcessor::Visit1_DrawVerticesIndirect,
						VTaskProcessor::Visit2_DrawVerticesIndirect );
	}
	
/*
=================================================
	AddTask (DrawIndexedIndirect)
=================================================
*/
	void  VFrameGraphThread::AddTask (LogicalPassID renderPass, const DrawIndexedIndirect &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording(), void());
		
		auto *	rp  = _resourceMngr.ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<DrawIndexedIndirect> >(
						this, task,
						VTaskProcessor::Visit1_DrawIndexedIndirect,
						VTaskProcessor::Visit2_DrawIndexedIndirect );
	}
	
/*
=================================================
	AddTask (DrawMeshesIndirect)
=================================================
*/
	void  VFrameGraphThread::AddTask (LogicalPassID renderPass, const DrawMeshesIndirect &task)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording(), void());
		
		auto *	rp  = _resourceMngr.ToLocal( renderPass );
		CHECK_ERR( rp, void());

		rp->AddTask< VFgDrawTask<DrawMeshesIndirect> >(
						this, task,
						VTaskProcessor::Visit1_DrawMeshesIndirect,
						VTaskProcessor::Visit2_DrawMeshesIndirect );
	}

/*
=================================================
	CreateRenderPass
=================================================
*/
	LogicalPassID  VFrameGraphThread::CreateRenderPass (const RenderPassDesc &desc)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsRecording() );
		ASSERT( EnumAny( _threadUsage, GraphicsBit ));

		return _resourceMngr.CreateLogicalRenderPass( desc );
	}
	
/*
=================================================
	Acquire
=================================================
*
	bool  VFrameGraphThread::Acquire (const ImageID &id, bool immutable)
	{
		EXLOCK( _rcCheck );
		// TODO
		FG_UNUSED( id, immutable );
		return false;
	}
	
/*
=================================================
	Acquire
=================================================
*
	bool  VFrameGraphThread::Acquire (const ImageID &id, MipmapLevel baseLevel, uint levelCount, ImageLayer baseLayer, uint layerCount, bool immutable)
	{
		EXLOCK( _rcCheck );
		// TODO
		FG_UNUSED( id, baseLevel, levelCount, baseLayer, layerCount, immutable );
		return false;
	}
	
/*
=================================================
	Acquire
=================================================
*
	bool  VFrameGraphThread::Acquire (const BufferID &id, bool immutable)
	{
		EXLOCK( _rcCheck );
		// TODO
		FG_UNUSED( id, immutable );
		return false;
	}
	
/*
=================================================
	Acquire
=================================================
*
	bool  VFrameGraphThread::Acquire (const BufferID &id, BytesU offset, BytesU size, bool immutable)
	{
		EXLOCK( _rcCheck );
		// TODO

		FG_UNUSED( id, offset, size, immutable );
		return false;
	}
	
/*
=================================================
	UpdateHostBuffer
=================================================
*/
	bool  VFrameGraphThread::UpdateHostBuffer (const BufferID &id, BytesU offset, BytesU size, const void *data)
	{
		EXLOCK( _rcCheck );

		void*	dst_ptr;
		CHECK_ERR( MapBufferRange( id, offset, INOUT size, OUT dst_ptr ));

		MemCopy( OUT dst_ptr, size, data, size );
		return true;
	}
	
/*
=================================================
	MapBufferRange
=================================================
*/
	bool  VFrameGraphThread::MapBufferRange (const BufferID &id, BytesU offset, INOUT BytesU &size, OUT void* &dataPtr)
	{
		EXLOCK( _rcCheck );

		VLocalBuffer const*		buffer = _resourceMngr.ToLocal( id.Get() );
		CHECK_ERR( buffer );

		VMemoryObj const*		memory = _resourceMngr.GetResource( buffer->ToGlobal()->GetMemoryID() );
		CHECK_ERR( memory );

		VMemoryObj::MemoryInfo	mem_info;
		CHECK_ERR( memory->GetInfo( OUT mem_info ));

		CHECK_ERR( mem_info.mappedPtr );
		CHECK_ERR( offset < buffer->Size() );

		size	= Min( size, mem_info.size - offset );
		dataPtr = mem_info.mappedPtr + offset;

		if ( _debugger )
			_debugger->AddHostWriteAccess( buffer->ToGlobal(), offset, size );
		
		return true;
	}


}	// FG

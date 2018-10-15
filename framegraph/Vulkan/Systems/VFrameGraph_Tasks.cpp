// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraph.h"
#include "VTaskProcessor.h"
#include "Shared/EnumUtils.h"

namespace FG
{

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
	Task  VFrameGraph::AddTask (const SubmitRenderPass &task)
	{
		CHECK_ERR( _isRecording and task.renderPass );
		CHECK_ERR( static_cast<VLogicalRenderPass *>(task.renderPass)->Submit() );

		auto*	rp_task = static_cast< VFgTask<SubmitRenderPass> *>(_taskGraph.Add( task ));

		_renderPassGraph.Add( rp_task );

		return rp_task;
	}
	
/*
=================================================
	AddTask (DispatchCompute)
=================================================
*/
	Task  VFrameGraph::AddTask (const DispatchCompute &task)
	{
		CHECK_ERR( _isRecording );

		return _taskGraph.Add( task );
	}
	
/*
=================================================
	AddTask (DispatchIndirectCompute)
=================================================
*/
	Task  VFrameGraph::AddTask (const DispatchIndirectCompute &task)
	{
		CHECK_ERR( _isRecording );

		return _taskGraph.Add( task );
	}
	
/*
=================================================
	AddTask (CopyBuffer)
=================================================
*/
	Task  VFrameGraph::AddTask (const CopyBuffer &task)
	{
		CHECK_ERR( _isRecording );
		CHECK_ERR( task.srcBuffer and task.dstBuffer );

		task.srcBuffer->AddUsage( EBufferUsage::TransferSrc );
		task.dstBuffer->AddUsage( EBufferUsage::TransferDst );

		return _taskGraph.Add( task );
	}
	
/*
=================================================
	AddTask (CopyImage)
=================================================
*/
	Task  VFrameGraph::AddTask (const CopyImage &task)
	{
		CHECK_ERR( _isRecording );
		CHECK_ERR( task.srcImage and task.dstImage );
		
		task.srcImage->AddUsage( EImageUsage::TransferSrc );
		task.dstImage->AddUsage( EImageUsage::TransferDst );

		return _taskGraph.Add( task );
	}
	
/*
=================================================
	AddTask (CopyBufferToImage)
=================================================
*/
	Task  VFrameGraph::AddTask (const CopyBufferToImage &task)
	{
		CHECK_ERR( _isRecording );
		CHECK_ERR( task.srcBuffer and task.dstImage );
		
		task.srcBuffer->AddUsage( EBufferUsage::TransferSrc );
		task.dstImage->AddUsage( EImageUsage::TransferDst );

		return _taskGraph.Add( task );
	}
	
/*
=================================================
	AddTask (CopyImageToBuffer)
=================================================
*/
	Task  VFrameGraph::AddTask (const CopyImageToBuffer &task)
	{
		CHECK_ERR( _isRecording );
		CHECK_ERR( task.srcImage and task.dstBuffer );
		
		task.srcImage->AddUsage( EImageUsage::TransferSrc );
		task.dstBuffer->AddUsage( EBufferUsage::TransferDst );

		return _taskGraph.Add( task );
	}
	
/*
=================================================
	AddTask (BlitImage)
=================================================
*/
	Task  VFrameGraph::AddTask (const BlitImage &task)
	{
		CHECK_ERR( _isRecording );
		CHECK_ERR( task.srcImage and task.dstImage );
		
		task.srcImage->AddUsage( EImageUsage::TransferSrc );
		task.dstImage->AddUsage( EImageUsage::TransferDst );

		return _taskGraph.Add( task );
	}
	
/*
=================================================
	AddTask (ResolveImage)
=================================================
*/
	Task  VFrameGraph::AddTask (const ResolveImage &task)
	{
		CHECK_ERR( _isRecording );
		CHECK_ERR( task.srcImage and task.dstImage );
		
		task.srcImage->AddUsage( EImageUsage::TransferSrc );
		task.dstImage->AddUsage( EImageUsage::TransferDst );

		return _taskGraph.Add( task );
	}
	
/*
=================================================
	AddTask (FillBuffer)
=================================================
*/
	Task  VFrameGraph::AddTask (const FillBuffer &task)
	{
		CHECK_ERR( _isRecording );
		CHECK_ERR( task.dstBuffer );

		task.dstBuffer->AddUsage( EBufferUsage::TransferDst );

		return _taskGraph.Add( task );
	}
	
/*
=================================================
	AddTask (ClearColorImage)
=================================================
*/
	Task  VFrameGraph::AddTask (const ClearColorImage &task)
	{
		CHECK_ERR( _isRecording );
		CHECK_ERR( task.dstImage );
		
		task.dstImage->AddUsage( EImageUsage::TransferDst );

		return _taskGraph.Add( task );
	}
	
/*
=================================================
	AddTask (ClearDepthStencilImage)
=================================================
*/
	Task  VFrameGraph::AddTask (const ClearDepthStencilImage &task)
	{
		CHECK_ERR( _isRecording );
		CHECK_ERR( task.dstImage );
		
		task.dstImage->AddUsage( EImageUsage::TransferDst );

		return _taskGraph.Add( task );
	}
	
/*
=================================================
	AddTask (UpdateBuffer)
=================================================
*/
	Task  VFrameGraph::AddTask (const UpdateBuffer &task)
	{
		CHECK_ERR( _isRecording );
		CHECK_ERR( task.dstBuffer );

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
			CHECK_ERR( _stagingMngr.StoreBufferData( task.data, readn, OUT copy.srcBuffer, OUT off, OUT size ));

			copy.AddRegion( off, task.offset + readn, size );
			readn += size;

			sync.depends.push_back( AddTask( copy ));
		}

		// return task handle
		if ( sync.depends.size() > 1 )
			return _taskGraph.Add( sync );
		else
			return sync.depends.front();
	}
	
/*
=================================================
	AddTask (UpdateImage)
=================================================
*/
	Task  VFrameGraph::AddTask (const UpdateImage &task)
	{
		CHECK_ERR( _isRecording );
		CHECK_ERR( task.dstImage );
		ASSERT( task.mipmapLevel.Get() < task.dstImage->MipmapLevels() );
		ASSERT( task.arrayLayer.Get() < task.dstImage->ArrayLayers() );
		ASSERT(Any( task.imageSize > uint3(0) ));


		const uint3		image_size		= Max( task.imageSize, 1u );
		const uint		bpp				= EPixelFormat_BitPerPixel( task.dstImage->PixelFormat(), task.aspectMask );
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
				CHECK_ERR( _stagingMngr.StoreImageData( task.data, readn, slice_pitch, total_size,
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
				CHECK_ERR( _stagingMngr.StoreImageData( data, readn, row_pitch, total_size,
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
			return _taskGraph.Add( sync );
		else
			return sync.depends.front();
	}
	
/*
=================================================
	AddTask (ReadBuffer)
=================================================
*/
	Task  VFrameGraph::AddTask (const ReadBuffer &task)
	{
		using DataLoadedEvent = VStagingBufferManager::BufferDataLoadedEvent;

		CHECK_ERR( _isRecording );
		CHECK_ERR( task.srcBuffer and task.callback );
		

		DataLoadedEvent		load_event{ task.callback, task.size };
		TaskGroupSync		sync;

		// copy to staging buffer
		for (BytesU written; written < task.size;)
		{
			CopyBuffer	copy;
			copy.taskName	= task.taskName;
			copy.debugColor	= task.debugColor;
			copy.depends	= task.depends;
			copy.srcBuffer	= task.srcBuffer;

			DataLoadedEvent::Range	range;
			CHECK_ERR( _stagingMngr.AddPendingLoad( written, task.size, OUT copy.dstBuffer, OUT range ));
			
			copy.AddRegion( range.offset, task.offset + written, range.size );
			written += range.size;

			load_event.parts.push_back( range );

			sync.depends.push_back( AddTask( copy ));
		}

		_stagingMngr.AddDataLoadedEvent( std::move(load_event) );
		

		// return task handle
		if ( sync.depends.size() > 1 )
			return _taskGraph.Add( sync );
		else
			return sync.depends.front();
	}
	
/*
=================================================
	AddTask (ReadImage)
=================================================
*/
	Task  VFrameGraph::AddTask (const ReadImage &task)
	{
		using DataLoadedEvent = VStagingBufferManager::ImageDataLoadedEvent;

		CHECK_ERR( _isRecording );
		CHECK_ERR( task.srcImage and task.callback );
		ASSERT( task.mipmapLevel.Get() < task.srcImage->MipmapLevels() );
		ASSERT( task.arrayLayer.Get() < task.srcImage->ArrayLayers() );
		ASSERT(Any( task.imageSize > uint3(0) ));

		
		const uint3			image_size	= Max( task.imageSize, 1u );
		const BytesU		min_size	= 16_Mb;		// TODO
		const uint			bpp			= EPixelFormat_BitPerPixel( task.srcImage->PixelFormat(), task.aspectMask );
		const BytesU		row_pitch	= (image_size.x * BytesU(bpp)) / 8;
		const BytesU		slice_pitch	= row_pitch * image_size.y;
		const BytesU		total_size	= slice_pitch * image_size.z;
		const uint			row_length	= uint((row_pitch * 8) / bpp);
		const uint			img_height	= uint(slice_pitch / row_pitch);

		DataLoadedEvent		load_event{ task.callback, total_size, image_size, row_pitch, slice_pitch, task.srcImage->PixelFormat(), task.aspectMask };
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
		
				DataLoadedEvent::Range	range;
				CHECK_ERR( _stagingMngr.AddPendingLoad( written, total_size, slice_pitch, OUT copy.dstBuffer, OUT range ));
			
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
		
				DataLoadedEvent::Range	range;
				CHECK_ERR( _stagingMngr.AddPendingLoad( written, total_size, row_pitch, OUT copy.dstBuffer, OUT range ));

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

		_stagingMngr.AddDataLoadedEvent( std::move(load_event) );


		// return task handle
		if ( sync.depends.size() > 1 )
			return _taskGraph.Add( sync );
		else
			return sync.depends.front();
	}

/*
=================================================
	AddTask (Present)
=================================================
*/
	Task  VFrameGraph::AddTask (const Present &task)
	{
		CHECK_ERR( _isRecording );
		CHECK_ERR( task.srcImage );
		
		task.srcImage->AddUsage( EImageUsage::ColorAttachment );

		return _taskGraph.Add( task );
	}
	
/*
=================================================
	AddTask (PresentVR)
=================================================
*
	Task  VFrameGraph::AddTask (const PresentVR &task)
	{
		CHECK_ERR( _isRecording );

		return _taskGraph.Add( task );
	}

/*
=================================================
	AddTask (TaskGroupSync)
=================================================
*/
	Task  VFrameGraph::AddTask (const TaskGroupSync &task)
	{
		CHECK_ERR( _isRecording );

		return _taskGraph.Add( task );
	}

/*
=================================================
	AddDrawTask (DrawTask)
=================================================
*/
	void  VFrameGraph::AddDrawTask (RenderPass renderPass, const DrawTask &task)
	{
		CHECK_ERR( _isRecording and renderPass, void() );

		void *	ptr = _drawTaskAllocator.Alloc< FGDrawTask<DrawTask> >();

		static_cast<VLogicalRenderPass *>(renderPass)->AddTask( PlacementNew< FGDrawTask<DrawTask> >(
					ptr,
					task,
					VTaskProcessor::Visit1_DrawTask,
					VTaskProcessor::Visit2_DrawTask ));
	}
	
/*
=================================================
	AddDrawTask (DrawIndexedTask)
=================================================
*/
	void  VFrameGraph::AddDrawTask (RenderPass renderPass, const DrawIndexedTask &task)
	{
		CHECK_ERR( _isRecording and renderPass, void() );
		
		void *	ptr = _drawTaskAllocator.Alloc< FGDrawTask<DrawIndexedTask> >();

		static_cast<VLogicalRenderPass *>(renderPass)->AddTask( PlacementNew< FGDrawTask<DrawIndexedTask> >(
					ptr,
					task,
					VTaskProcessor::Visit1_DrawIndexedTask,
					VTaskProcessor::Visit2_DrawIndexedTask ));
	}


}	// FG

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VStagingBufferManager.h"
#include "VFrameGraphThread.h"
#include "VMemoryManager.h"
#include "VFrameGraphDebugger.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VStagingBufferManager::VStagingBufferManager (VFrameGraphThread &fg) :
		_frameGraph{ fg }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VStagingBufferManager::~VStagingBufferManager ()
	{
		CHECK( _perFrame.empty() );
	}

/*
=================================================
	Initialize
=================================================
*/
	bool VStagingBufferManager::Initialize ()
	{
		CHECK_ERR( _perFrame.empty() );

		_perFrame.resize( _frameGraph.GetRingBufferSize() );
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VStagingBufferManager::Deinitialize ()
	{
		for (auto& frame : _perFrame)
		{
			for (auto& buf : frame.deviceToHost) {
				_frameGraph.ReleaseResource( buf.bufferId );
			}

			for (auto& buf : frame.hostToDevice) {
				_frameGraph.ReleaseResource( buf.bufferId );
			}
		}
		_perFrame.clear();
	}

/*
=================================================
	OnBeginFrame
=================================================
*/
	void VStagingBufferManager::OnBeginFrame (const uint frameId, bool isFirst)
	{
		_memoryRanges.Create( _frameGraph.GetAllocator() );
		_memoryRanges->reserve( 64 );

		_frameId = frameId;

		if ( isFirst )
			_OnFirstUsageInFrame();
		else
			_OnNextUsageInFrame();
	}
	
/*
=================================================
	_OnFirstUsageInFrame
=================================================
*/
	void VStagingBufferManager::_OnFirstUsageInFrame ()
	{
		using T = BufferView::value_type;

		auto&	frame	= _perFrame[_frameId];
		auto&	dev		= _frameGraph.GetDevice();

		// map device-to-host staging buffers
		for (auto& buf : frame.deviceToHost)
		{
			// buffer may be recreated on defragmentation pass, so we need to obtain actual pointer every frame
			CHECK( _MapMemory( buf ));
			
			if ( buf.isCoherent or buf.Empty() )
				continue;

			VkMappedMemoryRange	range = {};
			range.sType		= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range.memory	= buf.mem;
			range.offset	= VkDeviceSize(buf.memOffset + buf.offset);
			range.size		= VkDeviceSize(buf.size);
			_memoryRanges->push_back( range );
		}

		// device to host synchronization
		if ( _memoryRanges->size() )
		{
			VK_CALL( dev.vkInvalidateMappedMemoryRanges( dev.GetVkDevice(), uint(_memoryRanges->size()), _memoryRanges->data() ));
			_memoryRanges->clear();
		}


		// trigger buffer events
		for (auto& ev : frame.onBufferLoadedEvents)
		{
			FixedArray< ArrayView<T>, MaxBufferParts >	data_parts;
			BytesU										total_size;

			for (auto& part : ev.parts)
			{
				ArrayView<T>	view{ Cast<T>(part.buffer->mappedPtr + part.offset), size_t(part.size) };

				data_parts.push_back( view );
				total_size += part.size;
			}

			ASSERT( total_size == ev.totalSize );

			ev.callback( BufferView{data_parts} );
		}
		frame.onBufferLoadedEvents.clear();
		

		// trigger image events
		for (auto& ev : frame.onImageLoadedEvents)
		{
			FixedArray< ArrayView<T>, MaxImageParts >	data_parts;
			BytesU										total_size;

			for (auto& part : ev.parts)
			{
				ArrayView<T>	view{ Cast<T>(part.buffer->mappedPtr + part.offset), size_t(part.size) };

				data_parts.push_back( view );
				total_size += part.size;
			}

			ASSERT( total_size == ev.totalSize );

			ev.callback( ImageView{ data_parts, ev.imageSize, ev.rowPitch, ev.slicePitch, ev.format, ev.aspect });
		}
		frame.onImageLoadedEvents.clear();
		

		// map host-to-device staging buffers
		for (auto& buf : frame.hostToDevice)
		{
			buf.offset = buf.size = 0_b;

			// buffer may be recreated on defragmentation pass, so we need to obtain actual pointer every frame
			CHECK( _MapMemory( buf ));
		}
		
		// recycle
		for (auto& buf : frame.deviceToHost)
		{
			buf.offset = buf.size = 0_b;
		}
	}
	
/*
=================================================
	_OnNextUsageInFrame
=================================================
*/
	void VStagingBufferManager::_OnNextUsageInFrame ()
	{
		auto&	frame = _perFrame[_frameId];

		for (auto& buf : frame.hostToDevice)
		{
			buf.offset = buf.size;
		}
	}

/*
=================================================
	OnEndFrame
=================================================
*/
	void VStagingBufferManager::OnEndFrame (bool)
	{
		auto&	frame	= _perFrame[_frameId];
		auto&	dev		= _frameGraph.GetDevice();

		for (auto& buf : frame.hostToDevice)
		{
			if ( buf.isCoherent or buf.Empty() )
				continue;

			VkMappedMemoryRange	range = {};
			range.sType		= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range.memory	= buf.mem;
			range.offset	= VkDeviceSize(buf.memOffset + buf.offset);
			range.size		= VkDeviceSize(buf.size);
			_memoryRanges->push_back( range );
		}

		// host to device synchronization
		if ( _memoryRanges->size() ) {
			VK_CALL( dev.vkFlushMappedMemoryRanges( dev.GetVkDevice(), uint(_memoryRanges->size()), _memoryRanges->data() ));
		}

		_memoryRanges.Destroy();

		/*if ( _frameGraph.GetDebugger() )
		{
			for (auto& buf : frame.hostToDevice)
			{
				if ( buf.size == 0 )
					continue;

				auto*	data = _frameGraph.GetResourceManager()->GetResource( buf.bufferId.Get() );

				_frameGraph.GetDebugger()->AddBufferBarrier(
					data,
					ExeOrderIndex::HostWrite, ExeOrderIndex::Initial,
					VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
					VkBufferMemoryBarrier{
						VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						null,
						VK_ACCESS_HOST_WRITE_BIT, 0,
						VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
						data->Handle(),
						0, VkDeviceSize(buf.size)
					});
			}

			for (auto& buf : frame.deviceToHost)
			{
				if ( buf.size == 0 )
					continue;

				auto*	data = _frameGraph.GetResourceManager()->GetResource( buf.bufferId.Get() );

				_frameGraph.GetDebugger()->AddBufferBarrier(
					data,
					ExeOrderIndex::Last, ExeOrderIndex::HostRead,
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0,
					VkBufferMemoryBarrier{
						VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						null,
						0, VK_ACCESS_HOST_READ_BIT,
						VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
						data->Handle(),
						0, VkDeviceSize(buf.size)
					});
			}
		}*/
	}
	
/*
=================================================
	StorePartialData
=================================================
*/
	bool VStagingBufferManager::StorePartialData (ArrayView<uint8_t> srcData, const BytesU srcOffset,
												  OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size)
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
	StoreSolidData
=================================================
*/
	bool VStagingBufferManager::StoreSolidData (const void *dataPtr, BytesU dataSize, BytesU offsetAlign,
												OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size)
	{
		void *	ptr = null;
		if ( _GetWritable( dataSize, 1_b, offsetAlign, dataSize, OUT dstBuffer, OUT dstOffset, OUT size, OUT ptr ) )
		{
			MemCopy( ptr, size, dataPtr, size );
			return true;
		}
		return false;
	}
	
/*
=================================================
	GetWritableBuffer
=================================================
*/
	bool VStagingBufferManager::GetWritableBuffer (BytesU srcDataSize, BytesU dstMinSize, OUT RawBufferID &dstBuffer,
												   OUT BytesU &dstOffset, OUT BytesU &size, OUT void* &mappedPtr)
	{
		return _GetWritable( srcDataSize, 1_b, 16_b, dstMinSize, OUT dstBuffer, OUT dstOffset, OUT size, OUT mappedPtr );
	}

/*
=================================================
	StoreImageData
=================================================
*/
	bool VStagingBufferManager::StoreImageData (ArrayView<uint8_t> srcData, const BytesU srcOffset, const BytesU srcPitch, const BytesU srcTotalSize,
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
	bool VStagingBufferManager::_GetWritable (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
											  OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &outSize, OUT void* &mappedPtr)
	{
		ASSERT( blockAlign > 0_b and offsetAlign > 0_b );
		ASSERT( dstMinSize == AlignToSmaller( dstMinSize, blockAlign ));

		auto&	staging_buffers = _perFrame[_frameId].hostToDevice;


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
			ASSERT( dstMinSize < _stagingBufferSize );

			BufferID	buf_id = _frameGraph.CreateBuffer( BufferDesc{ _stagingBufferSize, EBufferUsage::TransferSrc | EBufferUsage::Uniform },
														   MemoryDesc{ EMemoryType::HostWrite }, "HostWriteBuffer" );
			CHECK_ERR( buf_id );

			RawMemoryID	mem_id = _frameGraph.GetResourceManager()->GetResource( buf_id.Get() )->GetMemoryID();
			CHECK_ERR( mem_id );

			staging_buffers.push_back({ std::move(buf_id), mem_id, _stagingBufferSize });

			suitable = &staging_buffers.back();
			CHECK( _MapMemory( *suitable ));
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
	bool VStagingBufferManager::_AddPendingLoad (const BytesU srcRequiredSize, const BytesU blockAlign, const BytesU offsetAlign, const BytesU dstMinSize,
												 OUT RawBufferID &dstBuffer, OUT OnBufferDataLoadedEvent::Range &range)
	{
		ASSERT( blockAlign > 0_b and offsetAlign > 0_b );
		ASSERT( dstMinSize == AlignToSmaller( dstMinSize, blockAlign ));

		auto&	staging_buffers = _perFrame[_frameId].deviceToHost;
		

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
			ASSERT( dstMinSize < _stagingBufferSize );

			BufferID	buf_id = _frameGraph.CreateBuffer( BufferDesc{ _stagingBufferSize, EBufferUsage::TransferDst },
														   MemoryDesc{ EMemoryType::HostRead }, "HostReadBuffer" );
			CHECK_ERR( buf_id );
			
			RawMemoryID	mem_id = _frameGraph.GetResourceManager()->GetResource( buf_id.Get() )->GetMemoryID();
			CHECK_ERR( mem_id );

			staging_buffers.push_back({ std::move(buf_id), mem_id, _stagingBufferSize });

			suitable = &staging_buffers.back();
			CHECK( _MapMemory( *suitable ));
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
	AddPendingLoad
=================================================
*/
	bool VStagingBufferManager::AddPendingLoad (const BytesU srcOffset, const BytesU srcTotalSize,
												OUT RawBufferID &dstBuffer, OUT OnBufferDataLoadedEvent::Range &range)
	{
		// skip blocks less than 1/N of data size
		const BytesU	min_size = (srcTotalSize + MaxBufferParts-1) / MaxBufferParts;

		return _AddPendingLoad( srcTotalSize - srcOffset, 1_b, 16_b, min_size, OUT dstBuffer, OUT range );
	}

/*
=================================================
	AddDataLoadedEvent
=================================================
*/
	bool VStagingBufferManager::AddDataLoadedEvent (OnBufferDataLoadedEvent &&ev)
	{
		CHECK_ERR( ev.callback and not ev.parts.empty() );

		_perFrame[_frameId].onBufferLoadedEvents.push_back( std::move(ev) );
		return true;
	}
	
/*
=================================================
	AddPendingLoad
=================================================
*/
	bool VStagingBufferManager::AddPendingLoad (const BytesU srcOffset, const BytesU srcTotalSize, const BytesU srcPitch,
												OUT RawBufferID &dstBuffer, OUT OnImageDataLoadedEvent::Range &range)
	{
		// skip blocks less than 1/N of total data size
		const BytesU	min_size = Max( (srcTotalSize + MaxImageParts-1) / MaxImageParts, srcPitch );

		return _AddPendingLoad( srcTotalSize - srcOffset, srcPitch, 16_b, min_size, OUT dstBuffer, OUT range );
	}

/*
=================================================
	AddDataLoadedEvent
=================================================
*/
	bool VStagingBufferManager::AddDataLoadedEvent (OnImageDataLoadedEvent &&ev)
	{
		CHECK_ERR( ev.callback and not ev.parts.empty() );

		_perFrame[_frameId].onImageLoadedEvents.push_back( std::move(ev) );
		return true;
	}
	
/*
=================================================
	_MapMemory
=================================================
*/
	bool VStagingBufferManager::_MapMemory (StagingBuffer &buf) const
	{
		VMemoryObj::MemoryInfo	info;
		if ( _frameGraph.GetResourceManager()->GetResource( buf.memoryId )->GetInfo( OUT info ) )
		{
			buf.mappedPtr	= info.mappedPtr;
			buf.memOffset	= info.offset;
			buf.mem			= info.mem;
			buf.isCoherent	= EnumEq( info.flags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
			return true;
		}
		return false;
	}


}	// FG

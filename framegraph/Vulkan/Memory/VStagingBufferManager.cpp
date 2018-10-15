// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VStagingBufferManager.h"
#include "VFrameGraph.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VStagingBufferManager::VStagingBufferManager (VFrameGraph &fg) :
		_frameGraph{ fg },
		_currFrame{ ~0u }
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
	bool VStagingBufferManager::Initialize (const uint swapchainLength)
	{
		CHECK_ERR( _perFrame.empty() );

		_perFrame.resize( swapchainLength );
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VStagingBufferManager::Deinitialize ()
	{
		_perFrame.clear();
	}

/*
=================================================
	OnBeginFrame
=================================================
*/
	void VStagingBufferManager::OnBeginFrame (const uint frameIdx)
	{
		using T = BufferView::value_type;

		_currFrame = frameIdx;

		auto&	frame = _perFrame[_currFrame];

		// map device to host staging buffers
		for (auto& buf : frame.deviceToHost)
		{
			// pipeline barrier doesn't needed here becouse of waiting fence
			//buf.buffer->AddPendingState({ EResourceState::HostRead, 0, VkDeviceSize(buf.size), firstTask });

			if ( not buf.Empty() ) {
				CHECK( _frameGraph.GetMemoryManager().MapMemory( OUT buf.mappedPtr, buf.buffer->GetMemory(), EMemoryMapFlags::Read, {{ 0_b, buf.size }} ));
			}
		}

		// trigger buffer events
		for (auto& ev : frame.bufferLoadEvents)
		{
			FixedArray< ArrayView<T>, MaxBufferParts >	data_parts;
			BytesU										total_size;

			for (auto& part : ev.parts)
			{
				ArrayView<T>	view{ static_cast<T*>(part.buffer->mappedPtr) + part.offset, size_t(part.size) };

				data_parts.push_back( view );
				total_size += part.size;
			}

			ASSERT( total_size == ev.totalSize );

			ev.callback( BufferView{data_parts} );
		}
		frame.bufferLoadEvents.clear();
		

		// trigger image events
		for (auto& ev : frame.imageLoadEvents)
		{
			FixedArray< ArrayView<T>, MaxImageParts >	data_parts;
			BytesU										total_size;

			for (auto& part : ev.parts)
			{
				ArrayView<T>	view{ static_cast<T*>(part.buffer->mappedPtr) + part.offset, size_t(part.size) };

				data_parts.push_back( view );
				total_size += part.size;
			}

			ASSERT( total_size == ev.totalSize );

			ev.callback( ImageView{ data_parts, ev.imageSize, ev.rowPitch, ev.slicePitch, ev.format, ev.aspect });
		}
		frame.imageLoadEvents.clear();

		
		// free staging buffers
		for (auto& buf : frame.hostToDevice)
		{
			//buf.buffer->MakeImmutable( false );	// TODO
			CHECK( _frameGraph.GetMemoryManager().MapMemory( OUT buf.mappedPtr, buf.buffer->GetMemory(), EMemoryMapFlags::WriteDiscard ));
		}

		for (auto& buf : frame.deviceToHost)
		{
			if ( buf.mappedPtr ) {
				CHECK( _frameGraph.GetMemoryManager().UnmapMemory( buf.buffer->GetMemory() ));
			}

			buf.size		= 0_b;
			buf.mappedPtr	= null;
		}
	}
	
/*
=================================================
	OnEndFrame
=================================================
*/
	void VStagingBufferManager::OnEndFrame (Task firstTask)
	{
		auto&	frame = _perFrame[_currFrame];

		// flush host to device staging buffers
		for (auto& buf : frame.hostToDevice)
		{
			if ( buf.mappedPtr ) {
				CHECK( _frameGraph.GetMemoryManager().UnmapMemory( buf.buffer->GetMemory(), {{0_b, buf.size}} ));
			}
			
			// pipeline barrier doesn't needed here becouse of waiting fence
			//buf.buffer->AddPendingState({ EResourceState::HostWrite, 0, VkDeviceSize(buf.size), firstTask });
			//buf.buffer->MakeImmutable( true );

			buf.size		= 0_b;
			buf.mappedPtr	= null;
		}
	}
	
/*
=================================================
	StoreBufferData
=================================================
*/
	bool VStagingBufferManager::StoreBufferData (ArrayView<uint8_t> srcData, const BytesU srcOffset,
												 OUT BufferPtr &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size)
	{
		// skip blocks less than 1/N of data size
		const BytesU	min_size = (ArraySizeOf(srcData) + MaxBufferParts-1) / MaxBufferParts;

		return _StoreData( srcData, srcOffset, 0_b, min_size, OUT dstBuffer, OUT dstOffset, OUT size );
	}

/*
=================================================
	StoreImageData
=================================================
*/
	bool VStagingBufferManager::StoreImageData (ArrayView<uint8_t> srcData, const BytesU srcOffset, const BytesU srcPitch, const BytesU srcTotalSize,
												OUT BufferPtr &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size)
	{
		// skip blocks less than 1/N of total data size
		const BytesU	min_size = Max( (srcTotalSize + MaxImageParts-1) / MaxImageParts, srcPitch );

		return _StoreData( srcData, srcOffset, srcPitch, srcPitch, OUT dstBuffer, OUT dstOffset, OUT size );
	}

/*
=================================================
	_StoreData
=================================================
*/
	bool VStagingBufferManager::_StoreData (ArrayView<uint8_t> srcData, const BytesU srcOffset, const BytesU srcAlign, const BytesU srcMinSize,
											OUT BufferPtr &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size)
	{
		const BytesU	required		= ArraySizeOf(srcData) - srcOffset;
		auto&			staging_buffers = _perFrame[_currFrame].hostToDevice;


		// search in existing
		StagingBuffer*	suitable		= null;
		StagingBuffer*	max_available	= null;

		for (auto& buf : staging_buffers)
		{
			const BytesU	av = buf.Available();

			if ( av >= required )
			{
				suitable = &buf;
				break;
			}

			if ( not max_available or av > max_available->Available() )
			{
				max_available = &buf;
			}
		}

		// no suitable space, try to use max available block
		if ( not suitable and max_available and max_available->Available() >= srcMinSize )
		{
			suitable = max_available;
		}

		// allocate new buffer
		if ( not suitable )
		{
			VBufferPtr	buf;
			CHECK_ERR( buf = Cast<VBuffer>(_frameGraph.CreateBuffer( EMemoryType::HostWrite, _stagingBufferSize, EBufferUsage::Transfer )) );

			void *		ptr = null;
			CHECK_ERR( _frameGraph.GetMemoryManager().MapMemory( OUT ptr, buf->GetMemory(), EMemoryMapFlags::WriteDiscard ));

			buf->SetDebugName( "StagingWBuffer"s << ToString(staging_buffers.size()) << '/' << ToString(_currFrame) );

			staging_buffers.push_back({ buf, ptr });

			suitable = &staging_buffers.back();
		}

		// write data to buffer
		dstOffset	= suitable->size;
		size		= Min( suitable->Available(), required );
		dstBuffer	= suitable->buffer;

		if ( srcAlign > 1_b )
			size = AlignToSmaller( size, srcAlign );

		MemCopy( suitable->mappedPtr, suitable->Available(), srcData.data() + srcOffset, size );

		suitable->size += size;
		return true;
	}
	
/*
=================================================
	_AddPendingLoad
=================================================
*/
	bool VStagingBufferManager::_AddPendingLoad (const BytesU srcRequiredSize, const BytesU srcAlign, const BytesU srcMinSize,
												 OUT BufferPtr &dstBuffer, OUT BufferDataLoadedEvent::Range &range)
	{
		auto&	staging_buffers = _perFrame[_currFrame].deviceToHost;
		

		// search in existing
		StagingBuffer*	suitable		= null;
		StagingBuffer*	max_available	= null;

		for (auto& buf : staging_buffers)
		{
			const BytesU	av = buf.Available();

			if ( av >= srcRequiredSize )
			{
				suitable = &buf;
				break;
			}

			if ( not max_available or av > max_available->Available() )
			{
				max_available = &buf;
			}
		}

		// no suitable space, try to use max available block
		if ( not suitable and max_available and max_available->Available() >= srcMinSize )
		{
			suitable = max_available;
		}

		// allocate new buffer
		if ( not suitable )
		{
			VBufferPtr	buf;
			CHECK_ERR( buf = Cast<VBuffer>(_frameGraph.CreateBuffer( EMemoryType::HostRead, _stagingBufferSize, EBufferUsage::Transfer )) );

			buf->SetDebugName( "StagingRBuffer"s << ToString(staging_buffers.size()) << '/' << ToString(_currFrame) );

			staging_buffers.push_back({ buf, null });

			suitable = &staging_buffers.back();
		}
		
		// write data to buffer
		range.buffer	= suitable;
		range.offset	= suitable->size;
		range.size		= Min( suitable->Available(), srcRequiredSize );
		dstBuffer		= suitable->buffer;
		
		if ( srcAlign > 1_b )
			range.size = AlignToSmaller( range.size, srcAlign );

		suitable->size += range.size;
		return true;
	}
	
/*
=================================================
	AddPendingLoad
=================================================
*/
	bool VStagingBufferManager::AddPendingLoad (const BytesU srcOffset, const BytesU srcTotalSize,
												OUT BufferPtr &dstBuffer, OUT BufferDataLoadedEvent::Range &range)
	{
		// skip blocks less than 1/N of data size
		const BytesU	min_size = (srcTotalSize + MaxBufferParts-1) / MaxBufferParts;

		return _AddPendingLoad( srcTotalSize - srcOffset, 0_b, min_size, OUT dstBuffer, OUT range );
	}

/*
=================================================
	AddDataLoadedEvent
=================================================
*/
	bool VStagingBufferManager::AddDataLoadedEvent (BufferDataLoadedEvent &&ev)
	{
		CHECK_ERR( ev.callback and not ev.parts.empty() );

		_perFrame[_currFrame].bufferLoadEvents.push_back( std::move(ev) );
		return true;
	}
	
/*
=================================================
	AddPendingLoad
=================================================
*/
	bool VStagingBufferManager::AddPendingLoad (const BytesU srcOffset, const BytesU srcTotalSize, const BytesU srcPitch,
												OUT BufferPtr &dstBuffer, OUT ImageDataLoadedEvent::Range &range)
	{
		// skip blocks less than 1/N of total data size
		const BytesU	min_size = Max( (srcTotalSize + MaxImageParts-1) / MaxImageParts, srcPitch );

		return _AddPendingLoad( srcTotalSize - srcOffset, srcPitch, min_size, OUT dstBuffer, OUT range );
	}

/*
=================================================
	AddDataLoadedEvent
=================================================
*/
	bool VStagingBufferManager::AddDataLoadedEvent (ImageDataLoadedEvent &&ev)
	{
		CHECK_ERR( ev.callback and not ev.parts.empty() );

		_perFrame[_currFrame].imageLoadEvents.push_back( std::move(ev) );
		return true;
	}


}	// FG

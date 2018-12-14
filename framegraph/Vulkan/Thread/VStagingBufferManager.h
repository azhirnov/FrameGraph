// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VBuffer.h"
#include "framegraph/Public/FrameGraphTask.h"

namespace FG
{

	//
	// Vulkan Staging Buffer Manager
	//

	class VStagingBufferManager final
	{
	// types
	private:
		static constexpr uint	MaxBufferParts	= 3;
		static constexpr uint	MaxImageParts	= 4;
		static constexpr BytesU	MinBufferPart	= 4_Kb;

		struct StagingBuffer
		{
		// variables
			BufferID		bufferId;
			RawMemoryID		memoryId;
			BytesU			capacity;
			BytesU			offset;
			BytesU			size;
			
			void *			mappedPtr	= null;
			BytesU			memOffset;					// can be used to flush memory ranges
			VkDeviceMemory	mem			= VK_NULL_HANDLE;
			bool			isCoherent	= false;

		// methods
			StagingBuffer () {}

			StagingBuffer (BufferID &&buf, RawMemoryID mem, BytesU capacity) :
				bufferId{std::move(buf)}, memoryId{mem}, capacity{capacity} {}

			ND_ bool	IsFull ()	const	{ return size >= capacity; }
			ND_ bool	Empty ()	const	{ return size == offset; }
		};

		
	public:
		struct OnBufferDataLoadedEvent
		{
		// types
			struct Range
			{
				StagingBuffer const*	buffer;
				BytesU					offset;
				BytesU					size;
			};

			using DataParts_t	= FixedArray< Range, MaxBufferParts >;
			using Callback_t	= ReadBuffer::Callback_t;
			
		// variables
			Callback_t		callback;
			DataParts_t		parts;
			BytesU			totalSize;

		// methods
			OnBufferDataLoadedEvent () {}
			OnBufferDataLoadedEvent (const Callback_t &cb, BytesU size) : callback{cb}, totalSize{size} {}
		};


		struct OnImageDataLoadedEvent
		{
		// types
			using Range			= OnBufferDataLoadedEvent::Range;
			using DataParts_t	= FixedArray< Range, MaxImageParts >;
			using Callback_t	= ReadImage::Callback_t;

		// variables
			Callback_t		callback;
			DataParts_t		parts;
			BytesU			totalSize;
			uint3			imageSize;
			BytesU			rowPitch;
			BytesU			slicePitch;
			EPixelFormat	format		= Default;
			EImageAspect	aspect		= EImageAspect::Color;

		// methods
			OnImageDataLoadedEvent () {}

			OnImageDataLoadedEvent (const Callback_t &cb, BytesU size, const uint3 &imageSize, BytesU rowPitch, BytesU slicePitch, EPixelFormat fmt, EImageAspect asp) :
				callback{cb}, totalSize{size}, imageSize{imageSize}, rowPitch{rowPitch}, slicePitch{slicePitch}, format{fmt}, aspect{asp} {}
		};

		
	private:
		struct PerFrame
		{
			FixedArray< StagingBuffer, 32 >		hostToDevice;	// CPU write, GPU read
			FixedArray< StagingBuffer, 32 >		deviceToHost;	// CPU read, GPU write
			Array< OnBufferDataLoadedEvent >	onBufferLoadedEvents;
			Array< OnImageDataLoadedEvent >		onImageLoadedEvents;
		};

		using PerFrameArray_t	= FixedArray< PerFrame, FG_MaxRingBufferSize >;
		using MemoryRanges_t	= std::vector< VkMappedMemoryRange, StdLinearAllocator<VkMappedMemoryRange> >;


	// variables
	private:
		VFrameGraphThread&			_frameGraph;

		Storage<MemoryRanges_t>		_memoryRanges;

		PerFrameArray_t				_perFrame;
		uint						_frameId			= 0;

		BytesU						_stagingBufferSize	= 16_Mb;


	// methods
	public:
		explicit VStagingBufferManager (VFrameGraphThread &fg);
		~VStagingBufferManager ();

		bool Initialize ();
		void Deinitialize ();

		void OnBeginFrame (uint frameId, bool isFirst);
		void OnEndFrame (bool isFirst);

		bool StoreBufferData (ArrayView<uint8_t> srcData, BytesU srcOffset,
							  OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size);

		bool StoreBufferData (const void *dataPtr, BytesU dataSize, BytesU offsetAlign,
							  OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size);

		bool GetWritableBuffer (BytesU srcDataSize, BytesU dstMinSize, OUT RawBufferID &dstBuffer,
								OUT BytesU &dstOffset, OUT BytesU &size, OUT void* &mappedPtr);
		
		bool StoreImageData (ArrayView<uint8_t> srcData, BytesU srcOffset, BytesU srcPitch, BytesU srcTotalSize,
							 OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size);
		bool AddPendingLoad (BytesU srcOffset, BytesU srcTotalSize, OUT RawBufferID &dstBuffer, OUT OnBufferDataLoadedEvent::Range &range);

		bool AddPendingLoad (BytesU srcOffset, BytesU srcTotalSize, BytesU srcPitch,
							 OUT RawBufferID &dstBuffer, OUT OnImageDataLoadedEvent::Range &range);

		bool AddDataLoadedEvent (OnImageDataLoadedEvent &&ev);
		bool AddDataLoadedEvent (OnBufferDataLoadedEvent &&ev);


	private:
		bool _GetWritable (BytesU srcDataSize, BytesU blockAlign, BytesU offsetAlign, BytesU dstMinSize,
						   OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size, OUT void* &mappedPtr);
		
		bool _AddPendingLoad (BytesU srcRequiredSize, BytesU blockAlign, BytesU offsetAlign, BytesU dstMinSize,
							  OUT RawBufferID &dstBuffer, OUT OnBufferDataLoadedEvent::Range &range);

		bool _MapMemory (StagingBuffer &buf) const;

		void _OnFirstUsageInFrame ();
		void _OnNextUsageInFrame ();
	};


}	// FG

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

		struct StagingBuffer
		{
		// variables
			BufferID		bufferId;
			RawMemoryID		memoryId;
			BytesU			capacity;
			BytesU			size;
			
			void *			mappedPtr	= null;
			BytesU			memOffset;					// can be used to flush memory ranges
			VkDeviceMemory	mem			= VK_NULL_HANDLE;

		// methods
			StagingBuffer () {}

			StagingBuffer (BufferID &&buf, RawMemoryID mem, BytesU capacity) :
				bufferId{std::move(buf)}, memoryId{mem}, capacity{capacity} {}

			ND_ BytesU	Capacity ()		const	{ return capacity; }
			ND_ BytesU	Available ()	const	{ return Capacity() - size; }
			ND_ bool	IsFull ()		const	{ return size >= Capacity(); }
			ND_ bool	Empty ()		const	{ return size == 0_b; }
		};

		
	public:
		struct BufferDataLoadedEvent
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
			BufferDataLoadedEvent () {}
			BufferDataLoadedEvent (const Callback_t &cb, BytesU size) : callback{cb}, totalSize{size} {}
		};


		struct ImageDataLoadedEvent
		{
		// types
			using Range			= BufferDataLoadedEvent::Range;
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
			ImageDataLoadedEvent () {}

			ImageDataLoadedEvent (const Callback_t &cb, BytesU size, const uint3 &imageSize, BytesU rowPitch, BytesU slicePitch, EPixelFormat fmt, EImageAspect asp) :
				callback{cb}, totalSize{size}, imageSize{imageSize}, rowPitch{rowPitch}, slicePitch{slicePitch}, format{fmt}, aspect{asp} {}
		};

		
	private:
		struct PerFrame
		{
			FixedArray< StagingBuffer, 32 >		hostToDevice;	// CPU write, GPU read
			FixedArray< StagingBuffer, 32 >		deviceToHost;	// CPU read, GPU write
			Array< BufferDataLoadedEvent >		bufferLoadEvents;
			Array< ImageDataLoadedEvent >		imageLoadEvents;
		};

		using PerFrameArray_t	= FixedArray< PerFrame, FG_MaxSwapchainLength >;


	// variables
	private:
		VFrameGraphThread&	_frameGraph;

		PerFrameArray_t		_perFrame;
		uint				_currFrame;

		BytesU				_stagingBufferSize	= 16_Mb;


	// methods
	public:
		explicit VStagingBufferManager (VFrameGraphThread &fg);
		~VStagingBufferManager ();

		bool Initialize ();
		void Deinitialize ();

		void OnBeginFrame (uint frameIdx);
		void OnEndFrame ();

		bool StoreBufferData (ArrayView<uint8_t> srcData, BytesU srcOffset,
							  OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size);
		
		bool StoreImageData (ArrayView<uint8_t> srcData, BytesU srcOffset, BytesU srcPitch, BytesU srcTotalSize,
							 OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size);
		bool AddPendingLoad (BytesU srcOffset, BytesU srcTotalSize, OUT RawBufferID &dstBuffer, OUT BufferDataLoadedEvent::Range &range);

		bool AddPendingLoad (BytesU srcOffset, BytesU srcTotalSize, BytesU srcPitch,
							 OUT RawBufferID &dstBuffer, OUT ImageDataLoadedEvent::Range &range);

		bool AddDataLoadedEvent (ImageDataLoadedEvent &&ev);
		bool AddDataLoadedEvent (BufferDataLoadedEvent &&ev);


	private:
		bool _StoreData (ArrayView<uint8_t> srcData, BytesU srcOffset, BytesU srcAlign, BytesU srcMinSize,
						 OUT RawBufferID &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size);
		
		bool _AddPendingLoad (BytesU srcRequiredSize, BytesU srcAlign, BytesU srcMinSize,
							  OUT RawBufferID &dstBuffer, OUT BufferDataLoadedEvent::Range &range);

		bool _MapMemory (StagingBuffer &buf) const;
	};


}	// FG

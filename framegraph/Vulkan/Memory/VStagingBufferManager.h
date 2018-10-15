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
			VBufferPtr	buffer;
			void *		mappedPtr;
			BytesU		size;

		// methods
			StagingBuffer () : mappedPtr{null} {}
			StagingBuffer (const VBufferPtr &buf, void *ptr) : buffer{buf}, mappedPtr{ptr} {}

			ND_ BytesU	Capacity ()		const	{ return buffer->Description().size; }
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
		VFrameGraph &		_frameGraph;

		PerFrameArray_t		_perFrame;
		uint				_currFrame;

		BytesU				_stagingBufferSize	= 16_Mb;


	// methods
	public:
		explicit VStagingBufferManager (VFrameGraph &fg);
		~VStagingBufferManager ();

		bool Initialize (uint swapchainLength);
		void Deinitialize ();

		void OnBeginFrame (uint frameIdx);
		void OnEndFrame (Task firstTask);

		bool StoreBufferData (ArrayView<uint8_t> srcData, BytesU srcOffset,
							  OUT BufferPtr &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size);
		
		bool StoreImageData (ArrayView<uint8_t> srcData, BytesU srcOffset, BytesU srcPitch, BytesU srcTotalSize,
							 OUT BufferPtr &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size);
		bool AddPendingLoad (BytesU srcOffset, BytesU srcTotalSize, OUT BufferPtr &dstBuffer, OUT BufferDataLoadedEvent::Range &range);

		bool AddPendingLoad (BytesU srcOffset, BytesU srcTotalSize, BytesU srcPitch,
							 OUT BufferPtr &dstBuffer, OUT ImageDataLoadedEvent::Range &range);

		bool AddDataLoadedEvent (ImageDataLoadedEvent &&ev);
		bool AddDataLoadedEvent (BufferDataLoadedEvent &&ev);


	private:
		bool _StoreData (ArrayView<uint8_t> srcData, BytesU srcOffset, BytesU srcAlign, BytesU srcMinSize,
						 OUT BufferPtr &dstBuffer, OUT BytesU &dstOffset, OUT BytesU &size);
		
		bool _AddPendingLoad (BytesU srcRequiredSize, BytesU srcAlign, BytesU srcMinSize,
							  OUT BufferPtr &dstBuffer, OUT BufferDataLoadedEvent::Range &range);
	};


}	// FG

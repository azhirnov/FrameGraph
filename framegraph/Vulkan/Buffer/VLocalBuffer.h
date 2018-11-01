// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/EResourceState.h"
#include "framegraph/Shared/ResourceDataRange.h"
#include "VBuffer.h"

namespace FG
{

	//
	// Vulkan Buffer thread local
	//

	class VLocalBuffer final : public ResourceBase
	{
		friend class VBufferUnitTest;

	// types
	public:
		using BufferRange = ResourceDataRange< VkDeviceSize >;
		
		struct BufferState
		{
		// variables
			EResourceState	state;
			BufferRange		range;
			Task			task;

		// methods
			BufferState () {}

			BufferState (EResourceState state, VkDeviceSize begin, VkDeviceSize end, Task task) :
				state{state}, range{begin, end}, task{task} {}
		};

	private:
		struct BufferBarrier
		{
		// variables
			BufferRange				range;
			VkPipelineStageFlags	stages		= 0;
			VkAccessFlags			access		= 0;
			ExeOrderIndex			index		= ExeOrderIndex::Initial;
			bool					isReadable : 1;
			bool					isWritable : 1;

		// methods
			BufferBarrier () : isReadable{false}, isWritable{false} {}
		};

		using BarrierArray_t	= Array< BufferBarrier >;	// TODO: fixed size array or custom allocator


	// variables
	private:
		VBuffer const*			_bufferData		= null;

		mutable BarrierArray_t	_pendingBarriers;
		mutable BarrierArray_t	_writeBarriers;
		mutable BarrierArray_t	_readBarriers;
		

	// methods
	public:
		VLocalBuffer () {}
		~VLocalBuffer ();

		bool Create (const VBuffer *);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);

		void AddPendingState (const BufferState &state) const;
		void CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger = null) const;

		ND_ bool				IsCreated ()	const	{ return _bufferData != null; }
		ND_ VkBuffer			Handle ()		const	{ return _bufferData->Handle(); }
		ND_ BufferDesc const&	Description ()	const	{ return _bufferData->Description(); }
		ND_ BytesU				Size ()			const	{ return Description().size; }


	private:
		ND_ static BarrierArray_t::iterator	_FindFirstBarrier (BarrierArray_t &arr, const BufferRange &range);
			static void						_ReplaceBarrier (BarrierArray_t &arr, BarrierArray_t::iterator iter, const BufferBarrier &barrier);
			static BarrierArray_t::iterator	_EraseBarriers (BarrierArray_t &arr, BarrierArray_t::iterator iter, const BufferRange &range);
	};


}	// FG

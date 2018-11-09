// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/EResourceState.h"
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
			EResourceState	state		= Default;
			BufferRange		range;
			Task			task		= null;

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
		void CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const;

		ND_ bool				IsCreated ()	const	{ SHAREDLOCK( _rcCheck );  return _bufferData != null; }
		ND_ VkBuffer			Handle ()		const	{ SHAREDLOCK( _rcCheck );  return _bufferData->Handle(); }
		ND_ VBuffer const*		ToGlobal ()		const	{ SHAREDLOCK( _rcCheck );  return _bufferData; }

		ND_ BufferDesc const&	Description ()	const	{ SHAREDLOCK( _rcCheck );  return _bufferData->Description(); }
		ND_ BytesU				Size ()			const	{ SHAREDLOCK( _rcCheck );  return Description().size; }


	private:
		ND_ static BarrierArray_t::iterator	_FindFirstBarrier (BarrierArray_t &arr, const BufferRange &range);
			static void						_ReplaceBarrier (BarrierArray_t &arr, BarrierArray_t::iterator iter, const BufferBarrier &barrier);
			static BarrierArray_t::iterator	_EraseBarriers (BarrierArray_t &arr, BarrierArray_t::iterator iter, const BufferRange &range);
	};


}	// FG

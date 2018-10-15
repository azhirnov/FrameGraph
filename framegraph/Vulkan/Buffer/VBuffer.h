// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/BufferResource.h"
#include "framegraph/Shared/ResourceDataRange.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Buffer
	//

	class VBuffer final : public BufferResource
	{
		friend class VFrameGraph;
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

		using BarrierArray_t	= Array< BufferBarrier >;	// TODO: fixed size array
		using DebugName_t		= StaticString<64>;


	// variables
	private:
		VkBuffer			_bufferId;
		VMemoryHandle		_memoryId;

		BarrierArray_t		_pendingBarriers;
		BarrierArray_t		_writeBarriers;
		BarrierArray_t		_readBarriers;

		VFrameGraphWeak		_frameGraph;
		DebugName_t			_debugName;


	// methods
	private:
		bool _CreateBuffer (const Description_t &desc, EMemoryTypeExt memType, VMemoryManager &alloc);
		void _Destroy ();

		ND_ static BarrierArray_t::iterator	_FindFirstBarrier (BarrierArray_t &arr, const BufferRange &range);
			static void						_ReplaceBarrier (BarrierArray_t &arr, BarrierArray_t::iterator iter, const BufferBarrier &barrier);
			static BarrierArray_t::iterator	_EraseBarriers (BarrierArray_t &arr, BarrierArray_t::iterator iter, const BufferRange &range);


	public:
		explicit VBuffer (const VFrameGraphWeak &fg);
		~VBuffer ();

		void AddPendingState (const BufferState &state);
		void CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger = null);
		
		void SetDebugName (StringView name) FG_OVERRIDE;
		bool MakeImmutable (bool immutable) FG_OVERRIDE;

		ND_ BufferPtr		GetReal (Task, EResourceState)	FG_OVERRIDE		{ return shared_from_this(); }
		ND_ VkBuffer		GetBufferID ()					const			{ return _bufferId; }
		ND_ VMemoryHandle	GetMemory ()					const			{ return _memoryId; }
		ND_ StringView		GetDebugName ()					const			{ return _debugName; }
		ND_ VBufferPtr		GetSelfShared ()								{ return std::static_pointer_cast<VBuffer>(shared_from_this()); }
	};

	
/*
=================================================
	Cast
=================================================
*/
	template <>
	ND_ forceinline VBufferPtr  Cast<VBuffer, BufferResource> (const BufferPtr &other)
	{
#		ifdef FG_DEBUG
			ASSERT( not other->IsLogical() );	// for logical resource use GetReal()
			return std::dynamic_pointer_cast<VBuffer>( other );
#		else
			return std::static_pointer_cast<VBuffer>( other );
#		endif
	}


}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/EResourceState.h"
#include "framegraph/Shared/ResourceDataRange.h"
#include "VBuffer.h"

namespace FG
{

	//
	// Vulkan Buffer thread local
	//

	class VLocalBuffer final
	{
		friend class VBufferUnitTest;

	// types
	public:
		using BufferRange = ResourceDataRange< VkDeviceSize >;
		
		struct BufferState
		{
		// variables
			EResourceState	state	= Default;
			BufferRange		range;
			VTask			task;

		// methods
			BufferState () {}

			BufferState (EResourceState state, VkDeviceSize begin, VkDeviceSize end, VTask task) :
				state{state}, range{begin, end}, task{task} {}
		};

	private:
		struct BufferAccess
		{
		// variables
			BufferRange				range;
			VkPipelineStageFlags	stages		= 0;
			VkAccessFlags			access		= 0;
			ExeOrderIndex			index		= ExeOrderIndex::Initial;
			bool					isReadable : 1;
			bool					isWritable : 1;

		// methods
			BufferAccess () : isReadable{false}, isWritable{false} {}
		};

		using AccessRecords_t	= Array< BufferAccess >;	// TODO: fixed size array or custom allocator
		using AccessIter_t		= AccessRecords_t::iterator;


	// variables
	private:
		Ptr<VBuffer const>			_bufferData;	// readonly access is thread safe

		mutable AccessRecords_t		_pendingAccesses;
		mutable AccessRecords_t		_accessForWrite;
		mutable AccessRecords_t		_accessForRead;
		mutable bool				_isImmutable	= false;


	// methods
	public:
		VLocalBuffer () {}
		VLocalBuffer (VLocalBuffer &&) = delete;
		~VLocalBuffer ();

		bool Create (const VBuffer *);
		void Destroy ();
		
		void SetInitialState (bool immutable) const;
		void AddPendingState (const BufferState &state) const;
		void ResetState (ExeOrderIndex index, VBarrierManager &barrierMngr, Ptr<VLocalDebugger> debugger) const;
		void CommitBarrier (VBarrierManager &barrierMngr, Ptr<VLocalDebugger> debugger) const;

		ND_ bool				IsCreated ()	const	{ return _bufferData != null; }
		ND_ VkBuffer			Handle ()		const	{ return _bufferData->Handle(); }
		ND_ VBuffer const*		ToGlobal ()		const	{ return _bufferData.get(); }

		ND_ BufferDesc const&	Description ()	const	{ return _bufferData->Description(); }
		ND_ BytesU				Size ()			const	{ return Description().size; }
		
		ND_ StringView			GetDebugName ()	const	{ return _bufferData->GetDebugName(); }


	private:
		ND_ static AccessIter_t	_FindFirstAccess (AccessRecords_t &arr, const BufferRange &range);
			static void			_ReplaceAccessRecords (INOUT AccessRecords_t &arr, AccessIter_t iter, const BufferAccess &barrier);
			static AccessIter_t	_EraseAccessRecords (INOUT AccessRecords_t &arr, AccessIter_t iter, const BufferRange &range);
	};


}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VLocalBuffer.h"
#include "VEnumCast.h"
#include "VTaskGraph.h"
#include "VBarrierManager.h"
#include "VFrameGraphDebugger.h"

namespace FG
{

/*
=================================================
	destructor
=================================================
*/
	VLocalBuffer::~VLocalBuffer ()
	{
		ASSERT( _bufferData == null );
	}

/*
=================================================
	Create
=================================================
*/
	bool VLocalBuffer::Create (const VBuffer *bufferData)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _bufferData == null );
		CHECK_ERR( bufferData );

		_bufferData		= bufferData;
		_isFirstBarrier	= true;
		_isImmutable	= _bufferData->IsReadOnly();

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VLocalBuffer::Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t)
	{
		SCOPELOCK( _rcCheck );

		_bufferData	= null;

		// check for uncommited barriers
		ASSERT( _pendingAccesses.empty() );
		ASSERT( _accessForWrite.empty() );
		ASSERT( _accessForRead.empty() );

		_pendingAccesses.clear();
		_accessForWrite.clear();
		_accessForRead.clear();
	}

/*
=================================================
	_FindFirstAccess
=================================================
*/
	inline VLocalBuffer::AccessIter_t
		VLocalBuffer::_FindFirstAccess (AccessRecords_t &arr, const BufferRange &otherRange)
	{
		auto iter = arr.begin();
		auto end  = arr.end();

		for (; iter != end and iter->range.end <= otherRange.begin; ++iter)
		{}
		return iter;
	}
	
/*
=================================================
	_ReplaceAccessRecords
=================================================
*/
	inline void VLocalBuffer::_ReplaceAccessRecords (INOUT AccessRecords_t &arr, AccessIter_t iter, const BufferAccess &barrier)
	{
		ASSERT( iter >= arr.begin() and iter <= arr.end() );

		bool replaced = false;

		for (; iter != arr.end();)
		{
			if ( iter->range.begin <  barrier.range.begin and
				 iter->range.end   <= barrier.range.end )
			{
				//	|1111111|22222|
				//     |bbbbb|			+
				//  |11|....			=
				iter->range.end = barrier.range.begin;
				++iter;
				continue;
			}

			if ( iter->range.begin < barrier.range.begin and
				 iter->range.end   > barrier.range.end )
			{
				//  |111111111111111|
				//      |bbbbb|			+
				//  |111|bbbbb|11111|	=

				const auto	src = *iter;

				iter->range.end = barrier.range.begin;

				iter = arr.insert( iter+1, barrier );
				replaced = true;

				iter = arr.insert( iter+1, src );

				iter->range.begin = barrier.range.end;
				break;
			}

			if ( iter->range.begin >= barrier.range.begin and
				 iter->range.begin <  barrier.range.end )
			{
				if ( iter->range.end > barrier.range.end )
				{
					//  ...|22222222222|
					//   |bbbbbbb|			+
					//  ...bbbbbb|22222|	=
					iter->range.begin = barrier.range.end;

					if ( not replaced )
					{
						arr.insert( iter, barrier );
						replaced = true;
					}
					break;
				}

				if ( replaced )
				{
					//  ...|22222|33333|
					//   |bbbbbbbbbbb|		+
					//  ...|bbbbbbbbb|...	=
					iter = arr.erase( iter );
				}
				else
				{
					*iter = barrier;
					++iter;
					replaced = true;
				}
				continue;
			}

			break;
		}
			
		if ( not replaced )
		{
			arr.insert( iter, barrier );
		}
	}
	
/*
=================================================
	_EraseAccessRecords
=================================================
*/
	inline VLocalBuffer::AccessIter_t
		VLocalBuffer::_EraseAccessRecords (INOUT AccessRecords_t &arr, AccessIter_t iter, const BufferRange &range)
	{
		if ( arr.empty() )
			return iter;

		ASSERT( iter >= arr.begin() and iter <= arr.end() );

		for (; iter != arr.end();)
		{
			if ( iter->range.begin < range.begin and
				 iter->range.end   > range.end )
			{
				const auto	src = *iter;
				iter->range.end = range.begin;

				iter = arr.insert( iter+1, src );
				iter->range.begin = range.end;
				break;
			}

			if ( iter->range.begin < range.begin )
			{
				ASSERT( iter->range.end >= range.begin );

				iter->range.end = range.begin;
				++iter;
				continue;
			}

			if ( iter->range.end > range.end )
			{
				ASSERT( iter->range.begin <= range.end );

				iter->range.begin = range.end;
				break;
			}

			iter = arr.erase( iter );
		}

		return iter;
	}

/*
=================================================
	AddPendingState
=================================================
*/
	void VLocalBuffer::AddPendingState (const BufferState &bs) const
	{
		ASSERT( bs.range.begin < Size() and bs.range.end <= Size() );
		ASSERT( bs.task );
		SCOPELOCK( _rcCheck );

		if ( _isImmutable )
			return;

		BufferAccess		pending;
		pending.range		= bs.range;
		pending.stages		= EResourceState_ToPipelineStages( bs.state );
		pending.access		= EResourceState_ToAccess( bs.state );
		pending.isReadable	= EResourceState_IsReadable( bs.state );
		pending.isWritable	= EResourceState_IsWritable( bs.state );
		pending.index		= bs.task->ExecutionOrder();
		
		
		// merge with pending
		BufferRange	range	= pending.range;
		auto		iter	= _FindFirstAccess( _pendingAccesses, range );

		if ( iter != _pendingAccesses.end() and iter->range.begin > range.begin )
		{
			pending.range = BufferRange{ range.begin, iter->range.begin };
			
			iter = _pendingAccesses.insert( iter, pending );
			++iter;

			range.begin = iter->range.begin;
		}

		for (; iter != _pendingAccesses.end() and iter->range.IsIntersects( range ); ++iter)
		{
			ASSERT( iter->index == pending.index );

			iter->range.begin	= Min( iter->range.begin, range.begin );
			range.begin			= iter->range.end;
							
			iter->stages		|= pending.stages;
			iter->access		|= pending.access;
			iter->isReadable	|= pending.isReadable;
			iter->isWritable	|= pending.isWritable;
		}

		if ( not range.IsEmpty() )
		{
			pending.range = range;
			_pendingAccesses.insert( iter, pending );
		}
	}
	
/*
=================================================
	ResetState
=================================================
*/
	void VLocalBuffer::ResetState (ExeOrderIndex index, VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _pendingAccesses.empty() );	// you must commit all pending states before reseting
		
		if ( _isImmutable )
			return;

		// add full range barrier
		{
			BufferAccess		pending;
			pending.range		= BufferRange{ 0, VkDeviceSize(Size()) };
			pending.stages		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			pending.access		= 0;
			pending.isReadable	= true;
			pending.isWritable	= false;
			pending.index		= index;

			_pendingAccesses.push_back( std::move(pending) );
		}

		CommitBarrier( barrierMngr, debugger );

		// flush
		_accessForWrite.clear();
		_accessForRead.clear();
	}

/*
=================================================
	CommitBarrier
=================================================
*/
	void VLocalBuffer::CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const
	{
		SCOPELOCK( _rcCheck );

		if ( _isImmutable )
			return;

		VkPipelineStageFlags	dst_stages = 0;

		const auto	AddBarrier	= [this, &barrierMngr, debugger, &dst_stages] (const BufferRange &sharedRange, const BufferAccess &src, const BufferAccess &dst)
		{
			if ( not sharedRange.IsEmpty() )
			{
				VkBufferMemoryBarrier	barrier = {};
				barrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				barrier.pNext				= null;
				barrier.buffer				= Handle();
				barrier.offset				= sharedRange.begin;
				barrier.size				= sharedRange.Count();
				barrier.srcAccessMask		= src.access;
				barrier.dstAccessMask		= dst.access;
				barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;

				dst_stages |= dst.stages;
				barrierMngr.AddBufferBarrier( src.stages, dst.stages, 0, barrier );

				if ( debugger )
					debugger->AddBufferBarrier( _bufferData, src.index, dst.index, src.stages, dst.stages, 0, barrier );
			}
		};

		for (const auto& pending : _pendingAccesses)
		{
			const auto	w_iter	= _FindFirstAccess( _accessForWrite, pending.range );
			const auto	r_iter	= _FindFirstAccess( _accessForRead, pending.range );

			if ( pending.isWritable )
			{
				// write -> write, read -> write barriers
				bool	ww_barrier	= true;

				if ( w_iter != _accessForWrite.end() and r_iter != _accessForRead.end() )
				{
					ww_barrier = (w_iter->index >= r_iter->index);
				}

				if ( ww_barrier and w_iter != _accessForWrite.end() )
				{
					// write -> write barrier
					for (auto iter = w_iter; iter != _accessForWrite.end() and iter->range.begin < pending.range.end; ++iter)
					{
						AddBarrier( iter->range.Intersect( pending.range ), *iter, pending );
					}
				}
				else
				if ( r_iter != _accessForRead.end() )
				{
					// read -> write barrier
					for (auto iter = r_iter; iter != _accessForRead.end() and iter->range.begin < pending.range.end; ++iter)
					{
						AddBarrier( iter->range.Intersect( pending.range ), *iter, pending );
					}
				}
				
				// store to '_accessForWrite'
				_ReplaceAccessRecords( _accessForWrite, w_iter, pending );
				_EraseAccessRecords( _accessForRead, r_iter, pending.range );
			}
			else
			{
				// write -> read barrier only
				for (auto iter = w_iter; iter != _accessForWrite.end() and iter->range.begin < pending.range.end; ++iter)
				{
					AddBarrier( iter->range.Intersect( pending.range ), *iter, pending );
				}

				// store to '_accessForRead'
				_ReplaceAccessRecords( _accessForRead, r_iter, pending );
			}
		}

		if ( dst_stages and _isFirstBarrier )
		{
			//barrierMngr.WaitSharedSemaphore( _bufferData->GetSemaphore(), dst_stages );
			_isFirstBarrier = false;
		}

		_pendingAccesses.clear();
	}


}	// FG

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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
		CHECK_ERR( GetState() == EState::Initial );
		CHECK_ERR( _bufferData == null );
		CHECK_ERR( bufferData and bufferData->IsCreated() );

		_bufferData	= bufferData;

		_OnCreate();
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

		_pendingBarriers.clear();
		_writeBarriers.clear();
		_readBarriers.clear();

		_OnDestroy();
	}

/*
=================================================
	_FindFirstBarrier
=================================================
*/
	inline VLocalBuffer::BarrierArray_t::iterator
		VLocalBuffer::_FindFirstBarrier (BarrierArray_t &arr, const BufferRange &otherRange)
	{
		auto iter = arr.begin();
		auto end  = arr.end();

		for (; iter != end and iter->range.end <= otherRange.begin; ++iter)
		{}
		return iter;
	}
	
/*
=================================================
	_ReplaceBarrier
=================================================
*/
	inline void VLocalBuffer::_ReplaceBarrier (BarrierArray_t &arr, BarrierArray_t::iterator iter, const BufferBarrier &barrier)
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
	_EraseBarriers
=================================================
*/
	inline VLocalBuffer::BarrierArray_t::iterator
		VLocalBuffer::_EraseBarriers (BarrierArray_t &arr, BarrierArray_t::iterator iter, const BufferRange &range)
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

		//if ( _isImmutable )
		//	return;


		BufferBarrier		barrier;
		barrier.range		= bs.range;
		barrier.stages		= EResourceState_ToPipelineStages( bs.state );
		barrier.access		= EResourceState_ToAccess( bs.state );
		barrier.isReadable	= EResourceState_IsReadable( bs.state );
		barrier.isWritable	= EResourceState_IsWritable( bs.state );
		barrier.index		= bs.task->ExecutionOrder();
		
		
		// merge with pending
		BufferRange	range	= barrier.range;
		auto		iter	= _FindFirstBarrier( _pendingBarriers, range );

		if ( iter != _pendingBarriers.end() and iter->range.begin > range.begin )
		{
			barrier.range = BufferRange{ range.begin, iter->range.begin };
			
			iter = _pendingBarriers.insert( iter, barrier );
			++iter;

			range.begin = iter->range.begin;
		}

		for (; iter != _pendingBarriers.end() and iter->range.IsIntersects( range ); ++iter)
		{
			ASSERT( iter->index == barrier.index );

			iter->range.begin	= Min( iter->range.begin, range.begin );
			range.begin			= iter->range.end;
							
			iter->stages		|= barrier.stages;
			iter->access		|= barrier.access;
			iter->isReadable	|= barrier.isReadable;
			iter->isWritable	|= barrier.isWritable;
		}

		if ( not range.IsEmpty() )
		{
			barrier.range = range;
			_pendingBarriers.insert( iter, barrier );
		}
	}
	
/*
=================================================
	CommitBarrier
=================================================
*/
	void VLocalBuffer::CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const
	{
		SCOPELOCK( _rcCheck );

		for (const auto& pending : _pendingBarriers)
		{
			const auto	w_iter		= _FindFirstBarrier( _writeBarriers, pending.range );
			const auto	r_iter		= _FindFirstBarrier( _readBarriers, pending.range );

			const auto	AddBarrier	= [this, &barrierMngr, debugger] (const BufferRange &sharedRange, const BufferBarrier &src, const BufferBarrier &dst)
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

					barrierMngr.AddBufferBarrier( src.stages, dst.stages, 0, barrier );

					if ( debugger )
						debugger->AddBufferBarrier( _bufferData, src.index, dst.index, src.stages, dst.stages, 0, barrier );
				}
			};


			if ( pending.isWritable )
			{
				// write -> write, read -> write barriers
				bool	ww_barrier	= true;

				if ( w_iter != _writeBarriers.end() and r_iter != _readBarriers.end() )
				{
					ww_barrier = (w_iter->index >= r_iter->index);
				}

				if ( ww_barrier and w_iter != _writeBarriers.end() )
				{
					// write -> write barrier
					for (auto iter = w_iter; iter != _writeBarriers.end() and iter->range.begin < pending.range.end; ++iter)
					{
						AddBarrier( iter->range.Intersect( pending.range ), *iter, pending );
					}
				}
				else
				if ( r_iter != _readBarriers.end() )
				{
					// read -> write barrier
					for (auto iter = r_iter; iter != _readBarriers.end() and iter->range.begin < pending.range.end; ++iter)
					{
						AddBarrier( iter->range.Intersect( pending.range ), *iter, pending );
					}
				}

				
				// store to '_writeBarriers'
				_ReplaceBarrier( _writeBarriers, w_iter, pending );
				_EraseBarriers( _readBarriers, r_iter, pending.range );
			}
			else
			{
				// write -> read barrier only
				for (auto iter = w_iter; iter != _writeBarriers.end() and iter->range.begin < pending.range.end; ++iter)
				{
					AddBarrier( iter->range.Intersect( pending.range ), *iter, pending );
				}


				// store to '_readBarriers'
				_ReplaceBarrier( _readBarriers, r_iter, pending );
			}
		}

		_pendingBarriers.clear();
	}


}	// FG

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VBuffer.h"
#include "Shared/EnumUtils.h"
#include "VEnumCast.h"
#include "VFrameGraph.h"
#include "VBarrierManager.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VBuffer::VBuffer (const VFrameGraphWeak &fg) :
		_bufferId{ VK_NULL_HANDLE },	_memoryId{ Default },
		_frameGraph{ fg }
	{}

/*
=================================================
	destructor
=================================================
*/
	VBuffer::~VBuffer ()
	{
		_Destroy();
	}
	
/*
=================================================
	_CreateBuffer
=================================================
*/
	bool VBuffer::_CreateBuffer (const Description_t &desc, EMemoryTypeExt memType, VMemoryManager &alloc)
	{
		CHECK_ERR( _bufferId == VK_NULL_HANDLE );
		CHECK_ERR( not _memoryId );
		
		VDevice const&	dev = _frameGraph.lock()->GetDevice();

		_descr = desc;

		// create buffer
		VkBufferCreateInfo	info = {};

		info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.pNext			= null;
		info.flags			= 0;
		info.usage			= VEnumCast( _descr.usage );
		info.size			= VkDeviceSize( _descr.size );
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK( dev.vkCreateBuffer( dev.GetVkDevice(), &info, null, OUT &_bufferId ) );

		CHECK_ERR( alloc.AllocForBuffer( _bufferId, memType, OUT _memoryId ) );

		if ( not _debugName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_bufferId), _debugName, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT );
		}
		return true;
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void VBuffer::_Destroy ()
	{
		VFrameGraphPtr	fg = _frameGraph.lock();

		if ( fg )
		{
			if ( IsExternal() )
			{
				fg->GetDevice().FreeExternalBuffer( _bufferId );
				ASSERT( not _memoryId );
			}
			else
			{
				fg->GetDevice().DeleteBuffer( _bufferId );
				fg->GetMemoryManager().Dealloc( _memoryId );
			}
		}

		_bufferId	= VK_NULL_HANDLE;
		_memoryId	= Default;
		//_descr	= Default;	// keep it for debugging
		_frameGraph.reset();
	}
	
/*
=================================================
	SetDebugName
=================================================
*/
	void VBuffer::SetDebugName (StringView name)
	{
		VFrameGraphPtr	fg = _frameGraph.lock();

		if ( _bufferId and fg )
		{
			fg->GetDevice().SetObjectName( BitCast<uint64_t>(_bufferId), name, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT );
		}

		_debugName = name;
	}
	
/*
=================================================
	MakeImmutable
=================================================
*/
	bool VBuffer::MakeImmutable (bool immutable)
	{
		if ( immutable == _isImmutable )
			return true;

		_isImmutable = immutable;

		if ( _isImmutable )
		{
			// TODO: add whole size barrier to '_pendingBarriers'
			ASSERT(false);
		}
		return true;
	}

/*
=================================================
	_FindFirstBarrier
=================================================
*/
	inline VBuffer::BarrierArray_t::iterator  VBuffer::_FindFirstBarrier (BarrierArray_t &arr, const BufferRange &otherRange)
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
	inline void VBuffer::_ReplaceBarrier (BarrierArray_t &arr, BarrierArray_t::iterator iter, const BufferBarrier &barrier)
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
	inline VBuffer::BarrierArray_t::iterator  VBuffer::_EraseBarriers (BarrierArray_t &arr, BarrierArray_t::iterator iter, const BufferRange &range)
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
	void VBuffer::AddPendingState (const BufferState &bs)
	{
		if ( _isImmutable )
			return;

		ASSERT( bs.range.begin < Size() and bs.range.end <= Size() );
		ASSERT( bs.task );

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
	void VBuffer::CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger)
	{
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
					barrier.buffer				= _bufferId;
					barrier.offset				= sharedRange.begin;
					barrier.size				= sharedRange.Count();
					barrier.srcAccessMask		= src.access;
					barrier.dstAccessMask		= dst.access;
					barrier.srcQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex	= VK_QUEUE_FAMILY_IGNORED;

					barrierMngr.AddBufferBarrier( src.stages, dst.stages, 0, barrier );

					if ( debugger )
						debugger->AddBufferBarrier( this, src.index, dst.index, src.stages, dst.stages, 0, barrier );
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

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Multiple producer, multiple consumer lock-free algorithm.
	Limitiations:
	- call 'Get' and 'Swap' only with your own ID.
	- after call 'Swap' thread has exclusive access to resulting data until the next 'Swap' call with same ID.
	- the maximum number of threads is 15 for 64 bit atomics and 7 for 32 bit atomics.

	The valid usage:
	1. Single producer, single consumer, where first thread continiously write/update/append data
	   and second thread frequently read data.

	2. Async messages/events between multiple threads. Data is a fixed-sized or dynamicly allocated queue or stack.
	   Any thread can append events to the queue. Any thread can remove event for processing or keep it
	   then after swap buffers this event can be processed by another thread.

	available swap pairs:
	thread 1 owns:  [A] \  / [D]      [D]    [D]
	thread 2 owns:  [B]  \/  [B] \  / [A]    [A]
	thread 3 owns:  [C]  /\  [C]  \/  [C] \/ [B]
	unused:         [D] /  \ [A]  /\  [B] /\ [C]


	example:
		constexpr uint						numThreads = 2;
		StaticArray< Value1, numThreads+1 >	values1;		// DOD
		StaticArray< Value2, numThreads+1 >	values2;		// DOD
		LfDoubleBuffer< numThreads >			bufferIndex;

		void thread (uint id)
		{
			uint	i = bufferIndex.Get( id );
			values1[i] = ...
			values2[i] = ...
			i = bufferIndex.Swap( id );
			values1[i] = ...
		}

		void main ()
		{
			for (uint i = 0; i < numThreads; ++i) {
				std::thread  t{ []() { thread(i); };
				t.detach();
			}
		}
*/

#pragma once

#include "stl/CompileTime/Math.h"
#include "stl/Math/BitMath.h"
#include <atomic>
#include <thread>

namespace FG
{


	//
	// Lock-free Double Buffer
	//

	template <uint MaxThreads = 2>
	struct LfDoubleBuffer final
	{
	// types
	public:
		static constexpr uint		MaxItems	= MaxThreads + 1;
		static constexpr uint		Offset		= CT_IntLog2< MaxItems > + uint(not IsPowerOfTwo( MaxItems ));

		using Self			= LfDoubleBuffer< MaxThreads >;
		using Value_t		= Conditional< (Offset * MaxItems > sizeof(uint32_t)*8), uint64_t, uint32_t >;	// TODO: uint128_t support
		using Atomic_t		= std::atomic< Value_t >;

		STATIC_ASSERT( MaxThreads >= 2 );
		STATIC_ASSERT( Offset * MaxItems <= sizeof(Value_t)*8 );
		STATIC_ASSERT( Atomic_t::is_always_lock_free );

		static constexpr Value_t	Mask	= (Value_t(1) << Offset) - 1;


	// variables
	private:
		alignas(FG_CACHE_LINE) Atomic_t		_mode;


	// methods
	public:
		LfDoubleBuffer ()
		{
			Value_t  value = 0;
			for (uint i = 0; i < MaxItems; ++i) {
				value |= i << (Offset * i);
			}
			_mode.store( value, memory_order_release );
		}


		ND_ Value_t  Get (const uint id)
		{
			ASSERT( id < MaxThreads );

			Value_t  index = _mode.load( memory_order_acquire );
			index = (index >> (Offset*id)) & Mask;
			
			return index;
		}


		ND_ Value_t  Swap (const uint id)
		{
			ASSERT( id < MaxThreads );

			const uint		offset1		= Offset * MaxThreads;	// offset of unused data index
			const uint		offset2		= Offset * id;			// offset of current data index
			const uint		delta		= offset1 - offset2;
			const Value_t	mask1		= (Mask << offset1);
			const Value_t	mask2		= (Mask << offset2);
			const Value_t	inv_mask12	= ~(mask1 | mask2);

			Value_t			expected	= _mode.load( memory_order_relaxed );
			Value_t			next		= 0;
			uint			i			= 0;

			do {
				next =	(expected & inv_mask12) |			// remove unused and current
						((expected & mask1) >> delta) |		// move unused to current
						((expected & mask2) << delta);		// move current to unused

				if ( ++i > 1000 ) {
					i = 0;
					std::this_thread::yield();
				}
			}
			while ( not _mode.compare_exchange_weak( INOUT expected, next, memory_order_release, memory_order_relaxed ));

			return (next >> offset2) & Mask;
		}
	};


}	// FG

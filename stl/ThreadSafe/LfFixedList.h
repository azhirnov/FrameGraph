// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/EnumUtils.h"
#include "stl/Math/BitMath.h"
#include <atomic>

namespace FGC
{

	//
	// Lock-free Double-linked List on static memory
	//

	template <typename T, size_t Size>
	struct alignas(FG_CACHE_LINE) LfFixedList
	{
		STATIC_ASSERT( Size < (1u << 10) );

	// types
	public:
		struct Node
		{
			uint	prev  : 10;
			uint	next  : 10;
			uint	curr  : 12;
		};

		static constexpr uint	BitsCount = sizeof(uint64_t) * 8;

		using AtomicArray_t		= StaticArray< Atomic<uint>, Size >;
		using ValueArray_t		= StaticArray< T, Size >;
		using Bitfields_t		= StaticArray< Atomic<uint64_t>, (Size + BitsCount-1) / BitsCount >;


	// variables
	private:
		AtomicArray_t		_nodes;

		Bitfields_t			_bits;		// 1 - is unassigned bit, 0 - assigned bit
		ValueArray_t		_values;


	// methods
	public:
		LfFixedList ()
		{}


		bool Insert (const T &value, OUT uint &outIndex)
		{
			if ( _Assign( OUT outIndex ) )
			{
				_values[outIndex] = value;

				if ( _InsertIndex( outIndex ) )
					return true;

				_Unassign( outIndex );
			}

			outIndex = UMax;
			return false;
		}


		bool Erase (uint index)
		{
		}


		bool Find (const T &value, OUT uint &outIndex)
		{
		}


		ND_ Is


	private:
		bool _Assign (OUT uint &index)
		{
			for (size_t i = 0; i < _bits.size(); ++i)
			{
				auto&		bits	= _bits[i];
				uint64_t	curr	= bits.load( memory_order_relaxed );
				const int	pos		= BitScanForward( curr );

				if ( pos >= 0 )
				{
					// try to lock this bit
					bool		locked;
					uint64_t	mask = (1ull << pos);
					for (;;)
					{
						// change bit from 1 to 0
						locked = bits.compare_exchange_weak( INOUT curr, curr ^ mask, memory_order_release, memory_order_relaxed );

						if ( (curr & mask) == 0 )
							break;	// bit is locked by current thread or other
					}

					if ( not locked )
						continue;

					index = i * BitsCount + pos;
					return true;
				}
			}
			return false;
		}


		void _Unassign (uint index)
		{
			const uint		i		= index / BitsCount;
			const uint		pos		= index % BitsCount;
			auto&			bits	= _bits[i];
			const uint64_t	mask	= (1ull << pos);
			uint64_t		curr	= bits.load( memory_order_acquire );
			
			ASSERT( (curr & mask) == 0 );
			
			// try to unlock this bit
			for (bool unlocked = false; not unlocked;)
			{
				// change bit from 0 to 1
				unlocked = bits.compare_exchange_weak( INOUT curr, curr | mask, memory_order_release, memory_order_relaxed );
				ASSERT( (curr & mask) == 0 );
			}
		}


		bool _InsertIndex (const uint newIndex)
		{
		}
	};


}	// FGC

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/CompileTime/TypeTraits.h"
#include "stl/Containers/FixedArray.h"
#include "stl/CompileTime/Math.h"
#include "stl/ThreadSafe/AtomicPtr.h"
#include "stl/Memory/UntypedAllocator.h"
#include <atomic>

namespace FGC
{

	//
	// Chunked Indexed Pool
	//

	template <typename ValueType,
			  typename IndexType,
			  size_t ChunkSize = 32,
			  size_t MaxChunks = 16,
			  typename AllocatorType = UntypedAlignedAllocator
			 >
	struct LfIndexedPool final
	{
		STATIC_ASSERT( ChunkSize > 0 and MaxChunks > 0 );
		STATIC_ASSERT( IsPowerOfTwo( ChunkSize ) );	// must be power of 2 to increase performance
		
	// types
	public:
		using Self				= LfIndexedPool< ValueType, IndexType, ChunkSize, MaxChunks, AllocatorType >;
		using Index_t			= IndexType;
		using Value_t			= ValueType;
		using Allocator_t		= AllocatorType;

	private:
		using Bitfield_t		= Conditional< (ChunkSize > 64), void, Conditional< (ChunkSize > 32), uint64_t, Conditional< (ChunkSize > 16), uint32_t, uint16_t >>>;
		using BitfieldArray_t	= StaticArray< std::atomic<Bitfield_t>, MaxChunks >;
		
		using ValueChunk_t		= StaticArray< Value_t, ChunkSize >;
		using ValueChunks_t		= StaticArray< std::atomic<ValueChunk_t *>, MaxChunks >;
		
		static constexpr uint	ChunkSizePOT = CT_IntLog2< ChunkSize >;

		STATIC_ASSERT( BitfieldArray_t::value_type::is_always_lock_free );
		STATIC_ASSERT( ValueChunks_t::value_type::is_always_lock_free );


	// variables
	private:
		BitfieldArray_t		_assignedBits;	// 1 - is unassigned bit, 0 - assigned bit
		BitfieldArray_t		_createdBits;	// 1 - constructor has been called
		ValueChunks_t		_values;
		Allocator_t			_alloc;


	// methods
	public:
		LfIndexedPool (const Self &) = delete;
		LfIndexedPool (Self &&) = default;

		Self& operator = (const Self &) = delete;
		Self& operator = (Self &&) = default;
		

		explicit LfIndexedPool (const Allocator_t &alloc = Allocator_t()) :
			_alloc{ alloc }
		{
			for (size_t i = 0; i < MaxChunks; ++i)
			{
				_assignedBits[i].store( UMax, memory_order_relaxed );
				_createdBits[i].store( 0, memory_order_relaxed );
				_values[i].store( null, memory_order_relaxed );
			}

			// flush cache
			std::atomic_thread_fence( memory_order_release );
		}

		~LfIndexedPool()
		{
			Release();
		}
		
		
		template <typename FN>
		void Release (FN &&dtor)
		{
			// invalidate cache
			std::atomic_thread_fence( memory_order_acquire );

			for (size_t i = 0; i < MaxChunks; ++i)
			{
				ValueChunk_t*	value		= _values[i].load( memory_order_relaxed );
				Bitfield_t		ctor_bits	= _createdBits[i].load( memory_order_relaxed );
				Bitfield_t		assigned	= _assignedBits[i].load( memory_order_relaxed );

				FG_UNUSED( assigned );
				ASSERT( assigned == UMax );

				if ( not value )
					continue;
				
				// reset
				_assignedBits[i].store( UMax, memory_order_relaxed );
				_createdBits[i].store( 0, memory_order_relaxed );
				_values[i].store( null, memory_order_relaxed );

				for (size_t j = 0; j < ChunkSize; ++j)
				{
					if ( ctor_bits & (1u<<j) )
						dtor( value->data()[j] );
				}

				_alloc.Deallocate( value, SizeOf<ValueChunk_t>, AlignOf<ValueChunk_t> );
			}
		}

		void Release ()
		{
			return Release([] (Value_t& value) { value.~Value_t(); });
		}


		template <typename FN>
		ND_ bool  Assign (OUT Index_t &outIndex, FN &&ctor)
		{
			for (size_t i = 0; i < MaxChunks; ++i)
			{
				Bitfield_t		bits = _assignedBits[i].load( memory_order_relaxed );
				ValueChunk_t*	ptr  = _values[i].load( memory_order_relaxed );
				
				// allocate
				if ( ptr == null )
				{
					ptr = Cast<ValueChunk_t>(_alloc.Allocate( SizeOf<ValueChunk_t>, AlignOf<ValueChunk_t> ));

					// set new pointer and invalidate cache
					for (ValueChunk_t* expected = null;
						 not _values[i].compare_exchange_weak( INOUT expected, ptr, memory_order_acquire );)
					{
						// another thread has been allocated this chunk
						if ( expected != null and expected != ptr )
						{
							_alloc.Deallocate( ptr, SizeOf<ValueChunk_t>, AlignOf<ValueChunk_t> );
							ptr = expected;
							break;
						}
					}
				}

				// find available index
				for (int index = BitScanForward( bits ); index >= 0;)
				{
					const Bitfield_t	mask = Bitfield_t(1) << index;

					if ( _assignedBits[i].compare_exchange_weak( INOUT bits, bits ^ mask, memory_order_acquire, memory_order_relaxed ))
					{
						outIndex = Index_t(index) | (Index_t(i) << ChunkSizePOT);

						if ( !(_createdBits[i].fetch_or( mask, memory_order_relaxed ) & mask) )
						{
							ctor( &(*ptr)[index], outIndex );
						}
						return true;
					}

					index = BitScanForward( bits );
				}
			}
			return false;
		}
		
		ND_ bool  Assign (OUT Index_t &outIndex)
		{
			return Assign( OUT outIndex, [](Value_t* ptr, Index_t) { PlacementNew<Value_t>( ptr ); });
		}


		void  Unassign (Index_t index)
		{
			const uint	chunk_idx	= index >> ChunkSizePOT;
			const uint	bit_idx		= index & (ChunkSize-1);
			Bitfield_t	mask		= 1 << bit_idx;
			Bitfield_t	old_bits	= _assignedBits[chunk_idx].fetch_or( mask, memory_order_relaxed );	// 0 -> 1

			FG_UNUSED( old_bits );
			ASSERT( !(old_bits & mask) );
		}


		ND_ bool  IsAssigned (Index_t index)
		{
			const uint	chunk_idx	= index >> ChunkSizePOT;
			const uint	bit_idx		= index & (ChunkSize-1);
			Bitfield_t	mask		= 1 << bit_idx;
			Bitfield_t	bits		= _assignedBits[chunk_idx].load( memory_order_relaxed );

			return !(bits & mask);
		}


		ND_ Value_t&  operator [] (Index_t index)
		{
			ASSERT( IsAssigned( index ));

			const uint		chunk_idx	= index >> ChunkSizePOT;
			const uint		bit_idx		= index & (ChunkSize-1);
			ValueChunk_t*	chunk		= _values[chunk_idx].load( memory_order_relaxed );

			ASSERT( chunk );
			return (*chunk)[ bit_idx ];
		}


		ND_ size_t  AssignedBitsCount ()
		{
			size_t	count = 0;
			for (size_t i = 0; i < MaxChunks; ++i)
			{
				Bitfield_t	bits = _assignedBits[i].load( memory_order_relaxed );

				count += BitCount( ~bits );	// count of zeros
			}
			return count;
		}
		
		ND_ size_t  CreatedObjectsCount ()
		{
			size_t	count = 0;
			for (size_t i = 0; i < MaxChunks; ++i)
			{
				Bitfield_t	bits = _createdBits[i].load( memory_order_relaxed );

				count += BitCount( bits );
			}
			return count;
		}


		ND_ static constexpr size_t  Capacity ()
		{
			return ChunkSize * MaxChunks;
		}
	};


}	// FGC

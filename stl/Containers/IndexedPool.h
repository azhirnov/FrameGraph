// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"
#include "stl/Math/Bytes.h"
#include "stl/Memory/UntypedAllocator.h"

namespace FG
{

	//
	// Indexed Pool
	//

	template <typename T,
			  typename IndexType = uint,
			  typename AllocatorType = UntypedAlignedAllocator
			 >
	struct IndexedPool
	{
	// types
	public:
		using Self			= IndexedPool< T, IndexType, AllocatorType >;
		using Index_t		= IndexType;
		using Value_t		= T;
		using Allocator_t	= AllocatorType;


	// variables
	private:
		Index_t *		_freeIndices		= null;
		Value_t *		_values				= null;
		uint			_count				= 0;
		uint			_freeIndicesCount	= 0;
		Allocator_t		_alloc;


	// methods
	public:
		explicit IndexedPool (uint count, const Allocator_t &alloc = Allocator_t());
		explicit IndexedPool (BytesU memSize, const Allocator_t &alloc = Allocator_t());
		IndexedPool () {}
		IndexedPool (const Self &) = delete;
		IndexedPool (Self &&);
		~IndexedPool ();

		Self& operator = (const Self &) = delete;
		Self& operator = (Self &&);

		bool Allocate (uint count);
		void Release ();

		ND_ bool Assign (OUT Index_t &index);
			void Unassign (Index_t index);

		ND_ Value_t &		operator [] (Index_t index);
		ND_ Value_t const&	operator [] (Index_t index) const;

		ND_ bool			Empty ()		const	{ return _freeIndicesCount == _count; }
		ND_ uint			Count ()		const	{ return _count - _freeIndicesCount; }
		ND_ uint			Capacity ()		const	{ return _count; }

		ND_ bool			IsAllocated ()	const	{ return _values != null; }
	};


		
/*
=================================================
	constructor
=================================================
*/
	template <typename T, typename I, typename A>
	inline IndexedPool<T,I,A>::IndexedPool (uint count, const Allocator_t &alloc) :
		_alloc{ alloc }
	{
		CHECK( Allocate( count ));
	}
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T, typename I, typename A>
	inline IndexedPool<T,I,A>::IndexedPool (BytesU memSize, const Allocator_t &alloc) :
		_alloc{ alloc }
	{
		const BytesU	max_align = Max( AlignOf<Value_t>, AlignOf<Index_t> );

		uint	count = uint(memSize / (SizeOf<Index_t> + SizeOf<Value_t>));
		
		for (; count != 0;)
		{
			BytesU	idx_size	= AlignToLarger( SizeOf<Index_t> * count, max_align );
			BytesU	mem_size	= AlignToLarger( SizeOf<Value_t> * count, max_align ) + idx_size;

			if ( mem_size > memSize ) {
				--count;
				continue;
			}
			
			CHECK( Allocate( count ));
			break;
		}
	}
	
/*
=================================================
	move-constructor
=================================================
*/
	template <typename T, typename I, typename A>
	inline IndexedPool<T,I,A>::IndexedPool (Self &&other) :
		_freeIndices{ other._freeIndices },
		_values{ other._values },
		_count{ other._count },
		_freeIndicesCount{ other._freeIndicesCount },
		_alloc{ std::move(other._alloc) }
	{
		other._freeIndices		= null;
		other._values			= null;
		other._count			= 0;
		other._freeIndicesCount	= 0;
	}
	
/*
=================================================
	operator =
=================================================
*/
	template <typename T, typename I, typename A>
	inline IndexedPool<T,I,A>&  IndexedPool<T,I,A>::operator = (IndexedPool<T,I,A> &&rhs)
	{
		Release();
		
		_freeIndicesCount	= rhs._freeIndicesCount;
		_freeIndices		= rhs._freeIndices;
		_values				= rhs._values;
		_count				= rhs._count;
		_alloc				= std::move(rhs._alloc);
		
		rhs._freeIndices		= null;
		rhs._values				= null;
		rhs._count				= 0;
		rhs._freeIndicesCount	= 0;

		return *this;
	}

/*
=================================================
	destructor
=================================================
*/
	template <typename T, typename I, typename A>
	inline IndexedPool<T,I,A>::~IndexedPool ()
	{
		Release();
	}
	
/*
=================================================
	Allocate
=================================================
*/
	template <typename T, typename I, typename A>
	inline bool IndexedPool<T,I,A>::Allocate (uint count)
	{
		if ( _freeIndices or _values )
			return false;

		ASSERT( count > 0 );
		ASSERT( count < std::numeric_limits<Index_t>::max() );

		_freeIndicesCount = _count = count;

		const BytesU	max_align	= Max( AlignOf<Value_t>, AlignOf<Index_t> );
		const BytesU	idx_size	= AlignToLarger( SizeOf<Index_t> * _count, max_align );
		const BytesU	mem_size	= AlignToLarger( SizeOf<Value_t> * _count, max_align ) + idx_size;
		void *			ptr			= _alloc.Allocate( mem_size, max_align );

		if ( not ptr )
			return false;

		_freeIndices	= BitCast< Index_t *>( ptr );
		_values			= BitCast< Value_t *>( ptr + idx_size );
		
		for (uint i = 0; i < _freeIndicesCount; ++i) {
			_freeIndices[i] = Index_t(_freeIndicesCount-1 - i);
		}
		return true;
	}

/*
=================================================
	Release
=================================================
*/
	template <typename T, typename I, typename A>
	inline void IndexedPool<T,I,A>::Release ()
	{
		ASSERT( _freeIndicesCount == _count );	// not all elements was destroyed
		
		const BytesU	max_align = Max( AlignOf<Value_t>, AlignOf<Index_t> );

		_alloc.Deallocate( _freeIndices, max_align );
		
		_freeIndices		= null;
		_values				= null;
		_count				= 0;
		_freeIndicesCount	= 0;
	}
	
/*
=================================================
	Assign
=================================================
*/
	template <typename T, typename I, typename A>
	inline bool IndexedPool<T,I,A>::Assign (OUT Index_t &index)
	{
		if ( _freeIndicesCount == 0 )
			return false;

		index = _freeIndices[ --_freeIndicesCount ];

		PlacementNew<T>( &_values[index] );
		return true;
	}
	
/*
=================================================
	Unassign
=================================================
*/
	template <typename T, typename I, typename A>
	inline void IndexedPool<T,I,A>::Unassign (Index_t index)
	{
		ASSERT( index < _count );
		ASSERT( _freeIndicesCount < _count );

		_values[index].~T();

		_freeIndices[ _freeIndicesCount++ ] = index;
	}
	
/*
=================================================
	operator []
=================================================
*/
	template <typename T, typename I, typename A>
	inline typename IndexedPool<T,I,A>::Value_t &  IndexedPool<T,I,A>::operator [] (Index_t index)
	{
		ASSERT( index < _count );
		return _values[ index ];
	}
	
	template <typename T, typename I, typename A>
	inline typename IndexedPool<T,I,A>::Value_t const&  IndexedPool<T,I,A>::operator [] (Index_t index) const
	{
		ASSERT( index < _count );
		return _values[ index ];
	}


}	// FG

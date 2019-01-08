// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "IndexedPool2.h"
#include "stl/ThreadSafe/DummyLock.h"

namespace FG
{


	//
	// Chunked Indexed Pool
	//
	
	template <typename ValueType,
			  typename IndexType = uint,
			  size_t ChunkSize = 1024,
			  typename AllocatorType = UntypedAlignedAllocator,
			  typename AssignOpLock = DummyLock
			 >
	struct ChunkedIndexedPool2 final
	{
		STATIC_ASSERT( IsPowerOfTwo( ChunkSize ) );

	// types
	public:
		using Self			= ChunkedIndexedPool2< ValueType, IndexType, ChunkSize, AllocatorType, AssignOpLock >;
		using Index_t		= IndexType;
		using Value_t		= ValueType;
		using Allocator_t	= AllocatorType;

	private:
		using IdxPool_t		= IndexedPool2< ValueType, IndexType, ChunkSize >;
		using StdAlloc_t	= typename AllocatorType::template StdAllocator_t< IdxPool_t* >;
		using Chunks_t		= std::vector< IdxPool_t*, StdAlloc_t >;


	// variables
	private:
		mutable AssignOpLock	_assignOpLock;
		uint					_blockSizePowOf2;
		Chunks_t				_chunks;
		Allocator_t				_alloc;


	// methods
	public:
		explicit ChunkedIndexedPool2 (const Allocator_t &alloc = Allocator_t());

		ChunkedIndexedPool2 (const Self &) = delete;
		ChunkedIndexedPool2 (Self &&) = default;

		~ChunkedIndexedPool2() { Release(); }

		Self& operator = (const Self &) = delete;
		Self& operator = (Self &&) = default;
		
		void Release ();
		void Swap (Self &other);

		ND_ bool Assign (OUT Index_t &index);
		ND_ bool IsAssigned (Index_t index) const;
			bool Unassign (Index_t index);
		
		ND_ Value_t &		operator [] (Index_t index);
		ND_ Value_t const&	operator [] (Index_t index) const;

		ND_ bool			empty ()	const	{ return _chunks.empty(); }
		ND_ size_t			size ()		const	{ return _chunks.size() << _blockSizePowOf2; }
	};
	

	
/*
=================================================
	constructor
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline ChunkedIndexedPool2<T,I,S,A,L>::ChunkedIndexedPool2 (const Allocator_t &alloc) :
		_blockSizePowOf2{ uint(IntLog2( S )) },
		_chunks{ StdAlloc_t{alloc} },
		_alloc{ alloc }
	{
	}
	
/*
=================================================
	Release
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline void ChunkedIndexedPool2<T,I,S,A,L>::Release ()
	{
		for (auto chunk : _chunks)
		{
			chunk->~IdxPool_t();
			_alloc.Deallocate( chunk, SizeOf<IdxPool_t>, AlignOf<IdxPool_t> );
		}
		_chunks.clear();
	}

/*
=================================================
	Swap
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline void ChunkedIndexedPool2<T,I,S,A,L>::Swap (Self &other)
	{
		// TODO: swap _assignOpLock ?
		CHECK( _alloc == other._alloc );
		std::swap( _chunks, other._chunks );
		std::swap( _blockSizePowOf2, other._blockSizePowOf2 );
	}

/*
=================================================
	Assign
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline bool  ChunkedIndexedPool2<T,I,S,A,L>::Assign (OUT Index_t &index)
	{
		SCOPELOCK( _assignOpLock );

		const Index_t	mask = (Index_t(1) << _blockSizePowOf2) - 1;

		for (size_t i = 0, count = _chunks.size(); i < count; ++i)
		{
			if ( _chunks[i]->Assign( OUT index ) )
			{
				index = (index & mask) | uint(i << _blockSizePowOf2);
				return true;
			}
		}

		// create new
		{
			void*	ptr = _alloc.Allocate( SizeOf<IdxPool_t>, AlignOf<IdxPool_t> );
			_chunks.emplace_back( PlacementNew< IdxPool_t >( ptr ) );
		}

		if ( _chunks.back()->Assign( OUT index ) )
		{
			index = (index & mask) | uint((_chunks.size()-1) << _blockSizePowOf2);
			return true;
		}

		return false;
	}
	
/*
=================================================
	IsAssigned
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline bool  ChunkedIndexedPool2<T,I,S,A,L>::IsAssigned (Index_t index) const
	{
		SCOPELOCK( _assignOpLock );

		const Index_t	chunk_idx	= index >> _blockSizePowOf2;
		const Index_t	idx			= index & ((Index_t(1) << _blockSizePowOf2) - 1);
		CHECK_ERR( chunk_idx < _chunks.size() );

		return _chunks[ chunk_idx ]->IsAssigned( idx );
	}

/*
=================================================
	Unassign
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline bool  ChunkedIndexedPool2<T,I,S,A,L>::Unassign (Index_t index)
	{
		SCOPELOCK( _assignOpLock );

		const Index_t	chunk_idx	= index >> _blockSizePowOf2;
		const Index_t	idx			= index & ((Index_t(1) << _blockSizePowOf2) - 1);
		CHECK_ERR( chunk_idx < _chunks.size() );

		_chunks[ chunk_idx ]->Unassign( idx );
		return true;
	}
		
/*
=================================================
	operator []
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline typename ChunkedIndexedPool2<T,I,S,A,L>::Value_t &
		ChunkedIndexedPool2<T,I,S,A,L>::operator [] (Index_t index)
	{
		const Index_t	chunk_idx	= index >> _blockSizePowOf2;
		const Index_t	idx			= index & ((Index_t(1) << _blockSizePowOf2) - 1);
		ASSERT( chunk_idx < _chunks.size() );

		return _chunks[chunk_idx]->operator[] (idx);
	}
	
/*
=================================================
	operator []
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline typename ChunkedIndexedPool2<T,I,S,A,L>::Value_t const&
		ChunkedIndexedPool2<T,I,S,A,L>::operator [] (Index_t index) const
	{
		const Index_t	chunk_idx	= index >> _blockSizePowOf2;
		const Index_t	idx			= index & ((Index_t(1) << _blockSizePowOf2) - 1);
		ASSERT( chunk_idx < _chunks.size() );

		return _chunks[chunk_idx]->operator[] (idx);
	}


}	// FG

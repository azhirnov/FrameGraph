// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "IndexedPool.h"
#include "stl/CompileTime/DefaultType.h"
#include "stl/ThreadSafe/DummyLock.h"

namespace FG
{


	//
	// Chunked Indexed Pool
	//
	
	template <typename T,
			  typename IndexType = uint,
			  typename AllocatorType = UntypedAlignedAllocator,
			  typename AssignOpLock = DummyLock
			 >
	struct ChunkedIndexedPool1 final
	{
	// types
	public:
		using Self			= ChunkedIndexedPool1< T, IndexType, AllocatorType, AssignOpLock >;
		using Index_t		= IndexType;
		using Value_t		= T;
		using Allocator_t	= AllocatorType;

	private:
		using IdxPool_t		= IndexedPool< T, IndexType, AllocatorType >;
		using StdAlloc_t	= typename AllocatorType::template StdAllocator_t< IdxPool_t >;
		using Chunks_t		= std::vector< IdxPool_t, StdAlloc_t >;


	// variables
	private:
		AssignOpLock	_assignOpLock;
		Allocator_t		_alloc;
		Chunks_t		_chunks;
		uint			_blockSizePowOf2 = 1;


	// methods
	public:
		ChunkedIndexedPool1 () {}
		explicit ChunkedIndexedPool1 (uint blockSize, const Allocator_t &alloc = Allocator_t());

		ChunkedIndexedPool1 (const Self &) = delete;
		ChunkedIndexedPool1 (Self &&) = default;

		~ChunkedIndexedPool1() {}

		Self& operator = (const Self &) = delete;
		Self& operator = (Self &&) = default;
		
		void Release ()				{ _chunks.clear(); }
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
	template <typename T, typename I, typename A, typename L>
	inline ChunkedIndexedPool1<T,I,A,L>::ChunkedIndexedPool1 (uint blockSize, const Allocator_t &alloc) :
		_alloc{ alloc },
		_chunks{ StdAlloc_t{_alloc} },
		_blockSizePowOf2{ IntLog2(blockSize) + uint(not IsPowerOfTwo(blockSize)) }
	{
		ASSERT( _blockSizePowOf2 > 0 );
	}
	
/*
=================================================
	Swap
=================================================
*/
	template <typename T, typename I, typename A, typename L>
	void ChunkedIndexedPool1<T,I,A,L>::Swap (Self &other)
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
	template <typename T, typename I, typename A, typename L>
	inline bool  ChunkedIndexedPool1<T,I,A,L>::Assign (OUT Index_t &index)
	{
		SCOPELOCK( _assignOpLock );

		const Index_t	mask = (Index_t(1) << _blockSizePowOf2) - 1;
		
		for (size_t i = 0, count = _chunks.size(); i < count; ++i)
		{
			if ( _chunks[i].Assign( OUT index ) )
			{
				index = (index & mask) | uint(i << _blockSizePowOf2);
				return true;
			}
		}

		_chunks.emplace_back( (1u << _blockSizePowOf2), _alloc );

		if ( _chunks.back().Assign( OUT index ) )
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
	template <typename T, typename I, typename A, typename L>
	inline bool  ChunkedIndexedPool1<T,I,A,L>::IsAssigned (Index_t index) const
	{
		ASSERT(false);
		return false;
	}

/*
=================================================
	Unassign
=================================================
*/
	template <typename T, typename I, typename A, typename L>
	inline bool  ChunkedIndexedPool1<T,I,A,L>::Unassign (Index_t index)
	{
		SCOPELOCK( _assignOpLock );

		const Index_t	chunk_idx	= index >> _blockSizePowOf2;
		const Index_t	idx			= index & ((Index_t(1) << _blockSizePowOf2) - 1);
		CHECK_ERR( chunk_idx < _chunks.size() );

		auto&	chunk = _chunks[ chunk_idx ];

		chunk.Unassign( idx );
		return true;
	}
		
/*
=================================================
	operator []
=================================================
*/
	template <typename T, typename I, typename A, typename L>
	inline typename ChunkedIndexedPool1<T,I,A,L>::Value_t &
		ChunkedIndexedPool1<T,I,A,L>::operator [] (Index_t index)
	{
		const Index_t	chunk_idx	= index >> _blockSizePowOf2;
		const Index_t	idx			= index & ((Index_t(1) << _blockSizePowOf2) - 1);

		return _chunks[chunk_idx][idx];
	}
	
/*
=================================================
	operator []
=================================================
*/
	template <typename T, typename I, typename A, typename L>
	inline typename ChunkedIndexedPool1<T,I,A,L>::Value_t const&
		ChunkedIndexedPool1<T,I,A,L>::operator [] (Index_t index) const
	{
		const Index_t	chunk_idx	= index >> _blockSizePowOf2;
		const Index_t	idx			= index & ((Index_t(1) << _blockSizePowOf2) - 1);

		return _chunks[chunk_idx][idx];
	}


}	// FG

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/IndexedPool.h"
#include "stl/CompileTime/DefaultType.h"

namespace FG
{


	//
	// Chunked Indexed Pool
	//
	
	template <typename T,
			  typename IndexType = uint,
			  typename AllocatorType = UntypedAlignedAllocator
			 >
	struct ChunkedIndexedPool
	{
	// types
	public:
		using Self			= ChunkedIndexedPool< T, IndexType, AllocatorType >;
		using Index_t		= IndexType;
		using Value_t		= T;
		using Allocator_t	= AllocatorType;

	private:
		using IdxPool_t		= IndexedPool< T, IndexType, AllocatorType >;
		using StdAlloc_t	= typename AllocatorType::template StdAllocator_t< IdxPool_t >;
		using Chunks_t		= std::vector< IdxPool_t, StdAlloc_t >;


	// variables
	private:
		Allocator_t		_alloc;
		Chunks_t		_chunks;
		uint			_blockSizePowOf2 = 1;


	// methods
	public:
		explicit ChunkedIndexedPool (uint blockSize, const Allocator_t &alloc = Allocator_t());
		ChunkedIndexedPool () {}
		ChunkedIndexedPool (const Self &) = delete;
		ChunkedIndexedPool (Self &&) = default;
		~ChunkedIndexedPool() {}

		Self& operator = (const Self &) = delete;
		Self& operator = (Self &&) = default;
		
		void Release ()		{ _chunks.clear(); }

		ND_ bool Assign (OUT Index_t &index);
			bool Unassign (Index_t index);
		
		ND_ Value_t &		operator [] (Index_t index);
		ND_ Value_t const&	operator [] (Index_t index) const;
	};
	

	
/*
=================================================
	constructor
=================================================
*/
	template <typename T, typename I, typename A>
	inline ChunkedIndexedPool<T,I,A>::ChunkedIndexedPool (uint blockSize, const Allocator_t &alloc) :
		_alloc{ alloc },
		_chunks{ StdAlloc_t{_alloc} },
		_blockSizePowOf2{ IntLog2(blockSize) + uint(not IsPowerOfTwo(blockSize)) }
	{
		ASSERT( _blockSizePowOf2 > 0 );
	}

/*
=================================================
	Assign
=================================================
*/
	template <typename T, typename I, typename A>
	inline bool  ChunkedIndexedPool<T,I,A>::Assign (OUT Index_t &index)
	{
		ASSERT( (_chunks.size() + _blockSizePowOf2) < sizeof(Index_t)*8 );

		const Index_t	mask = (Index_t(1) << _blockSizePowOf2) - 1;

		for (size_t i = 0; i < _chunks.size(); ++i)
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
	Unassign
=================================================
*/
	template <typename T, typename I, typename A>
	inline bool  ChunkedIndexedPool<T,I,A>::Unassign (Index_t index)
	{
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
	template <typename T, typename I, typename A>
	inline typename ChunkedIndexedPool<T,I,A>::Value_t &
		ChunkedIndexedPool<T,I,A>::operator [] (Index_t index)
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
	template <typename T, typename I, typename A>
	inline typename ChunkedIndexedPool<T,I,A>::Value_t const&
		ChunkedIndexedPool<T,I,A>::operator [] (Index_t index) const
	{
		const Index_t	chunk_idx	= index >> _blockSizePowOf2;
		const Index_t	idx			= index & ((Index_t(1) << _blockSizePowOf2) - 1);

		return _chunks[chunk_idx][idx];
	}


}	// FG

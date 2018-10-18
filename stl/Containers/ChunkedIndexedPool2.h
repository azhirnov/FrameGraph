// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/IndexedPool2.h"
#include "stl/CompileTime/DefaultType.h"
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
		using StdAlloc_t	= typename AllocatorType::template StdAllocator_t< IdxPool_t >;
		using Chunks_t		= std::list< IdxPool_t, StdAlloc_t >;


	// variables
	private:
		AssignOpLock	_assignOpLock;
		Chunks_t		_chunks;
		uint			_blockSizePowOf2;


	// methods
	public:
		explicit ChunkedIndexedPool2 (const Allocator_t &alloc = Allocator_t());

		ChunkedIndexedPool2 (const Self &) = delete;
		ChunkedIndexedPool2 (Self &&) = default;

		~ChunkedIndexedPool2() {}

		Self& operator = (const Self &) = delete;
		Self& operator = (Self &&) = default;
		
		void Release ()				{ _chunks.clear(); }
		void Swap (Self &other);

		ND_ bool Assign (OUT Index_t &index);
		ND_ bool IsAssigned (Index_t index) const;
			bool Unassign (Index_t index);
		
		ND_ Value_t &		operator [] (Index_t index);
		ND_ Value_t const&	operator [] (Index_t index) const;

		ND_ bool			Empty ()	const	{ return _chunks.empty(); }
		ND_ size_t			Count ()	const	{ return _chunks.size() << _blockSizePowOf2; }
	};
	

	
/*
=================================================
	constructor
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline ChunkedIndexedPool2<T,I,S,A,L>::ChunkedIndexedPool2 (const Allocator_t &alloc) :
		_chunks{ StdAlloc_t{alloc} },
		_blockSizePowOf2{ uint(IntLog2( S )) }
	{
	}
	
/*
=================================================
	Swap
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	void ChunkedIndexedPool2<T,I,S,A,L>::Swap (Self &other)
	{
		// TODO: swap _assignOpLock ?
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
		ASSERT( (_chunks.size() + _blockSizePowOf2) < sizeof(Index_t)*8 );
		SCOPELOCK( _assignOpLock );

		const Index_t	mask	= (Index_t(1) << _blockSizePowOf2) - 1;
		size_t			i		= 0;

		for (auto iter = _chunks.begin(); iter != _chunks.end(); ++iter, ++i)
		{
			if ( iter->Assign( OUT index ) )
			{
				index = (index & mask) | uint(i << _blockSizePowOf2);
				return true;
			}
		}

		_chunks.emplace_back();

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
	template <typename T, typename I, size_t S, typename A, typename L>
	inline bool  ChunkedIndexedPool2<T,I,S,A,L>::IsAssigned (Index_t index) const
	{
		SCOPELOCK( _assignOpLock );
		
		const Index_t	chunk_idx	= index >> _blockSizePowOf2;
		const Index_t	idx			= index & ((Index_t(1) << _blockSizePowOf2) - 1);
		ASSERT( chunk_idx < _chunks.size() );

		auto	chunk_iter = _chunks.begin();
		std::advance( chunk_iter, chunk_idx );

		return chunk_iter->IsAssigned( idx );
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
		ASSERT( chunk_idx < _chunks.size() );
		
		auto	chunk_iter = _chunks.begin();
		std::advance( chunk_iter, chunk_idx );

		chunk_iter->Unassign( idx );
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
		
		auto	chunk_iter = _chunks.begin();
		std::advance( chunk_iter, chunk_idx );

		return chunk_iter->operator [] (idx);
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
		
		auto	chunk_iter = _chunks.begin();
		std::advance( chunk_iter, chunk_idx );

		return chunk_iter->operator [] (idx);
	}


}	// FG

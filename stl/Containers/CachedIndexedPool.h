// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ChunkedIndexedPool.h"

namespace FG
{

	//
	// Cached Chunked Indexed Pool
	//

	template <typename ValueType,
			  typename IndexType,
			  size_t ChunkSize,
			  size_t MaxChunks = 16,
			  typename AllocatorType = UntypedAlignedAllocator,
			  typename AssignOpLock = DummyLock,
			  template <typename T> class AtomicChunkPtr = NonAtomicPtr
			 >
	struct CachedIndexedPool final
	{
	// types
	public:
		using Self			= CachedIndexedPool< ValueType, IndexType, ChunkSize, MaxChunks, AllocatorType, AssignOpLock, AtomicChunkPtr >;
		using Index_t		= IndexType;
		using Value_t		= ValueType;
		using Allocator_t	= AllocatorType;

	private:
		struct THash  { size_t operator () (const Value_t *value) const						{ return std::hash<Value_t>()( *value ); }};
		struct TEqual { bool   operator () (const Value_t *lhs, const Value_t *rhs) const	{ return *lhs == *rhs; }};

		using Pool_t		= ChunkedIndexedPool< Value_t, IndexType, ChunkSize, MaxChunks, AllocatorType, AssignOpLock, AtomicChunkPtr >;
		using StdAlloc_t	= typename AllocatorType::template StdAllocator_t<Pair< Value_t const* const, Index_t >>;
		using Cache_t		= std::unordered_map< Value_t const*, Index_t, THash, TEqual, StdAlloc_t >;


	// variables
	private:
		Pool_t		_pool;
		Cache_t		_cache;


	// methods
	public:
		CachedIndexedPool (Self &&) = default;

		Self&  operator = (Self &&) = default;


		explicit CachedIndexedPool (const Allocator_t &alloc = Allocator_t()) :
			_pool{ alloc },  _cache{ StdAlloc_t{alloc} }
		{}

		~CachedIndexedPool ()
		{}
		

		void  Release ()
		{
			_pool.Release();

			Cache_t		temp{ _cache.get_allocator() };
			std::swap( _cache, temp );
		}


		void  Swap (Self &other)
		{
			_pool.Swap( other._pool );
			_cache.swap( other._cache );
		}

		
		ND_ Pair<Index_t, bool>  Insert (Index_t index, Value_t&& value)
		{
			std::swap( _pool[index], value );

			return AddToCache( index );
		}


		ND_ Pair<Index_t, bool>  AddToCache (Index_t index)
		{
			auto	result = _cache.insert({ &_pool[index], index });

			return { result.first->second, result.second };
		}


		bool  RemoveFromCache (Index_t index)
		{
			auto	iter = _cache.find( &_pool[index] );

			if ( iter != _cache.end() and iter->second == index )
			{
				_cache.erase( iter );
				return true;
			}
			return false;
		}

		
		ND_ Index_t  Find (const Value_t *value) const
		{
			auto	iter = _cache.find( value );

			if ( iter != _cache.end() )
				return iter->second;

			return UMax;
		}
		

		ND_ BytesU  DynamicSize () const
		{
			BytesU	sz = _pool.DynamicSize();
			//sz += _cache.max_bucket_count() * _cache.max_size();	// TODO
			return sz;
		}


		template <typename ArrayType>
		ND_ size_t  Assign (size_t count, INOUT ArrayType &arr)			{ return _pool.Assign( count, INOUT arr ); }
		
		template <typename ArrayType>
			void  Unassign (size_t count, INOUT ArrayType &arr)			{ return _pool.Unassign( count, INOUT arr ); }

		ND_ bool  Assign (OUT Index_t &index)							{ return _pool.Assign( OUT index ); }
			void  Unassign (Index_t index)								{ return _pool.Unassign( index ); }

		ND_ Value_t &			operator [] (Index_t index)				{ return _pool[ index ]; }
		ND_ Value_t const&		operator [] (Index_t index)		const	{ return _pool[ index ]; }

		ND_ bool				empty ()						const	{ return _pool.empty(); }
		ND_ size_t				size ()							const	{ return _pool.size(); }
		ND_ constexpr size_t	capacity ()						const	{ return _pool.capacity(); }
	};
	
	
}	// FG

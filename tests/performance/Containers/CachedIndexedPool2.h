// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ChunkedIndexedPool2.h"

namespace FG
{

	//
	// Cached Chunked Indexed Pool
	//

	template <typename ValueType,
			  typename IndexType = uint,
			  size_t ChunkSize = 1024,
			  typename AllocatorType = UntypedAlignedAllocator,
			  typename AssignOpLock = DummyLock
			 >
	struct CachedIndexedPool2 final
	{
	// types
	public:
		using Self			= CachedIndexedPool2< ValueType, IndexType, ChunkSize, AllocatorType, AssignOpLock >;
		using Index_t		= IndexType;
		using Value_t		= ValueType;
		using Allocator_t	= AllocatorType;

	private:
		struct THash  { size_t operator () (const Value_t *value) const						{ return std::hash<Value_t>()( *value ); } };
		struct TEqual { bool   operator () (const Value_t *lhs, const Value_t *rhs) const	{ return *lhs == *rhs; } };

		using Pool_t		= ChunkedIndexedPool2< Value_t, IndexType, ChunkSize, AllocatorType, AssignOpLock >;
		using StdAlloc_t	= typename AllocatorType::template StdAllocator_t<Pair< Value_t const* const, Index_t >>;
		using Cache_t		= std::unordered_map< Value_t const*, Index_t, THash, TEqual, StdAlloc_t >;


	// variables
	private:
		Pool_t		_pool;
		Cache_t		_cache;


	// methods
	public:
		explicit CachedIndexedPool2 (const Allocator_t &alloc = Allocator_t());
		CachedIndexedPool2 (Self &&) = default;
		~CachedIndexedPool2() {}

		Self&  operator = (Self &&) = default;
		
		void  Release ();
		void  Swap (Self &other);

		ND_ bool  Assign (OUT Index_t &index)							{ return _pool.Assign( OUT index ); }
		ND_ bool  IsAssigned (Index_t index)							{ return _pool.IsAssigned( index ); }
			bool  Unassign (Index_t index, bool removeFromcache);

		ND_ Pair<Index_t,bool>  Insert (Index_t index, Value_t&& value);
		ND_ Pair<Index_t,bool>	AddToCache (Index_t index);
		
		ND_ Index_t				Find (const Value_t *value) const;

		ND_ Value_t &			operator [] (Index_t index)				{ return _pool[ index ]; }
		ND_ Value_t const&		operator [] (Index_t index)		const	{ return _pool[ index ]; }

		ND_ bool				empty ()						const	{ return _pool.empty(); }
		ND_ size_t				size ()							const	{ return _pool.size(); }
	};
	
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline CachedIndexedPool2<T,I,S,A,L>::CachedIndexedPool2 (const Allocator_t &alloc) :
		_pool{ alloc },
		_cache{ StdAlloc_t{alloc} }
	{
	}
	
/*
=================================================
	Release
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline void  CachedIndexedPool2<T,I,S,A,L>::Release ()
	{
		_pool.Release();

		Cache_t		temp{ _cache.get_allocator() };
		std::swap( _cache, temp );
	}
	
/*
=================================================
	Swap
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline void  CachedIndexedPool2<T,I,S,A,L>::Swap (Self &other)
	{
		_pool.Swap( other._pool );
		_cache.swap( other._cache );
	}

/*
=================================================
	Unassign
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline bool  CachedIndexedPool2<T,I,S,A,L>::Unassign (Index_t index, bool removeFromcache)
	{
		if ( removeFromcache )
		{
			auto	iter = _cache.find( &_pool[index] );

			if ( iter != _cache.end() and iter->second == index )
				_cache.erase( iter );
		}
		return _pool.Unassign( index );
	}
	
/*
=================================================
	Insert
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline Pair<I, bool>  CachedIndexedPool2<T,I,S,A,L>::Insert (Index_t index, Value_t&& value)
	{
		std::swap( _pool[index], value );

		return AddToCache( index );
	}
	
/*
=================================================
	AddToCache
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline Pair<I, bool>  CachedIndexedPool2<T,I,S,A,L>::AddToCache (Index_t index)
	{
		auto	result = _cache.insert({ &_pool[index], index });

		return { result.first->second, result.second };
	}
	
/*
=================================================
	Find
=================================================
*/
	template <typename T, typename I, size_t S, typename A, typename L>
	inline I  CachedIndexedPool2<T,I,S,A,L>::Find (const Value_t *value) const
	{
		auto	iter = _cache.find( value );

		if ( iter != _cache.end() )
			return iter->second;

		return ~Index_t(0);
	}


}	// FG

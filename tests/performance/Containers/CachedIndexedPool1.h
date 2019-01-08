// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ChunkedIndexedPool1.h"

namespace FG
{

	//
	// Cached Chunked Indexed Pool
	//

	template <typename T,
			  typename IndexType = uint,
			  typename AllocatorType = UntypedAlignedAllocator,
			  typename AssignOpLock = DummyLock
			 >
	struct CachedIndexedPool1 final
	{
	// types
	public:
		using Self			= CachedIndexedPool1< T, IndexType, AllocatorType, AssignOpLock >;
		using Index_t		= IndexType;
		using Value_t		= T;
		using Allocator_t	= AllocatorType;

	private:
		struct THash  { size_t operator () (const T *value) const				{ return std::hash<T>()( *value ); } };
		struct TEqual { bool   operator () (const T *lhs, const T *rhs) const	{ return *lhs == *rhs; } };

		using Pool_t		= ChunkedIndexedPool1< T, IndexType, AllocatorType, AssignOpLock >;
		using StdAlloc_t	= typename AllocatorType::template StdAllocator_t<Pair< T const* const, Index_t >>;
		using Cache_t		= std::unordered_map< T const*, Index_t, THash, TEqual, StdAlloc_t >;


	// variables
	private:
		Pool_t		_pool;
		Cache_t		_cache;


	// methods
	public:
		CachedIndexedPool1 () {}
		CachedIndexedPool1 (Self &&) = default;
		explicit CachedIndexedPool1 (uint blockSize, const Allocator_t &alloc = Allocator_t());
		~CachedIndexedPool1() {}

		Self&  operator = (Self &&) = default;
		
		void  Release ();
		void  Swap (Self &other);

		ND_ bool  Assign (OUT Index_t &index)							{ return _pool.Assign( OUT index ); }
		ND_ bool  IsAssigned (Index_t index)							{ return _pool.IsAssigned( index ); }
			bool  Unassign (Index_t index, bool removeFromcache);

		ND_ Pair<Index_t,bool>  Insert (Index_t index, Value_t&& value);
		
		ND_ Value_t &			operator [] (Index_t index)				{ return _pool[ index ]; }
		ND_ Value_t const&		operator [] (Index_t index)		const	{ return _pool[ index ]; }

		ND_ bool				empty ()						const	{ return _pool.Empty(); }
		ND_ size_t				size ()							const	{ return _pool.Count(); }
	};
	
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T, typename I, typename A, typename L>
	inline CachedIndexedPool1<T,I,A,L>::CachedIndexedPool1 (uint blockSize, const Allocator_t &alloc) :
		_pool{ blockSize, alloc },
		_cache{ StdAlloc_t{alloc} }
	{
	}
	
/*
=================================================
	Release
=================================================
*/
	template <typename T, typename I, typename A, typename L>
	inline void  CachedIndexedPool1<T,I,A,L>::Release ()
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
	template <typename T, typename I, typename A, typename L>
	inline void  CachedIndexedPool1<T,I,A,L>::Swap (Self &other)
	{
		_pool.Swap( other._pool );
		_cache.swap( other._cache );
	}

/*
=================================================
	Unassign
=================================================
*/
	template <typename T, typename I, typename A, typename L>
	inline bool  CachedIndexedPool1<T,I,A,L>::Unassign (Index_t index, bool removeFromcache)
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
	template <typename T, typename I, typename A, typename L>
	inline Pair<typename CachedIndexedPool1<T,I,A,L>::Index_t, bool>
		CachedIndexedPool1<T,I,A,L>::Insert (Index_t index, Value_t&& value)
	{
		_pool[index] = std::move(value);

		auto	result = _cache.insert({ &_pool[index], index });

		return { result.first->second, result.second };
	}


}	// FG

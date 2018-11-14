// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include <atomic>

namespace FG
{


	//
	// Lock-free Fixed Size Stack (TODO: rename to Appendable?)
	//

	template <typename T, size_t Size>
	struct LfFixedStack
	{
	// variables
	private:
		alignas(FG_CACHE_LINE) std::atomic<uint>	_count {0};
		StaticArray< T, Size >						_data;


	// methods
	public:
		LfFixedStack ()
		{}

		bool Push (const T &value)
		{
			const uint	idx = _count.fetch_add( 1, memory_order_relaxed );
			if ( idx < Size ) {
				_data[idx] = value;
				return true;
			}
			return false;
		}
		
		bool Push (T &&value)
		{
			const uint	idx = _count.fetch_add( 1, memory_order_relaxed );
			if ( idx < Size ) {
				_data[idx] = std::move(value);
				return true;
			}
			return false;
		}

		ND_ ArrayView<T>  Get () const
		{
			// warning: add memory barrier before reading!
			// std::atomic_thread_fence();
			return _data;
		}
	};



	//
	// Lock-free Fixed Size Stack
	//

	template <typename ...Types, size_t Size>
	struct LfFixedStack< std::tuple<Types...>, Size >
	{
	// types
	private:
		using TupleOfArrays_t = std::tuple< StaticArray<Types, Size>... >;


	// variables
	private:
		alignas(FG_CACHE_LINE) std::atomic<uint>	_count {0};
		TupleOfArrays_t								_data;
		static constexpr size_t						_tupleSize = std::tuple_size_v< TupleOfArrays_t >;

		
	// methods
	public:
		LfFixedStack ()
		{}
		
		bool Push (const Types&... args)
		{
			const uint	idx = _count.fetch_add( 1, memory_order_relaxed );
			if ( idx < Size ) {
				_SetTupleElements<0>( idx, std::forward<const Types&>( args )... );
				return true;
			}
			return false;
		}

		bool Push (Types&&... args)
		{
			const uint	idx = _count.fetch_add( 1, memory_order_relaxed );
			if ( idx < Size ) {
				_SetTupleElements<0>( idx, std::forward<Types &&>( args )... );
				return true;
			}
			return false;
		}
		
		template <typename T>
		ND_ ArrayView<T>  Get () const
		{
			// warning: add memory barrier before reading!
			// std::atomic_thread_fence();
			return std::get< StaticArray<T,Size> >( _data );
		}


	private:
		template <size_t I, typename Arg0, typename ...Args>
		void _SetTupleElements (uint idx, Arg0 &&arg0, Args&&... args)
		{
			std::get<I>( _data )[idx] = std::forward<Arg0 &&>( arg0 );

			if constexpr ( I+1 < _tupleSize )
			{
				_SetTupleElements<I+1>( idx, std::forward<Args &&>( args )... );
			}
		}
	};


}	// FG
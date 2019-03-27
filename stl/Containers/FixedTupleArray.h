// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/CompileTime/TypeList.h"

namespace FGC
{

	//
	// Fixed Size Tuple Array
	//

	template <size_t ArraySize, typename ...Types>
	struct FixedTupleArray
	{
	// types
	private:
		template <typename T>
		using ElemArray	= T [ArraySize];

		using Array_t	= Tuple< ElemArray<Types>... >;
		using Self		= FixedTupleArray< ArraySize, Types... >;


	// variables
	private:
		Array_t		_arrays;
		size_t		_count	= 0;


	// methods
	public:
		constexpr FixedTupleArray ()
		{}

		~FixedTupleArray ()
		{}


		template <typename T>
		ND_ constexpr ArrayView<T>	get ()		const	{ return ArrayView<T>{ &std::get< ElemArray<T> >( _arrays )[0], _count }; }
		
		template <size_t I>
		ND_ constexpr auto			get ()		const	{ return get< TypeList<Types...>::template Get<I> >(); }

		ND_ constexpr size_t		size ()		const	{ return _count; }
		ND_ constexpr bool			empty ()	const	{ return _count == 0; }

		ND_ static constexpr size_t	capacity ()			{ return ArraySize; }

		
		constexpr void  push_back (const Types&... values)
		{
			ASSERT( _count < capacity() );
			_PushBack<0>( values... );
			++_count;
		}

		constexpr void  push_back (Types&&... values)
		{
			ASSERT( _count < capacity() );
			_PushBack<0>( std::forward<Types&&>(values)... );
			++_count;
		}
		
		constexpr void  insert (size_t pos, const Types&... values)
		{
			ASSERT( _count < capacity() );
			if ( pos >= _count )	_PushBack<0>( values... );
			else					_Insert<0>( pos, values... );
			++_count;
		}

		constexpr void  insert (size_t pos, Types&&... values)
		{
			ASSERT( _count < capacity() );
			if ( pos >= _count )	_PushBack<0>( std::forward<Types&&>(values)... );
			else					_Insert<0>( pos, std::forward<Types&&>(values)... );
			++_count;
		}

		/*constexpr void resize (size_t newSize)
		{
			newSize = Min( newSize, capacity() );

			if ( newSize < _count )
			{
				// delete elements
				for (size_t i = newSize; i < _count; ++i)
				{
					_Destroy<0>( i );
				}
				DEBUG_ONLY( ::memset( data() + newSize, 0, sizeof(T) * (_count - newSize) ));
			}

			if ( newSize > _count )
			{
				// create elements
				for (size_t i = _count; i < newSize; ++i)
				{
					PlacementNew<T>( data() + i );
				}
			}

			_count = newSize;
		}*/


		constexpr void clear ()
		{
			for (size_t i = 0; i < _count; ++i) {
				_Destroy<0>( i );
			}

			_count = 0;
		}


	private:
		template <typename T>	ND_ constexpr T*	_Data ()	{ return &std::get< ElemArray<T> >( _arrays )[0]; }
		template <size_t I>		ND_ constexpr auto*	_Data ()	{ return &std::get<I>( _arrays )[0]; }


		template <size_t I, typename Arg0, typename ...Args>
		constexpr void  _PushBack (Arg0 &&arg0, Args&&... args)
		{
			using T = std::remove_const_t< std::remove_reference_t< Arg0 >>;

			PlacementNew<T>( _Data<I>() + _count, std::forward<Arg0>(arg0) );
			
			if constexpr ( I+1 < CountOf<Types...>() )
				_PushBack<I+1>( std::forward<Args&&>(args)... );
		}
		

		template <size_t I, typename Arg0, typename ...Args>
		constexpr void  _Insert (size_t pos, Arg0 &&arg0, Args&&... args)
		{
			using T = std::remove_const_t< std::remove_reference_t< Arg0 >>;
			T* data = _Data<I>();

			for (size_t i = _count; i > pos; --i) {
				PlacementNew<T>( data + i, std::move(data[i-1]) );
			}

			PlacementNew<T>( data + pos, std::forward<Arg0>(arg0) );
			
			if constexpr ( I+1 < CountOf<Types...>() )
				_Insert<I+1>( pos, std::forward<Args&&>(args)... );
		}


		template <size_t I>
		constexpr void  _Destroy (size_t index)
		{
			using T = typename TypeList< Types... >::template Get<I>;

			_Data<T>()[index].~T();
			DEBUG_ONLY( ::memset( &_Data<T>()[index], 0, sizeof(T) ));

			if constexpr ( I+1 < CountOf<Types...>() )
				_Destroy<I+1>( index );
		}
	};


}	// FGC

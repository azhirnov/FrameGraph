// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"
#include "stl/Containers/FixedArray.h"

namespace FGC
{
	
/*
=================================================
	CountOf
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr size_t  CountOf (T& value)
	{
		return std::size( value );
	}
	
	template <typename ...Types>
	ND_ forceinline constexpr size_t  CountOf ()
	{
		return sizeof... (Types);
	}
	
	template <typename T, size_t I, typename Class>
	ND_ forceinline constexpr size_t  CountOf (T (Class::*) [I])
	{
		return I;
	}
	
	template <size_t I>
	ND_ forceinline constexpr size_t  CountOf (const BitSet<I> &)
	{
		return I;
	}

/*
=================================================
	ArraySizeOf
=================================================
*/
	template <typename T, typename A>
	ND_ forceinline BytesU  ArraySizeOf (const std::vector<T,A> &arr)
	{
		return BytesU( arr.size() * sizeof(T) );
	}

	template <typename T, size_t S>
	ND_ forceinline BytesU  ArraySizeOf (const FixedArray<T,S> &arr)
	{
		return BytesU( arr.size() * sizeof(T) );
	}

	template <typename T>
	ND_ forceinline BytesU  ArraySizeOf (const ArrayView<T> &arr)
	{
		return BytesU( arr.size() * sizeof(T) );
	}
	
	template <typename T, size_t S>
	ND_ forceinline constexpr BytesU  ArraySizeOf (const StaticArray<T,S> &)
	{
		return BytesU( S * sizeof(T) );
	}

/*
=================================================
	Distance
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr ptrdiff_t  Distance (T *lhs, T *rhs)
	{
		return std::distance< T *>( lhs, rhs );
	}
	
	template <typename T>
	ND_ forceinline constexpr ptrdiff_t  Distance (const T *lhs, T *rhs)
	{
		return std::distance< T const *>( lhs, rhs );
	}
	
	template <typename T>
	ND_ forceinline constexpr ptrdiff_t  Distance (T *lhs, const T *rhs)
	{
		return std::distance< T const *>( lhs, rhs );
	}
	
/*
=================================================
	IsFirstElement
=================================================
*/
	template <typename Elem, typename Container>
	ND_ inline bool  IsFirstElement (const Elem &elem, const Container &arr)
	{
		return &arr.front() == &elem;
	}
	
/*
=================================================
	IsLastElement
=================================================
*/
	template <typename Elem, typename Container>
	ND_ inline bool  IsLastElement (const Elem &elem, const Container &arr)
	{
		return &arr.back() == &elem;
	}

/*
=================================================
	LowerBound (binary search)
=================================================
*/
	template <typename T, typename Key>
	ND_ forceinline size_t  LowerBound (ArrayView<T> arr, const Key &key)
	{
		size_t	left	= 0;
		size_t	right	= arr.size();

		for (; left < right; )
		{
			size_t	mid = (left + right) >> 1;

			if ( arr[mid] < key )
				left = mid + 1;
			else
				right = mid;
		}

		return left < arr.size() and arr[left] == key ? left : UMax;
	}
	
	template <typename T, typename Key>
	ND_ forceinline size_t  LowerBound (const Array<T> &arr, const Key &key)
	{
		return LowerBound( ArrayView<T>{arr}, key );
	}
	
/*
=================================================
	BinarySearch
=================================================
*/
	template <typename T, typename Key>
	ND_ forceinline size_t  BinarySearch (ArrayView<T> arr, const Key &key)
	{
		ptrdiff_t	left	= 0;
		ptrdiff_t	right	= arr.size();

		for (; left < right; )
		{
			ptrdiff_t	mid = (left + right) >> 1;

			if ( arr[mid] < key )
				left = mid + 1;
			else
			if ( not (arr[mid] == key) )
				right = mid - 1;
			else
				return mid;
		}

		return left < ptrdiff_t(arr.size()) and arr[left] == key ? left : -1;
	}
	
	template <typename T, typename Key>
	ND_ forceinline size_t  BinarySearch (const Array<T> &arr, const Key &key)
	{
		return BinarySearch( ArrayView<T>{arr}, key );
	}
	
/*
=================================================
	ExponentialSearch
=================================================
*/
	template <typename T, typename Key>
	ND_ forceinline size_t  ExponentialSearch (ArrayView<T> arr, const Key &key)
	{
		if ( arr.empty() )
			return UMax;

		size_t	left	= 0;
		size_t	right	= arr.size();
		size_t	bound	= 1;

		for (; bound < right and arr[bound] < key; bound *= 2)
		{}

		left  = bound >> 1;
		right = Min( bound+1, right );

		for (; left < right; )
		{
			size_t	mid = (left + right) >> 1;

			if ( arr[mid] < key )
				left = mid + 1;
			else
				right = mid;
		}

		return left < arr.size() and arr[left] == key ? left : UMax;
	}
	
	template <typename T, typename Key>
	ND_ forceinline size_t  ExponentialSearch (const Array<T> &arr, const Key &key)
	{
		return ExponentialSearch( ArrayView<T>{arr}, key );
	}
	
/*
=================================================
	Reverse
=================================================
*/
namespace _fgc_hidden_
{
	template <typename Container>
	class ReverseContainerView
	{
	private:
		Container &		_container;

	public:
		explicit ReverseContainerView (Container& container) : _container{container} {}

		ND_ auto  begin ()	{ return std::rbegin( _container ); }
		ND_ auto  end ()	{ return std::rend( _container ); }
	};

}	// _fgc_hidden_

	template <typename Container>
	ND_ auto  Reverse (Container& container)
	{
		return _fgc_hidden_::ReverseContainerView<Container>{ container };
	}

	template <typename Container>
	ND_ auto  Reverse (const Container& container)
	{
		return _fgc_hidden_::ReverseContainerView<const Container>{ container };
	}

}	// FGC

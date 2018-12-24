// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"
#include "stl/Containers/FixedArray.h"

namespace FG
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
	BinarySearch
=================================================
*/
	template <typename T, typename Key>
	ND_ forceinline size_t  BinarySearch (ArrayView<T> arr, const Key &key)
	{
		size_t	left	= 0;
		size_t	right	= arr.size()-1;

		for (; right - left > 1; )
		{
			size_t	mid = (left + right) >> 1;

			if ( arr[mid] < key )
				left = mid;
			else
				right = mid;
		}

		if ( arr[left] == key )
			return left;
		
		if ( arr[right] == key )
			return right;

		return UMax;
	}

}	// FG

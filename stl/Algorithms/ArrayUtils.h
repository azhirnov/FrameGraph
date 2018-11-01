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
	template <typename T>
	ND_ forceinline BytesU  ArraySizeOf (const Array<T> &arr)
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


}	// FG

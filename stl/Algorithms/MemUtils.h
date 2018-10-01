// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"
#include "stl/Containers/FixedArray.h"

namespace FG
{

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
	MemCopy
=================================================
*/
	template <typename T1, typename T2>
	forceinline void MemCopy (T1 &dst, const T2 &src)
	{
		STATIC_ASSERT( sizeof(dst) == sizeof(src) );
		//STATIC_ASSERT( std::is_trivial_v<T1> and std::is_trivial_v<T2> );	// TODO

        ::memcpy( &dst, &src, sizeof(src) );
	}

/*
=================================================
	MemCopy
=================================================
*/
	forceinline void MemCopy (void *dst, BytesU dstSize, const void *src, BytesU srcSize)
	{
		ASSERT( srcSize <= dstSize );
		ASSERT( dst and src );

        ::memcpy( dst, src, size_t(srcSize) );
	}


}	// FG

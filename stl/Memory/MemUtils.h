// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"
#include "stl/Math/BitMath.h"
#include "stl/CompileTime/TypeTraits.h"

namespace FG
{

/*
=================================================
	AddressOf
=================================================
*/
	template <typename T>
	ND_ forceinline decltype(auto)  AddressOf (T &value)
	{
		return std::addressof( value );
	}

/*
=================================================
	CheckPointerAlignment
=================================================
*/
	template <typename T>
	ND_ forceinline bool  CheckPointerAlignment (void *ptr) noexcept
	{
		constexpr size_t	align = alignof(T);

		STATIC_ASSERT( IsPowerOfTwo( align ), "Align must be power of 2" );

		return (sizeof(T) < align) or not (size_t(ptr) & (align-1));
	}
	
/*
=================================================
	PlacementNew
=================================================
*/
	template <typename T, typename ...Types>
	forceinline T *  PlacementNew (OUT void *ptr, Types&&... args) noexcept
	{
		ASSERT( CheckPointerAlignment<T>( ptr ) );
		return ( new(ptr) T{ std::forward<Types &&>(args)... } );
	}

/*
=================================================
	MemCopy
=================================================
*/
	template <typename T1, typename T2>
	forceinline void  MemCopy (T1 &dst, const T2 &src)
	{
		STATIC_ASSERT( sizeof(dst) == sizeof(src) );
		//STATIC_ASSERT( std::is_trivial_v<T1> and std::is_trivial_v<T2> );	// TODO
		STATIC_ASSERT( not IsConst<T1> );

		::memcpy( &dst, &src, sizeof(src) );
	}

	forceinline void  MemCopy (void *dst, BytesU dstSize, const void *src, BytesU srcSize)
	{
		ASSERT( srcSize <= dstSize );
		ASSERT( dst and src );

		::memcpy( dst, src, size_t(std::min(srcSize, dstSize)) );
	}


}	// FG

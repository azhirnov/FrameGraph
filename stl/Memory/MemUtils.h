// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"
#include "stl/Containers/FixedArray.h"

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
	CountOf
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr size_t  CountOf (T& value)
	{
		return std::size( value );
	}
	
/*
=================================================
	CheckPointerAlignment
=================================================
*/
	template <typename T>
	forceinline bool CheckPointerAlignment (void *ptr) noexcept
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
		return ( new(ptr) T( std::forward<Types &&>(args)... ) );
	}
	
/*
=================================================
	Allocate
=================================================
*
	ND_ FG_ALLOCATOR forceinline void*  Allocate (BytesU size, BytesU align)
	{
		return ::operator new ( size_t(size), std::align_val_t(size_t(align)), std::nothrow_t{} );
	}
	
/*
=================================================
	Deallocate
=================================================
*
	forceinline void Deallocate (void *ptr, BytesU align)
	{
		::operator delete ( ptr, std::align_val_t(size_t(align)), std::nothrow_t() );
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

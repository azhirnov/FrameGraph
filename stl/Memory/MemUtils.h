// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"
#include "stl/CompileTime/TypeTraits.h"

namespace FGC
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
	PlacementNew
=================================================
*/
	template <typename T, typename ...Types>
    forceinline T *  PlacementNew (OUT void *ptr, Types&&... args)
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
		STATIC_ASSERT( sizeof(dst) >= sizeof(src) );
		//STATIC_ASSERT( std::is_trivial_v<T1> and std::is_trivial_v<T2> );	// TODO
		STATIC_ASSERT( not IsConst<T1> );

		std::memcpy( &dst, &src, sizeof(src) );
	}

	forceinline void  MemCopy (void *dst, BytesU dstSize, const void *src, BytesU srcSize)
	{
		ASSERT( srcSize <= dstSize );
		ASSERT( dst and src );

		std::memcpy( dst, src, size_t(std::min(srcSize, dstSize)) );
	}

/*
=================================================
	AllocOnStack
=================================================
*/
	ND_ forceinline void* AllocOnStack (BytesU size)
	{
	#ifdef PLATFORM_WINDOWS
		return _alloca( size_t(size) );
	#else
		return alloca( size_t(size) );
	#endif
	}


}	// FGC

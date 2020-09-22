// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"
#include "stl/Containers/Ptr.h"

namespace FGC
{

/*
=================================================
	Cast
=================================================
*/
	template <typename R, typename T>
	ND_ forceinline SharedPtr<R>  Cast (const SharedPtr<T> &other)
	{
		return std::static_pointer_cast<R>( other );
	}

/*
=================================================
	DynCast
=================================================
*/
	template <typename R, typename T>
	ND_ forceinline SharedPtr<R>  DynCast (const SharedPtr<T> &other)
	{
		return std::dynamic_pointer_cast<R>( other );
	}

/*
=================================================
	CheckPointerAlignment
=================================================
*/
	template <typename R, typename T>
	ND_ forceinline bool  CheckPointerAlignment (T const* ptr)
	{
		constexpr size_t	align = alignof(R);
		
		STATIC_ASSERT( ((align & (align - 1)) == 0), "Align must be power of 2" );

		return (size_t(ptr) & (align-1)) == 0;
	}

/*
=================================================
	Cast
=================================================
*/
	template <typename R, typename T>
	ND_ forceinline constexpr R const volatile*  Cast (T const volatile* value)
	{
		ASSERT( CheckPointerAlignment<R>( value ));
		return static_cast< R const volatile *>( static_cast< void const volatile *>(value) );
	}

	template <typename R, typename T>
	ND_ forceinline constexpr R volatile*  Cast (T volatile* value)
	{
		ASSERT( CheckPointerAlignment<R>( value ));
		return static_cast< R volatile *>( static_cast< void volatile *>(value) );
	}

	template <typename R, typename T>
	ND_ forceinline constexpr R const*  Cast (T const* value)
	{
		ASSERT( CheckPointerAlignment<R>( value ));
		return static_cast< R const *>( static_cast< void const *>(value) );
	}
	
	template <typename R, typename T>
	ND_ forceinline constexpr R*  Cast (T* value)
	{
		ASSERT( CheckPointerAlignment<R>( value ));
		return static_cast< R *>( static_cast< void *>(value) );
	}

	template <typename R, typename T>
	ND_ forceinline constexpr Ptr<R const>  Cast (Ptr<T const> value)
	{
		return Cast<R>( value.operator->() );
	}
	
	template <typename R, typename T>
	ND_ forceinline constexpr Ptr<R>  Cast (Ptr<T> value)
	{
		return Cast<R>( value.operator->() );
	}
	
/*
=================================================
	DynCast
=================================================
*/
	template <typename R, typename T>
	ND_ forceinline constexpr R const*  DynCast (T const* value)
	{
		return dynamic_cast< R const *>( value );
	}
	
	template <typename R, typename T>
	ND_ forceinline constexpr R*  DynCast (T* value)
	{
		return dynamic_cast< R *>( value );
	}

	template <typename R, typename T>
	ND_ forceinline constexpr Ptr<R const>  DynCast (Ptr<T const> value)
	{
		return DynCast<R>( value.operator->() );
	}
	
	template <typename R, typename T>
	ND_ forceinline constexpr Ptr<R>  DynCast (Ptr<T> value)
	{
		return DynCast<R>( value.operator->() );
	}

/*
=================================================
	MakeShared
=================================================
*/
	template <typename T, typename ...Types>
	ND_ forceinline SharedPtr<T>  MakeShared (Types&&... args)
	{
		return std::make_shared<T>( std::forward<Types&&>( args )... );
	}

/*
=================================================
	MakeUnique
=================================================
*/
	template <typename T, typename ...Types>
	ND_ forceinline UniquePtr<T>  MakeUnique (Types&&... args)
	{
		return std::make_unique<T>( std::forward<Types&&>( args )... );
	}
	
/*
=================================================
	BitCast
=================================================
*/
	template <typename To, typename From>
	ND_ inline constexpr To  BitCast (const From& src)
	{
		STATIC_ASSERT( sizeof(To) == sizeof(From), "must be same size!" );
		//STATIC_ASSERT( alignof(To) == alignof(From), "must be same align!" );
		//STATIC_ASSERT( std::is_trivially_copyable<From>::value and std::is_trivial<To>::value, "must be trivial types!" );

		To	dst;
		std::memcpy( OUT &dst, &src, sizeof(To) );
		return dst;
	}

/*
=================================================
	CheckCast
=================================================
*/
	template <typename To, typename From>
	ND_ inline constexpr To  CheckCast (const From& src)
	{
		if constexpr( std::is_signed_v<From> and std::is_unsigned_v<To> )
		{
			ASSERT( src >= 0 );
		}

		ASSERT( static_cast<From>(static_cast<To>(src)) == src );

		return static_cast<To>(src);
	}


}	// FGC

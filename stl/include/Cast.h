// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "stl/include/Common.h"

namespace FG
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
	Cast
=================================================
*/
	template <typename R, typename T>
	ND_ forceinline constexpr R const volatile*  Cast (T const volatile* value)
	{
		return static_cast< R const volatile *>( value );
	}

	template <typename R, typename T>
	ND_ forceinline constexpr R const*  Cast (T const* value)
	{
		return static_cast< R const *>( value );
	}
	
	template <typename R, typename T>
	ND_ forceinline constexpr R*  Cast (T* value)
	{
		return static_cast< R *>( value );
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
	BitCast
=================================================
*/
	template <typename To, typename From>
	ND_ inline constexpr To&  BitCast (From& from) noexcept
	{
		//STATIC_ASSERT( std::is_trivial_v<To> and std::is_trivial_v<From>, "must be trivial types!" );
		STATIC_ASSERT( sizeof(To) == sizeof(From), "must be same size!" );

		return reinterpret_cast< To& >( from );
	}

	template <typename To, typename From>
	ND_ inline constexpr const To&  BitCast (const From& from) noexcept
	{
		//STATIC_ASSERT( std::is_trivial_v<To> and std::is_trivial_v<From>, "must be trivial types!" );
		STATIC_ASSERT( sizeof(To) == sizeof(From), "must be same size!" );

		return reinterpret_cast< const To& >( from );
	}


}	// FG

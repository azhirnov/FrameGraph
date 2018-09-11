// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "stl/include/Vec.h"
#include "stl/include/StringUtils.h"

namespace FG
{

/*
=================================================
	ToString
=================================================
*/
	template <typename T>
	ND_ forceinline std::enable_if_t<not std::is_enum_v<T>, String>  ToString (const T &value)
	{
		return std::to_string( value );
	}

	template <typename E>
	ND_ forceinline std::enable_if_t<std::is_enum_v<E>, String>  ToString (const E &value)
	{
		using T = std::conditional_t< (sizeof(E) > sizeof(uint32_t)), uint32_t, uint64_t >;

		return std::to_string( T(value) );
	}

	ND_ forceinline String  ToString (const bool &value)
	{
		return value ? "true" : "false";
	}

/*
=================================================
	ToString (Vec)
=================================================
*/
	template <typename T, uint I>
	ND_ inline String  ToString (const Vec<T,I> &value)
	{
		String	str = "( ";

		for (uint i = 0; i < I; ++i)
		{
			if ( i > 0 )	str << ", ";
			str << ToString( value[i] );
		}
		str << " )";
		return str;
	}


}	// FG

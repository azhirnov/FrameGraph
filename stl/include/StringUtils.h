// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "stl/include/Math.h"
#include "stl/include/Vec.h"

namespace FG
{
	using namespace std::string_literals;

/*
=================================================
	operator << (String, String)
	operator << (String, StringView)
	operator << (String, CStyleString)
	operator << (String, char)
=================================================
*/
	template <typename T>
	forceinline std::basic_string<T>&&  operator << (std::basic_string<T> &&lhs, const std::basic_string<T> &rhs)
	{
		return std::move( std::move(lhs) += rhs );
	}

	template <typename T>
	forceinline std::basic_string<T>&  operator << (std::basic_string<T> &lhs, const std::basic_string<T> &rhs)
	{
		return (lhs += rhs);
	}

	template <typename T>
	forceinline std::basic_string<T>&&  operator << (std::basic_string<T> &&lhs, const std::basic_string_view<T> &rhs)
	{
		return std::move( std::move(lhs) += rhs.data() );
	}

	template <typename T>
	forceinline std::basic_string<T>&  operator << (std::basic_string<T> &lhs, const std::basic_string_view<T> &rhs)
	{
		return (lhs += rhs.data());
	}

	template <typename T>
	forceinline std::basic_string<T>&&  operator << (std::basic_string<T> &&lhs, T const * const rhs)
	{
		return std::move( std::move(lhs) += rhs );
	}

	template <typename T>
	forceinline std::basic_string<T>&  operator << (std::basic_string<T> &lhs, T const * const rhs)
	{
		return (lhs += rhs);
	}

	template <typename T>
	forceinline std::basic_string<T>&&  operator << (std::basic_string<T> &&lhs, const T rhs)
	{
		return std::move( std::move(lhs) += rhs );
	}

	template <typename T>
	forceinline std::basic_string<T>&  operator << (std::basic_string<T> &lhs, const T rhs)
	{
		return (lhs += rhs);
	}
	
/*
=================================================
	ToLower
=================================================
*/
namespace _fg_hidden_
{
	ND_ forceinline const char  ToLower (const char c)
	{
		return (c >= 'A' and c <= 'Z') ? c + ('a' - 'A') : c;
	}
}

/*
=================================================
	HasSubStringIC
----
	returns 'true' if 'str' has substring 'substr',
	comparison is case insansitive.
=================================================
*/
	ND_ inline bool  HasSubStringIC (StringView str, StringView substr)
	{
		using namespace _fg_hidden_;

		if ( str.empty() or substr.empty() )
			return false;

		for (size_t i = 0, j = 0; i < str.length(); ++i)
		{
			while ( ToLower( substr[j] ) == ToLower( str[i+j] ) and
					i+j < str.length() and j < substr.length() )
			{
				++j;
				if ( j >= substr.length() )
					return true;
			}
			j = 0;
		}
		return false;
	}
	
/*
=================================================
	StartsWithIC
----
	returns 'true' if 'str' starts with substring 'substr',
	comparison is case insansitive.
=================================================
*/
	ND_ inline bool  StartsWithIC (StringView str, StringView substr)
	{
		using namespace _fg_hidden_;

		if ( str.length() < substr.length() )
			return false;

		for (size_t i = 0; i < substr.length(); ++i)
		{
			if ( ToLower(str[i]) != ToLower(substr[i]) )
				return false;
		}
		return true;
	}

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

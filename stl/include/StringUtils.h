// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "stl/include/Math.h"

namespace FG
{
	using namespace std::string_literals;

/*
=================================================
	operator + (String, StringView)
	operator + (StringView, String)
	operator + (StringView, StringView)
=================================================
*
	template <typename T>
	ND_ forceinline std::basic_string<T>  operator + (std::basic_string<T> &&lhs, const std::basic_string_view<T> &rhs)
	{
		return std::move(lhs) + rhs.data();
	}

	template <typename T>
	ND_ forceinline std::basic_string<T>  operator + (const std::basic_string<T> &lhs, const std::basic_string_view<T> &rhs)
	{
		return lhs + rhs.data();
	}

	template <typename T>
	ND_ forceinline std::basic_string<T>  operator + (const std::basic_string_view<T> &lhs, const std::basic_string_view<T> &rhs)
	{
		return std::basic_string<T>(lhs) + rhs.data();
	}

	template <typename T>
	ND_ forceinline std::basic_string<T>  operator + (const std::basic_string_view<T> &lhs, const std::basic_string<T> &rhs)
	{
		return std::basic_string<T>(lhs) + rhs.data();
	}

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
	HasSubStringIC
----
	returns 'true' if 'str' has substring 'substr',
	comparison is case insansitive.
=================================================
*/
	ND_ inline bool  HasSubStringIC (StringView str, StringView substr)
	{
		const auto	to_lower =	[] (const char c) -> char
								{ return (c >= 'A' and c <= 'Z') ? c + ('a' - 'A') : c; };
		
		if ( str.empty() or substr.empty() )
			return false;

		for (size_t i = 0, j = 0; i < str.length(); ++i)
		{
			while ( to_lower( substr[j] ) == to_lower( str[i+j] ) and
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


}	// FG

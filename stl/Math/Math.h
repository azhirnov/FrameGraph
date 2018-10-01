// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace FG
{

/*
=================================================
	helpers
=================================================
*/
	namespace _fg_hidden_
	{
		template <typename T>
		static constexpr bool	IsInt	= std::is_integral_v<T> and std::is_signed_v<T>;
		
		template <typename T>
		static constexpr bool	IsUInt	= std::is_integral_v<T> and std::is_unsigned_v<T>;
		
		template <typename T1, typename T2, typename Result>
		using EnableForInt		= std::enable_if_t< IsInt<T1> and IsInt<T2>, Result >;
		
		template <typename T1, typename T2, typename Result>
		using EnableForUInt		= std::enable_if_t< IsUInt<T1> and IsUInt<T2>, Result >;

	}	// _fg_hidden_
	
/*
=================================================
	AdditionIsSafe
=================================================
*/
	template <typename T1, typename T2>
	ND_ inline _fg_hidden_::EnableForInt<T1, T2, bool>  AdditionIsSafe (const T1 a, const T2 b)
	{
		STATIC_ASSERT( std::is_scalar_v<T1> and std::is_scalar_v<T2> );

		using T = decltype(a + b);

		const T	x	= T(a);
		const T	y	= T(b);
		const T	min = std::numeric_limits<T>::min();
		const T	max = std::numeric_limits<T>::max();
		
		bool	overflow =	((y > 0) and (x > max - y))	or
							((y < 0) and (x < min - y));
		return not overflow;
	}

/*
=================================================
	AdditionIsSafe
=================================================
*/
	template <typename T1, typename T2>
	ND_ inline _fg_hidden_::EnableForUInt<T1, T2, bool>  AdditionIsSafe (const T1 a, const T2 b)
	{
		STATIC_ASSERT( std::is_scalar_v<T1> and std::is_scalar_v<T2> );
		
		using T = decltype(a + b);
		
		const T	x	= T(a);
		const T	y	= T(b);

		return (x + y) >= (x | y);
	}
	
/*
=================================================
	AlignToSmaller
=================================================
*/
	template <typename T>
	ND_ inline T  AlignToSmaller (const T &value, const T &align)
	{
		return (value / align) * align;
	}

/*
=================================================
	AlignToLarger
=================================================
*/
	template <typename T>
	ND_ inline T  AlignToLarger (const T &value, const T &align)
	{
		return ((value + align-1) / align) * align;
	}

/*
=================================================
	RoundToInt
=================================================
*/
	ND_ inline int  RoundToInt (float value)
	{
		return int(std::round( value ));
	}

/*
=================================================
	All
=================================================
*/
	ND_ inline constexpr bool  All (const bool &value)
	{
		return value;
	}
	
/*
=================================================
	Any
=================================================
*/
	ND_ inline constexpr bool  Any (const bool &value)
	{
		return value;
	}

/*
=================================================
	Max
=================================================
*/
	template <typename LT, typename RT>
	ND_ inline constexpr auto  Max (const LT &lhs, const RT &rhs)
	{
		using T = std::conditional_t< std::is_same_v<LT, RT>, LT, decltype(lhs + rhs) >;
		
		return lhs > rhs ? T(lhs) : T(rhs);
	}

/*
=================================================
	Min
=================================================
*/
	template <typename LT, typename RT>
	ND_ inline constexpr auto  Min (const LT &lhs, const RT &rhs)
	{
		using T = std::conditional_t< std::is_same_v<LT, RT>, LT, decltype(lhs + rhs) >;
		
		return lhs > rhs ? T(rhs) : T(lhs);
	}
	
/*
=================================================
	Clamp
=================================================
*/
	template <typename ValT, typename MinT, typename MaxT>
	ND_ inline constexpr auto  Clamp (const ValT &value, const MinT &minVal, const MaxT &maxVal)
	{
		ASSERT( minVal <= maxVal );
		return Min( maxVal, Max( value, minVal ) );
	}
	
/*
=================================================
	Equals
=================================================
*/
	template <typename T>
	ND_ inline constexpr std::enable_if_t<std::is_scalar_v<T>, bool>  Equals (const T &lhs, const T &rhs, const T &err = std::numeric_limits<T>::epsilon())
	{
		if constexpr ( std::is_integral_v<T> and std::is_unsigned_v<T> )
		{
			return lhs < rhs ? ((rhs - lhs) <= err) : ((lhs - rhs) <= err);
		}else
			return std::abs(lhs - rhs) <= err;
	}
	
/*
=================================================
	Abs
=================================================
*/
	template <typename T>
	ND_ inline constexpr T  Abs (const T &x)
	{
		return std::abs( x );
	}
	
/*
=================================================
	Fract
----
	GLSL-style fract wich return value in range 0..1
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr T  Fract (const T& x)
	{
		STATIC_ASSERT( std::is_scalar_v<T> and std::is_floating_point_v<T> );

		return x - std::floor(x);
	}

}	// FG

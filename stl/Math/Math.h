// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
		template <typename T1, typename T2, typename Result>
		using EnableForInt		= EnableIf< IsSignedInteger<T1> and IsSignedInteger<T2>, Result >;
		
		template <typename T1, typename T2, typename Result>
		using EnableForUInt		= EnableIf< IsUnsignedInteger<T1> and IsUnsignedInteger<T2>, Result >;

	}	// _fg_hidden_
	
/*
=================================================
	AdditionIsSafe
=================================================
*/
	template <typename T1, typename T2>
	ND_ forceinline constexpr _fg_hidden_::EnableForInt<T1, T2, bool>  AdditionIsSafe (const T1 a, const T2 b)
	{
		STATIC_ASSERT( IsScalar<T1> and IsScalar<T2> );

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
	ND_ forceinline constexpr _fg_hidden_::EnableForUInt<T1, T2, bool>  AdditionIsSafe (const T1 a, const T2 b)
	{
		STATIC_ASSERT( IsScalar<T1> and IsScalar<T2> );
		
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
	ND_ forceinline constexpr T  AlignToSmaller (const T &value, const T &align)
	{
		return (value / align) * align;
	}

/*
=================================================
	AlignToLarger
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr T  AlignToLarger (const T &value, const T &align)
	{
		return ((value + align-1) / align) * align;
	}

/*
=================================================
	RoundToInt
=================================================
*/
	ND_ forceinline int  RoundToInt (float value)
	{
		return int(std::round( value ));
	}

/*
=================================================
	All/Any
=================================================
*/
	ND_ forceinline constexpr bool  All (const bool &value)
	{
		return value;
	}
	
	ND_ forceinline constexpr bool  Any (const bool &value)
	{
		return value;
	}

/*
=================================================
	Max
=================================================
*/
	template <typename LT, typename RT>
	ND_ forceinline constexpr auto  Max (const LT &lhs, const RT &rhs)
	{
		using T = Conditional< IsSameTypes<LT, RT>, LT, decltype(lhs + rhs) >;
		
		return lhs > rhs ? T(lhs) : T(rhs);
	}

	template <typename T1, typename ...Types>
	ND_ forceinline constexpr auto  Max (const T1 &arg0, const Types& ...args)
	{
		return Max( arg0, Max( args... ));
	}
	
/*
=================================================
	Min
=================================================
*/
	template <typename LT, typename RT>
	ND_ forceinline constexpr auto  Min (const LT &lhs, const RT &rhs)
	{
		using T = Conditional< IsSameTypes<LT, RT>, LT, decltype(lhs + rhs) >;
		
		return lhs > rhs ? T(rhs) : T(lhs);
	}

	template <typename T1, typename ...Types>
	ND_ forceinline constexpr auto  Min (const T1 &arg0, const Types& ...args)
	{
		return Min( arg0, Min( args... ));
	}
	
/*
=================================================
	Clamp
=================================================
*/
	template <typename ValT, typename MinT, typename MaxT>
	ND_ forceinline constexpr auto  Clamp (const ValT &value, const MinT &minVal, const MaxT &maxVal)
	{
		ASSERT(All( minVal <= maxVal ));
		return Min( maxVal, Max( value, minVal ) );
	}
	
/*
=================================================
	Square
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr T  Square (const T &value)
	{
		return value * value;
	}

/*
=================================================
	Abs
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr EnableIf<IsScalar<T>, T>  Abs (const T &x)
	{
		return std::abs( x );
	}
	
/*
=================================================
	Equals
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr EnableIf<IsScalar<T>, bool>  Equals (const T &lhs, const T &rhs, const T &err = std::numeric_limits<T>::epsilon() * T(2))
	{
		if constexpr ( IsUnsignedInteger<T> )
		{
			return lhs < rhs ? ((rhs - lhs) <= err) : ((lhs - rhs) <= err);
		}else
			return Abs(lhs - rhs) <= err;
	}
	
/*
=================================================
	Floor
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr EnableIf<IsScalar<T> and IsFloatPoint<T>, T>  Floor (const T& x)
	{
		return std::floor( x );
	}
	
/*
=================================================
	Fract
----
	GLSL-style fract which returns value in range 0..1
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr T  Fract (const T& x)
	{
		return x - Floor( x );
	}

/*
=================================================
	IsIntersects
----
	1D intersection check
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr bool  IsIntersects (const T& begin1, const T& end1,
												  const T& begin2, const T& end2)
	{
		return (end1 > begin2) and (begin1 < end2);
	}

/*
=================================================
	GetIntersection
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr bool  GetIntersection (const T& begin1, const T& end1,
													 const T& begin2, const T& end2,
													 OUT T& outBegin, OUT T& outEnd)
	{
		outBegin = Max( begin1, begin2 );
		outEnd   = Min( end1, end2 );
		return outBegin < outEnd;
	}

/*
=================================================
	Lerp
----
	linear interpolation
=================================================
*/
	template <typename T, typename B>
	ND_ forceinline constexpr T  Lerp (const T& x, const T& y, const B& factor)
	{
		//return T(factor) * (y - x) + x;
		return x * (T(1) - T(factor)) + y * T(factor);
	}


}	// FG

// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace FGC
{

/*
=================================================
	helpers
=================================================
*/
	namespace _fgc_hidden_
	{
		template <typename T1, typename T2, typename Result>
		using EnableForInt		= EnableIf< IsSignedInteger<T1> and IsSignedInteger<T2>, Result >;
		
		template <typename T1, typename T2, typename Result>
		using EnableForUInt		= EnableIf< IsUnsignedInteger<T1> and IsUnsignedInteger<T2>, Result >;

	}	// _fgc_hidden_
	
/*
=================================================
	AdditionIsSafe
=================================================
*/
	template <typename T1, typename T2>
	ND_ forceinline constexpr _fgc_hidden_::EnableForInt<T1, T2, bool>  AdditionIsSafe (const T1 a, const T2 b)
	{
		STATIC_ASSERT( IsScalar<T1> and IsScalar<T2> );

		using T = decltype(a + b);

		const T	x	= T(a);
		const T	y	= T(b);
		const T	min = std::numeric_limits<T>::min();
		const T	max = std::numeric_limits<T>::max();
		
		bool	overflow =	((y > 0) & (x > max - y))	|
							((y < 0) & (x < min - y));
		return not overflow;
	}

/*
=================================================
	AdditionIsSafe
=================================================
*/
	template <typename T1, typename T2>
	ND_ forceinline constexpr _fgc_hidden_::EnableForUInt<T1, T2, bool>  AdditionIsSafe (const T1 a, const T2 b)
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
	template <typename T0, typename T1>
	ND_ forceinline constexpr auto  AlignToSmaller (const T0 &value, const T1 &align)
	{
		ASSERT( align > 0 );
		return (value / align) * align;
	}

/*
=================================================
	AlignToLarger
=================================================
*/
	template <typename T0, typename T1>
	ND_ forceinline constexpr auto  AlignToLarger (const T0 &value, const T1 &align)
	{
		ASSERT( align > 0 );
		return ((value + align-1) / align) * align;
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
	Sqrt
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr EnableIf<IsFloatPoint<T>, T>  Sqrt (const T &value)
	{
		return std::sqrt( value );
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
		if constexpr( IsUnsignedInteger<T> )
		{
			return lhs < rhs ? ((rhs - lhs) <= err) : ((lhs - rhs) <= err);
		}else
			return Abs(lhs - rhs) <= err;
	}
	
/*
=================================================
	Floor / Ceil / Trunc
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr EnableIf<IsScalar<T> and IsFloatPoint<T>, T>  Floor (const T& x)
	{
		return std::floor( x );
	}
	
	template <typename T>
	ND_ forceinline constexpr EnableIf<IsScalar<T> and IsFloatPoint<T>, T>  Ceil (const T& x)
	{
		return std::ceil( x );
	}
	
	template <typename T>
	ND_ forceinline constexpr EnableIf<IsScalar<T> and IsFloatPoint<T>, T>  Trunc (const T& x)
	{
#	if 1
		return std::trunc( x );
#	else
		return x > T(0) ? Floor(x) : Ceil(x);
#	endif
	}
	
/*
=================================================
	Round / RoundToInt / RoundToUint
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr EnableIf<IsScalar<T> and IsFloatPoint<T>, T>  Round (const T& x)
	{
		return std::round( x );
	}

	template <typename T>
	ND_ forceinline constexpr auto  RoundToInt (const T& x)
	{
		STATIC_ASSERT( IsFloatPoint<T> );
		
		if constexpr( sizeof(T) >= sizeof(int64_t) )
			return int64_t(std::round( x ));

		if constexpr( sizeof(T) >= sizeof(int32_t) )
			return int32_t(std::round( x ));
	}

	template <typename T>
	ND_ forceinline constexpr auto  RoundToUint (const T& x)
	{
		STATIC_ASSERT( IsFloatPoint<T> );
		
		if constexpr( sizeof(T) >= sizeof(uint64_t) )
			return uint64_t(std::round( x ));

		if constexpr( sizeof(T) >= sizeof(uint32_t) )
			return uint32_t(std::round( x ));
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
		return (end1 > begin2) & (begin1 < end2);
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
	ND_ forceinline constexpr EnableIf<IsScalar<T>, T>  Lerp (const T& x, const T& y, const B& factor)
	{
		//return T(factor) * (y - x) + x;
		return x * (T(1) - T(factor)) + y * T(factor);
	}
	
/*
=================================================
	SafeDiv
=================================================
*/
	template <typename T1, typename T2, typename T3>
	ND_ forceinline constexpr auto  SafeDiv (const T1& lhs, const T2& rhs, const T3& defVal)
	{
		STATIC_ASSERT( IsScalarOrEnum<T1> and IsScalarOrEnum<T2> and IsScalarOrEnum<T3> );

		using T = decltype( lhs + rhs + defVal );

		return not Equals( rhs, T(0) ) ? (T(lhs) / T(rhs)) : T(defVal);
	}
	
	template <typename T1, typename T2>
	ND_ forceinline constexpr auto  SafeDiv (const T1& lhs, const T2& rhs)
	{
		return SafeDiv( lhs, rhs, T1(0) );
	}

/*
=================================================
	Ln / Log / Log2 / Log10
=================================================
*/
	template <typename T>
	ND_ forceinline EnableIf<IsFloatPoint<T>, T>  Ln (const T& x)
	{
		return std::log( x );
	}

	template <typename T>
	ND_ forceinline EnableIf<IsFloatPoint<T>, T>  Log2 (const T& x)
	{
		return std::log2( x );
	}

	template <typename T>
	ND_ forceinline EnableIf<IsFloatPoint<T>, T>  Log10 (const T& x)
	{
		return std::log10( x );
	}

	template <auto Base, typename T>
	ND_ forceinline EnableIf<IsFloatPoint<T>, T>  Log (const T& x)
	{
		static constexpr auto log_base = std::log( Base );
		return std::log( x ) / log_base;
	}
	
/*
=================================================
	Wrap
=================================================
*/
	template <typename T>
	forceinline EnableIf<IsFloatPoint<T>, T>  Wrap (const T& value, const T& minValue, const T& maxValue)
	{
		// check for NaN
		if ( minValue >= maxValue )
			return minValue;

		T	result = T( minValue + std::fmod( value - minValue, maxValue - minValue ));
		
		if ( result < minValue )
			result += (maxValue - minValue);

		return result;
	}

}	// FGC

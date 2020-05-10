#pragma once

#include "stl/Algorithms/EnumUtils.h"

#ifdef COMPILER_MSVC
# include <intrin.h>
# pragma intrinsic( _BitScanForward, _BitScanReverse )
# if PLATFORM_BITS == 64
#	pragma intrinsic( _BitScanForward64, _BitScanReverse64 )
# endif
#endif
	
namespace FGC
{

/*
=================================================
	IsPowerOfTwo
=================================================
*/
	template <typename T>
	ND_ forceinline constexpr bool  IsPowerOfTwo (const T &x)
	{
		STATIC_ASSERT( IsInteger<T> or IsEnum<T> );

		return (x != 0) & ((x & (x - T(1))) == T(0));
	}
	
/*
=================================================
	IntLog2 / GetPowerOfTwo / BitScanReverse
=================================================
*/
	template <typename T>
	ND_ forceinline int  IntLog2 (const T& x)
	{
		STATIC_ASSERT( IsInteger<T> or IsEnum<T> );

		constexpr int	INVALID_INDEX = std::numeric_limits<int>::min();

	#ifdef COMPILER_MSVC
		unsigned long	index;
		
		if constexpr( sizeof(x) == 8 )
			return _BitScanReverse64( OUT &index, uint64_t(x) ) ? index : INVALID_INDEX;
		else
		if constexpr( sizeof(x) <= 4 )
			return _BitScanReverse( OUT &index, uint(x) ) ? index : INVALID_INDEX;
		
	#elif defined(COMPILER_GCC) or defined(COMPILER_CLANG)
		if constexpr( sizeof(x) == 8 )
			return x ? (sizeof(x)*8)-1 - __builtin_clzll( uint64_t(x) ) : INVALID_INDEX;
		else
		if constexpr( sizeof(x) <= 4 )
			return x ? (sizeof(x)*8)-1 - __builtin_clz( uint(x) ) : INVALID_INDEX;

	#else
		#error add BitScanReverse implementation
	#endif
	}

	template <typename T>
	ND_ forceinline int  BitScanReverse (const T& x)
	{
		return IntLog2( x );
	}
	
/*
=================================================
	BitScanForward
=================================================
*/
	template <typename T>
	ND_ forceinline int  BitScanForward (const T& x)
	{
		STATIC_ASSERT( IsInteger<T> or IsEnum<T> );

	#ifdef COMPILER_MSVC
		constexpr int	INVALID_INDEX = std::numeric_limits<int>::min();
		unsigned long	index;
		
		if constexpr( sizeof(x) == 8 )
			return _BitScanForward64( OUT &index, uint64_t(x) ) ? index : INVALID_INDEX;
		else
		if constexpr( sizeof(x) <= 4 )
			return _BitScanForward( OUT &index, uint(x) ) ? index : INVALID_INDEX;
		
	#elif defined(COMPILER_GCC) or defined(COMPILER_CLANG)
		if constexpr( sizeof(x) == 8 )
			return __builtin_ffsll( uint64_t(x) ) - 1;
		else
		if constexpr( sizeof(x) <= 4 )
			return __builtin_ffs( uint(x) ) - 1;

	#else
		#error add BitScanForward implementation
	#endif
	}
	
/*
=================================================
	BitCount
=================================================
*/
	template <typename T>
	ND_ forceinline size_t  BitCount (const T& x)
	{
		STATIC_ASSERT( IsInteger<T> or IsEnum<T> );

		if constexpr( sizeof(x) == 8 )
			return std::bitset<64>{ uint64_t(x) }.count();
		else
		if constexpr( sizeof(x) <= 4 )
			return std::bitset<32>{ uint(x) }.count();
	}


}	// FGC

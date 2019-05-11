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
		return ( x != 0 and ( (x & (x - T(1))) == T(0) ) );
	}
	
/*
=================================================
	IntLog2 / GetPowerOfTwo / BitScanReverse
=================================================
*/
	template <typename T>
	ND_ forceinline int  IntLog2 (const T& x)
	{
		constexpr int	INVALID_INDEX = std::numeric_limits<int>::min();

	#ifdef COMPILER_MSVC
		unsigned long	index;

		if constexpr ( sizeof(x) > sizeof(uint) )
			return _BitScanReverse64( OUT &index, x ) ? index : INVALID_INDEX;
		else
			return _BitScanReverse( OUT &index, x ) ? index : INVALID_INDEX;
		
	#elif defined(COMPILER_GCC) or defined(COMPILER_CLANG)
		if constexpr ( sizeof(x) > sizeof(uint) )
			return x ? (sizeof(x)*8)-1 - __builtin_clzll( x ) : INVALID_INDEX;
		else
			return x ? (sizeof(x)*8)-1 - __builtin_clz( x ) : INVALID_INDEX;

	#else
		//return std::ilogb( x );
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
	#ifdef COMPILER_MSVC
		constexpr int	INVALID_INDEX = std::numeric_limits<int>::min();
		unsigned long	index;

		if constexpr ( sizeof(x) > sizeof(uint) )
			return _BitScanForward64( OUT &index, x ) ? index : INVALID_INDEX;
		else
			return _BitScanForward( OUT &index, x ) ? index : INVALID_INDEX;
		
	#elif defined(COMPILER_GCC) or defined(COMPILER_CLANG)
		if constexpr ( sizeof(x) > sizeof(uint) )
			return __builtin_ffsll( x ) - 1;
		else
			return __builtin_ffs( x ) - 1;

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
		STATIC_ASSERT( IsUnsignedInteger<T> );

		if constexpr( sizeof(x) == 8 )
			return std::bitset<64>{ x }.count();
		else
		if constexpr( sizeof(x) <= 4 )
			return std::bitset<32>{ x }.count();
	}


}	// FGC

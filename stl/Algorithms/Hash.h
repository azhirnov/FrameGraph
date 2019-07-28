// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include <functional>
#include "stl/Log/Log.h"
#include "stl/CompileTime/TypeTraits.h"

namespace FGC
{

	//
	// Hash Value
	//

	struct HashVal
	{
	// variables
	private:
		size_t		_value	= 0;

	// methods
	public:
		constexpr HashVal () {}
		explicit constexpr HashVal (size_t val) : _value{val} {}

		ND_ constexpr bool	operator == (const HashVal &rhs)	const	{ return _value == rhs._value; }
		ND_ constexpr bool	operator != (const HashVal &rhs)	const	{ return not (*this == rhs); }
		ND_ constexpr bool	operator >  (const HashVal &rhs)	const	{ return _value > rhs._value; }
		ND_ constexpr bool  operator <  (const HashVal &rhs)	const	{ return _value < rhs._value; }

		constexpr HashVal&	operator << (const HashVal &rhs)
		{
			const size_t	mask	= (sizeof(_value)*8 - 1);
			size_t			val		= rhs._value;
			size_t			shift	= 8;

			shift &= mask;
			_value ^= (val << shift) | (val >> ( ~(shift-1) & mask ));

			return *this;
		}

		ND_ constexpr const HashVal  operator + (const HashVal &rhs) const
		{
			return HashVal(*this) << rhs;
		}

		ND_ explicit constexpr operator size_t () const	{ return _value; }
	};
//-----------------------------------------------------------------------------


	
/*
=================================================
	HashOf
=================================================
*/
	template <typename T>
    ND_ forceinline EnableIf<not IsFloatPoint<T>, HashVal>  HashOf (const T &value)
	{
		return HashVal( std::hash<T>()( value ));
	}

/*
=================================================
	HashOf (float)
=================================================
*/
    ND_ forceinline HashVal  HashOf (const float &value, uint32_t ignoreMantissaBits = (23-10))
	{
		ASSERT( ignoreMantissaBits < 23 );
		return HashVal( std::hash<uint32_t>()( reinterpret_cast< const uint32_t &>(value) & ~((1 << ignoreMantissaBits)-1) ));
	}

/*
=================================================
	HashOf (double)
=================================================
*/
    ND_ forceinline HashVal  HashOf (const double &value, uint32_t ignoreMantissaBits = (52-10))
	{
		ASSERT( ignoreMantissaBits < 52 );
		return HashVal( std::hash<uint64_t>()( reinterpret_cast< const uint64_t &>(value) & ~((1 << ignoreMantissaBits)-1) ));
	}

/*
=================================================
	HashOf (buffer)
----
	use private api to calculate hash of buffer
=================================================
*/
    ND_ forceinline HashVal  HashOf (const void *ptr, size_t sizeInBytes)
	{
		ASSERT( ptr and sizeInBytes );

		# if defined(FG_HAS_HASHFN_HashArrayRepresentation)
			return HashVal{std::_Hash_array_representation( static_cast<const unsigned char*>(ptr), sizeInBytes )};

        #elif defined(FG_HAS_HASHFN_Murmur2OrCityhash)
			return HashVal{std::__murmur2_or_cityhash<size_t>()( ptr, sizeInBytes )};

		#elif defined(FG_HAS_HASHFN_HashBytes)
			return HashVal{std::_Hash_bytes( ptr, sizeInBytes, 0 )};

		#else
			FG_COMPILATION_MESSAGE( "used fallback hash function" )
			const uint8_t*	buf		= static_cast<const uint8_t*>(ptr);
			HashVal			result;
			for (size_t i = 0; i < sizeInBytes; ++i) {
				result << HashVal{buf[i]};
			}
			return result;
		#endif
	}

}	// FGC


namespace std
{
	template <typename First, typename Second>
	struct hash< std::pair<First, Second> >
	{
        ND_ size_t  operator () (const std::pair<First, Second> &value) const
		{
			return size_t(FGC::HashOf( value.first ) + FGC::HashOf( value.second ));
		}
	};

}	// std

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include <functional>
#include "stl/Log/Log.h"
#include "stl/CompileTime/TypeTraits.h"

namespace FG
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
			size_t			shift	= 1;

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
	ND_ forceinline EnableIf<not IsFloatPoint<T>, HashVal>  HashOf (const T &value) noexcept
	{
		return HashVal( std::hash<T>()( value ));
	}

/*
=================================================
	HashOf (float)
=================================================
*/
	ND_ forceinline HashVal  HashOf (const float &value, uint32_t ignoreMantissaBits = (23-10)) noexcept
	{
		ASSERT( ignoreMantissaBits < 23 );
		return HashVal( std::hash<uint32_t>()( reinterpret_cast< const uint32_t &>(value) & ~((1 << ignoreMantissaBits)-1) ));
	}

/*
=================================================
	HashOf (double)
=================================================
*/
	ND_ forceinline HashVal  HashOf (const double &value, uint32_t ignoreMantissaBits = (52-10)) noexcept
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
	ND_ forceinline HashVal  HashOf (const void *ptr, size_t sizeInBytes) noexcept
	{
		ASSERT( ptr and sizeInBytes );

		// MS Visual C++ std implementation
		#if defined(COMPILER_MSVC)
		# if defined(_HASH_SEQ_DEFINED)
			return HashVal{std::_Hash_seq( static_cast<const unsigned char*>(ptr), sizeInBytes )};
		# elif _MSC_VER >= 1916
			return HashVal{std::_Hash_array_representation( static_cast<const unsigned char*>(ptr), sizeInBytes )};
		# elif _MSC_VER >= 1911
			return HashVal{std::_Hash_bytes( static_cast<const unsigned char*>(ptr), sizeInBytes )};
		# endif

		#elif defined(COMPILER_CLANG)
			return HashVal{std::__murmur2_or_cityhash<size_t>()( ptr, sizeInBytes )};
		#elif defined(COMPILER_GCC)
			return HashVal{std::_Hash_bytes( ptr, sizeInBytes, 0 )};
		#else
			#error "hash function not defined!"
		#endif
	}

}	// FG


namespace std
{
	template <typename First, typename Second>
	struct hash< std::pair<First, Second> >
	{
		ND_ size_t  operator () (const std::pair<First, Second> &value) const noexcept
		{
			return size_t(FG::HashOf( value.first ) + FG::HashOf( value.second ));
		}
	};

}	// std

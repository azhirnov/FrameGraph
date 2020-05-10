// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	//
	// Mipmap Level
	//
	
	struct MipmapLevel
	{
	// variables
	private:
		uint		_value;


	// methods
	public:
		constexpr MipmapLevel () : _value{0} {}

		explicit constexpr MipmapLevel (uint value) : _value(value) {}
		explicit constexpr MipmapLevel (uint64_t value) : _value(CheckCast<uint>(value)) {}

		ND_ constexpr uint	Get ()								 const		{ return _value; }
		
		ND_ constexpr bool	operator == (const MipmapLevel &rhs) const		{ return _value == rhs._value; }
		ND_ constexpr bool	operator != (const MipmapLevel &rhs) const		{ return _value != rhs._value; }
		ND_ constexpr bool	operator >  (const MipmapLevel &rhs) const		{ return _value >  rhs._value; }
		ND_ constexpr bool	operator <  (const MipmapLevel &rhs) const		{ return _value <  rhs._value; }
		ND_ constexpr bool	operator >= (const MipmapLevel &rhs) const		{ return _value >= rhs._value; }
		ND_ constexpr bool	operator <= (const MipmapLevel &rhs) const		{ return _value <= rhs._value; }
	};

	
	ND_ inline constexpr MipmapLevel operator "" _mipmap (unsigned long long value)		{ return MipmapLevel( CheckCast<uint>(value) ); }


}	// FG


namespace std
{
	template <>
	struct hash< FG::MipmapLevel >
	{
		ND_ size_t  operator () (const FG::MipmapLevel &value) const
		{
			return size_t(FGC::HashOf( value.Get() ));
		}
	};

}	// std

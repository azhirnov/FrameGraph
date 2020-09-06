// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	//
	// Image Array Layer
	//
	
	struct ImageLayer
	{
	// variables
	private:
		uint		_value	= 0;


	// methods
	public:
		constexpr ImageLayer () {}
		explicit constexpr ImageLayer (uint value) : _value{ value } {}
		explicit constexpr ImageLayer (uint64_t value) : _value{ CheckCast<uint>( value )} {}

		ND_ constexpr uint	Get ()								const	{ return _value; }
		
		ND_ ImageLayer		operator + (const ImageLayer &rhs)	const	{ return ImageLayer{ Get() + rhs.Get() }; }

		ND_ constexpr bool	operator == (const ImageLayer &rhs) const	{ return _value == rhs._value; }
		ND_ constexpr bool	operator != (const ImageLayer &rhs) const	{ return _value != rhs._value; }
		ND_ constexpr bool	operator >  (const ImageLayer &rhs) const	{ return _value >  rhs._value; }
		ND_ constexpr bool	operator <  (const ImageLayer &rhs) const	{ return _value <  rhs._value; }
		ND_ constexpr bool	operator >= (const ImageLayer &rhs) const	{ return _value >= rhs._value; }
		ND_ constexpr bool	operator <= (const ImageLayer &rhs) const	{ return _value <= rhs._value; }
	};
	

	ND_ inline constexpr ImageLayer operator "" _layer (unsigned long long value)	{ return ImageLayer{ uint64_t(value) }; }


}	// FG


namespace std
{
	template <>
	struct hash< FG::ImageLayer >
	{
		ND_ size_t  operator () (const FG::ImageLayer &value) const
		{
			return size_t(FGC::HashOf( value.Get() ));
		}
	};

}	// std

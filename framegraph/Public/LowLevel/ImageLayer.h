// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "framegraph/Public/LowLevel/Types.h"

namespace FG
{

	//
	// Image Array Layer
	//
	
	struct ImageLayer
	{
	// variables
	private:
		uint		_value;


	// methods
	public:
		constexpr ImageLayer () : _value(~0u) {}

		explicit constexpr ImageLayer (uint value) : _value(value) {}

		ND_ constexpr bool	IsDefined ()						const	{ return _value != ~0u; }

		ND_ constexpr uint	Get ()								const	{ return IsDefined() ? _value : 0; }
		
		ND_ ImageLayer		operator + (const ImageLayer &rhs)	const	{ return ImageLayer{ Get() + rhs.Get() }; }

		ND_ constexpr bool	operator == (const ImageLayer &rhs) const	{ return _value == rhs._value; }
		ND_ constexpr bool	operator != (const ImageLayer &rhs) const	{ return _value != rhs._value; }
		ND_ constexpr bool	operator >  (const ImageLayer &rhs) const	{ return _value >  rhs._value; }
		ND_ constexpr bool	operator <  (const ImageLayer &rhs) const	{ return _value <  rhs._value; }
		ND_ constexpr bool	operator >= (const ImageLayer &rhs) const	{ return _value >= rhs._value; }
		ND_ constexpr bool	operator <= (const ImageLayer &rhs) const	{ return _value <= rhs._value; }
	};
	

	ND_ inline constexpr ImageLayer operator "" _layer (unsigned long long value)	{ return ImageLayer( uint(value) ); }


}	// FG


namespace std
{
	template <>
	struct hash< FG::ImageLayer >
	{
		ND_ size_t  operator () (const FG::ImageLayer &value) const noexcept
		{
			return size_t(FG::HashOf( value.Get() ));
		}
	};

}	// std

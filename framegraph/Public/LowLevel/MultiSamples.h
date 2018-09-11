// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "framegraph/Public/LowLevel/Types.h"

namespace FG
{

	//
	// Multi Samples
	//
	
	struct MultiSamples
	{
	// variables
	private:
		uint	_value;


	// methods
	public:
		constexpr MultiSamples () : _value(0) {}

		explicit MultiSamples (uint samples) : _value( std::ilogb( samples ) ) {}

		ND_ constexpr uint	Get ()								const		{ return 1u << _value; }
		ND_ constexpr uint	GetPowerOf2 ()						const		{ return _value; }

		ND_ constexpr bool	IsEnabled ()						const		{ return _value > 0; }

		ND_ constexpr bool	operator == (const MultiSamples &rhs) const		{ return _value == rhs._value; }
		ND_ constexpr bool	operator != (const MultiSamples &rhs) const		{ return _value != rhs._value; }
		ND_ constexpr bool	operator >  (const MultiSamples &rhs) const		{ return _value >  rhs._value; }
		ND_ constexpr bool	operator <  (const MultiSamples &rhs) const		{ return _value <  rhs._value; }
		ND_ constexpr bool	operator >= (const MultiSamples &rhs) const		{ return _value >= rhs._value; }
		ND_ constexpr bool	operator <= (const MultiSamples &rhs) const		{ return _value <= rhs._value; }
	};
	

	ND_ inline MultiSamples operator "" _samples (unsigned long long value)		{ return MultiSamples( uint(value) ); }


}	// FG


namespace std
{
	template <>
	struct hash< FG::MultiSamples >
	{
		ND_ size_t  operator () (const FG::MultiSamples &value) const noexcept
		{
			return size_t(FG::HashOf( value.Get() ));
		}
	};

}	// std

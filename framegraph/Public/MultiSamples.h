// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	//
	// Multi Samples
	//
	
	struct MultiSamples
	{
	// variables
	private:
		uint8_t		_value;


	// methods
	public:
		constexpr MultiSamples () : _value{0} {}

		explicit MultiSamples (uint samples) : _value{ CheckCast<uint8_t>( IntLog2( samples ))} {}
		explicit MultiSamples (uint64_t samples) : _value{ CheckCast<uint8_t>( IntLog2( samples ))} {}

		ND_ constexpr uint		Get ()							const		{ return 1u << _value; }
		ND_ constexpr uint8_t	GetPowerOf2 ()					const		{ return _value; }

		ND_ constexpr bool		IsEnabled ()					const		{ return _value > 0; }

		ND_ constexpr bool	operator == (const MultiSamples &rhs) const		{ return _value == rhs._value; }
		ND_ constexpr bool	operator != (const MultiSamples &rhs) const		{ return _value != rhs._value; }
		ND_ constexpr bool	operator >  (const MultiSamples &rhs) const		{ return _value >  rhs._value; }
		ND_ constexpr bool	operator <  (const MultiSamples &rhs) const		{ return _value <  rhs._value; }
		ND_ constexpr bool	operator >= (const MultiSamples &rhs) const		{ return _value >= rhs._value; }
		ND_ constexpr bool	operator <= (const MultiSamples &rhs) const		{ return _value <= rhs._value; }
	};
	

	ND_ inline MultiSamples operator "" _samples (unsigned long long value)	{ return MultiSamples{ uint64_t(value) }; }


}	// FG


namespace std
{
	template <>
	struct hash< FG::MultiSamples >
	{
		ND_ size_t  operator () (const FG::MultiSamples &value) const
		{
			return size_t(FGC::HashOf( value.Get() ));
		}
	};

}	// std

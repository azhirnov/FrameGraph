// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	//
	// Image Color Swizzle
	//

	struct ImageSwizzle
	{
	// variables
	private:
		uint	_value	= 0x4321;	// 0 - unknown, 1 - R, 2 - G, 3 - B, 4 - A, 5 - O, 6 - 1, example: RGB0 - 0x1235


	// methods
	public:
		constexpr ImageSwizzle ()
		{}

		explicit constexpr ImageSwizzle (const uint4 &comp) : _value{0}
		{
			ASSERT(All( comp < 7 ));

			_value |= (comp.x & 0xF);
			_value |= (comp.y & 0xF) << 4;
			_value |= (comp.z & 0xF) << 8;
			_value |= (comp.w & 0xF) << 12;
		}

		ND_ constexpr uint Get () const
		{
			return _value;
		}

		ND_ constexpr uint4 ToVec () const
		{
			return uint4( _value & 0xF, (_value >> 4) & 0xF, (_value >> 8) & 0xF, (_value >> 12) & 0xF );
		}

		ND_ constexpr bool IsDefault () const
		{
			return _value == ImageSwizzle().Get();
		}

		ND_ constexpr bool operator == (const ImageSwizzle &rhs) const	{ return _value == rhs._value; }
		ND_ constexpr bool operator >  (const ImageSwizzle &rhs) const	{ return _value >  rhs._value; }


		friend constexpr ImageSwizzle  operator "" _swizzle (const char *str, const size_t len) noexcept;


	private:
		static constexpr uint _CharToValue (char c) noexcept
		{
			return	c == 'r' or c == 'R'	? 1 :
					c == 'g' or c == 'G'	? 2 :
					c == 'b' or c == 'B'	? 3 :
					c == 'a' or c == 'A'	? 4 :
					c == '0'				? 5 :
					c == '1'				? 6 : 0;
		}
	};
	

	ND_ constexpr ImageSwizzle  operator "" _swizzle (const char *str, const size_t len) noexcept
	{
		ASSERT( len > 0 and len <= 4 );

		ImageSwizzle	res;
		res._value = 0;

		for (size_t i = 0; i < len; ++i)
		{
			const char	c = str[i];
			const uint	v = ImageSwizzle::_CharToValue( c );

			ASSERT( v != 0 );	// 'c' must be R, G, B, A, 0, 1
			res._value |= (v << i*4);
		}
		return res;
	}

}	// FG


namespace std
{
	template <>
	struct hash< FG::ImageSwizzle >
	{
		ND_ size_t  operator () (const FG::ImageSwizzle &value) const noexcept
		{
			return size_t(FGC::HashOf( value.Get() == 0 ? FG::ImageSwizzle().Get() : value.Get() ));
		}
	};

}	// std

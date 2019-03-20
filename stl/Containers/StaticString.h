// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"
#include "stl/Containers/StringView.h"

namespace FGC
{

	//
	// Static String
	//

	template <typename CharT, size_t StringSize>
	struct TStaticString
	{
	// type
	public:
		using iterator			= CharT *;
		using const_iterator	= CharT const *;
		using View_t			= BasicStringView< CharT >;
		using Self				= TStaticString< CharT, StringSize >;


	// variables
	private:
		CharT		_array [StringSize] = {};
		size_t		_length	= 0;


	// methods
	public:
		constexpr TStaticString ()
		{
			DEBUG_ONLY( ::memset( _array, 0, sizeof(_array) ));
		}
		
		TStaticString (const View_t &view) : TStaticString{ view.data(), view.length() }
		{}
		
		constexpr TStaticString (const CharT *str)
		{
			for (; str[_length] and _length < StringSize; ++_length) {
				_array[_length] = str[_length];
			}
			_array[_length] = CharT(0);
		}

		constexpr TStaticString (const CharT *str, size_t length)
		{
			ASSERT( length < StringSize );
			
			for (; _length < length and _length < StringSize; ++_length) {
				_array[_length] = str[_length];
			}
			_array[_length] = CharT(0);
		}


		ND_ constexpr operator View_t ()					const	{ return View_t{ _array, length() }; }

		ND_ constexpr size_t		size ()					const	{ return _length; }
		ND_ constexpr size_t		length ()				const	{ return size(); }
		ND_ constexpr size_t		capacity ()				const	{ return StringSize; }
		ND_ constexpr bool			empty ()				const	{ return _length == 0; }
		ND_ constexpr CharT const *	c_str ()				const	{ return _array; }
		ND_ constexpr CharT const *	data ()					const	{ return _array; }

		ND_ constexpr CharT &		operator [] (size_t i)			{ ASSERT( i < _length );  return _array[i]; }
		ND_ constexpr CharT const &	operator [] (size_t i)	const	{ ASSERT( i < _length );  return _array[i]; }

		ND_ bool	operator == (const View_t &rhs)			const	{ return View_t(*this) == rhs; }
		ND_ bool	operator != (const View_t &rhs)			const	{ return not (*this == rhs); }
		ND_ bool	operator >  (const View_t &rhs)			const	{ return View_t(*this) > rhs; }
		ND_ bool	operator <  (const View_t &rhs)			const	{ return View_t(*this) < rhs; }

		ND_ iterator		begin ()								{ return &_array[0]; }
		ND_ const_iterator	begin ()						const	{ return &_array[0]; }
		ND_ iterator		end ()									{ return &_array[_length]; }
		ND_ const_iterator	end ()							const	{ return &_array[_length]; }


		void clear ()
		{
			_array[0]	= CharT(0);
			_length		= 0;
		}
	};


	template <size_t StringSize>
	using StaticString = TStaticString< char, StringSize >;

}	// FGC


namespace std
{

	template <typename CharT, size_t StringSize>
	struct hash< FGC::TStaticString< CharT, StringSize > >
	{
		ND_ size_t  operator () (const FGC::TStaticString<CharT, StringSize> &value) const noexcept
		{
			return hash< FGC::BasicStringView<CharT> >()( value );
		}
	};

}	// std

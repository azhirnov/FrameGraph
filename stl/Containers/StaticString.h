// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"

namespace FG
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
		using View_t			= std::basic_string_view< CharT >;
		using Self				= TStaticString< CharT, StringSize >;


	// variables
	private:
		std::array< CharT, StringSize >	_array;
		size_t							_length	= 0;


	// methods
	public:
		TStaticString () {}

		TStaticString (const View_t &view)
		{
			ASSERT( view.size() < StringSize );

			_length = Min( view.size(), StringSize-1 );

			::memcpy( _array.data(), view.data(), _length * sizeof(CharT) );

			_array[_length] = CharT(0);
		}

		TStaticString (const CharT *str) : TStaticString{ View_t{str} }
		{}


		ND_ operator View_t ()						const	{ return View_t{ _array.data(), length() }; }

		ND_ size_t			size ()					const	{ return _length; }
		ND_ size_t			length ()				const	{ return size(); }
		ND_ size_t			capacity ()				const	{ return StringSize; }
		ND_ bool			empty ()				const	{ return _length == 0; }
		ND_ CharT const *	c_str ()				const	{ return _array.data(); }
		ND_ CharT const *	data ()					const	{ return _array.data(); }

		ND_ CharT &			operator [] (size_t i)			{ ASSERT( i < _length );  return _array[i]; }
		ND_ CharT const &	operator [] (size_t i)	const	{ ASSERT( i < _length );  return _array[i]; }

		ND_ bool	operator == (const View_t &rhs)	const	{ return View_t(*this) == rhs; }
		ND_ bool	operator != (const View_t &rhs)	const	{ return not (*this == rhs); }
		ND_ bool	operator >  (const View_t &rhs)	const	{ return View_t(*this) > rhs; }
		ND_ bool	operator <  (const View_t &rhs)	const	{ return View_t(*this) < rhs; }

		ND_ iterator		begin ()						{ return _array.begin(); }
		ND_ const_iterator	begin ()				const	{ return _array.begin(); }
		ND_ iterator		end ()							{ return _array.begin() + _length; }
		ND_ const_iterator	end ()					const	{ return _array.begin() + _length; }


		void clear ()
		{
			_array[0]	= CharT(0);
			_length		= 0;
		}
	};


    template <size_t StringSize>
	using StaticString = TStaticString< char, StringSize >;

}	// FG


namespace std
{

    template <typename CharT, size_t StringSize>
	struct hash< FG::TStaticString< CharT, StringSize > >
	{
		ND_ size_t  operator () (const FG::TStaticString<CharT, StringSize> &value) const noexcept
		{
			return hash< std::basic_string_view<CharT> >()( value );
		}
	};

}	// std

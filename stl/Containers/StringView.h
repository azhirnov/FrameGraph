// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/StringViewFwd.h"
#include "stl/Common.h"

#ifndef FG_STD_STRINGVIEW

namespace FG
{

	//
	// String View
	//

	template <typename T>
	struct BasicStringView
	{
	// types
	public:
		using size_type		= size_t;
		using value_type	= T;
		using String_t		= std::basic_string<T>;


	// variables
	private:
		T const *	_data		= null;
		size_t		_length		= 0;


	// methods
	public:
		constexpr BasicStringView () {}

		constexpr BasicStringView (const T* ptr) : _data{ptr}, _length{ptr ? std::strlen(ptr) : 0} {}
		constexpr BasicStringView (const T* ptr, size_t len) : _data{ptr}, _length{len} {}

		constexpr BasicStringView (const String_t &str) : _data{str.data()}, _length{str.length()} {}

		ND_ constexpr explicit operator String_t ()			const	{ return String_t{ _data, _length }; }

		ND_ constexpr T const&	operator [] (size_t index)	const	{ ASSERT(index < _length);  return _data[index]; }

		ND_ constexpr T const*	data ()						const	{ return _data; }
		ND_ constexpr size_t	size ()						const	{ return _length; }
		ND_ constexpr size_t	length ()					const	{ return _length; }
		ND_ constexpr bool		empty ()					const	{ return _length == 0; }
		
		ND_ constexpr T const*	begin ()					const	{ return _data; }
		ND_ constexpr T const*	end ()						const	{ return _data + _length; }


		ND_ constexpr bool  operator == (const BasicStringView<T> &rhs) const
		{
			if ( length() != rhs.length() )
				return false;

			if ( length() == 0 or rhs.data() == data() )
				return true;
		
			for (size_t i = 0; i < length(); ++i) {
				if ( _data[i] != rhs._data[i] )
					return false;
			}
			return true;
		}

		ND_ constexpr bool  operator != (const BasicStringView<T> &rhs) const
		{
			return not (*this == rhs);
		}

		ND_ constexpr bool  operator > (const BasicStringView<T> &rhs) const
		{
			for (size_t i = 0, size = std::min(length(), rhs.length()); i < size; ++i)
			{
				if ( _data[i] != rhs._data[i] )
					return uint(_data[i]) > uint(rhs._data[i]);
			}
			return length() > rhs.length();
		}

		ND_ constexpr bool  operator < (const BasicStringView<T> &rhs) const
		{
			return rhs > *this;
		}


		static constexpr auto npos {static_cast<size_type>(-1)};

		ND_ constexpr size_t  find (const BasicStringView<T> &other, size_t start = 0) const
		{
			if ( empty() or other.empty() )
				return npos;

			for (size_t i = start, j = 0; i < length(); ++i)
			{
				while ( other[j] == _data[i+j] and i+j < length() and j < other.length() )
				{
					++j;
					if ( j >= other.length() ) {
						return i;
					}
				}
				j = 0;
			}
			return npos;
		}


		ND_ constexpr size_t  find_last_of (const BasicStringView<T> &other) const
		{
			size_t	result = npos;

			for (size_t pos = 0;;)
			{
				pos = find( other, pos );

				if ( pos != npos ) {
					pos += other.length();
					result = pos-1;
				}else
					break;
			}
			return result;
		}
		

		ND_ constexpr BasicStringView<T>  substr (const size_t pos, size_t count = UMax) const
		{
			if ( pos >= length() )
				return {};

			if ( (count >= length()) or (count + pos >= length()) )
				count = length() - pos;

			return { &_data[pos], count };
		}

		ND_ HashVal  _CalcHash () const
		{
			return empty() ? HashVal{0} : HashOf( _data, _length * sizeof(T) );
		}
	};

}	// FG

namespace std
{
	template <typename T>
	struct hash< FG::BasicStringView<T> > {
		ND_ size_t  operator () (const FG::BasicStringView<T> &value) const noexcept {
			return size_t(value._CalcHash());
		}
	};
}

#endif	// FG_STD_STRINGVIEW

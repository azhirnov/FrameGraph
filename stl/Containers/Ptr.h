// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace FG
{

	//
	// Raw Pointer Wrapper
	//

	template <typename T>
	struct Ptr
	{
	// variables
	private:
		T *		_value	= null;

	// methods
	public:
		Ptr () {}
		Ptr (T *ptr) : _value{ptr} {}
		Ptr (const Ptr<T> &other) : _value{other._value} {}

		ND_ T *		operator -> ()					const	{ ASSERT( _value );  return _value; }
		ND_ T &		operator *  ()					const	{ ASSERT( _value );  return *_value; }

		ND_ explicit operator T * ()				const	{ return _value; }

		ND_ operator Ptr<const T> ()				const	{ return _value; }

		template <typename B>
		ND_ explicit operator B  ()					const	{ return static_cast<B>( _value ); }

		ND_ explicit operator bool ()				const	{ return _value != null; }

		ND_ bool  operator == (const Ptr<T> &rhs)	const	{ return _value == rhs._value; }
		ND_ bool  operator != (const Ptr<T> &rhs)	const	{ return not (*this == rhs); }
	};

}	// FG


namespace std
{

	template <typename T>
	struct hash< FG::Ptr<T> > {
		ND_ size_t  operator () (const FG::Ptr<T> &value) const noexcept {
			return hash<T *>()( value.operator->() );
		}
	};

}	// std
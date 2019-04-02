// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Utils/Math/GLM.h"

namespace FGC
{

	//
	// Radians
	//

	template <typename T>
	struct RadiansTempl
	{
	// types
	public:
		using Self		= RadiansTempl<T>;
		using Value_t	= T;


	// variables
	private:
		T		_value;


	// methods
	public:
		constexpr RadiansTempl () : _value{} {}
		constexpr explicit RadiansTempl (T val) : _value{val} {}

		ND_ constexpr explicit operator T ()		const	{ return _value; }

		ND_ constexpr static Self  Pi ()					{ return Self{T( 3.14159265358979323846 )}; }

		ND_ Self   operator - ()					const	{ return Self{ -_value }; }

			Self&  operator += (const Self rhs)				{ _value += rhs._value;  return *this; }
			Self&  operator -= (const Self rhs)				{ _value -= rhs._value;  return *this; }
			Self&  operator *= (const Self rhs)				{ _value *= rhs._value;  return *this; }
			Self&  operator /= (const Self rhs)				{ _value /= rhs._value;  return *this; }

			Self&  operator += (const T rhs)				{ _value += rhs;  return *this; }
			Self&  operator -= (const T rhs)				{ _value -= rhs;  return *this; }
			Self&  operator *= (const T rhs)				{ _value *= rhs;  return *this; }
			Self&  operator /= (const T rhs)				{ _value /= rhs;  return *this; }

		ND_ Self   operator + (const T rhs)			const	{ return Self{ _value + rhs }; }
		ND_ Self   operator - (const T rhs)			const	{ return Self{ _value - rhs }; }
		ND_ Self   operator * (const T rhs)			const	{ return Self{ _value * rhs }; }
		ND_ Self   operator / (const T rhs)			const	{ return Self{ _value / rhs }; }
		
		ND_ friend Self  operator + (T lhs, Self rhs)		{ return Self{ lhs + rhs._value }; }
		ND_ friend Self  operator - (T lhs, Self rhs)		{ return Self{ lhs - rhs._value }; }
		ND_ friend Self  operator * (T lhs, Self rhs)		{ return Self{ lhs * rhs._value }; }
		ND_ friend Self  operator / (T lhs, Self rhs)		{ return Self{ lhs / rhs._value }; }
	};


	using Rad = RadiansTempl<float>;
	
	inline static constexpr Rad  Pi = Rad::Pi();
	
	ND_ constexpr Rad  operator "" _rad (long double value)			{ return Rad{ Rad::Value_t(value) }; }
	ND_ constexpr Rad  operator "" _rad (unsigned long long value)	{ return Rad{ Rad::Value_t(value) }; }
	
	ND_ constexpr Rad  operator "" _deg (long double value)			{ return Rad{ glm::radians(Rad::Value_t(value)) }; }
	ND_ constexpr Rad  operator "" _deg (unsigned long long value)	{ return Rad{ glm::radians(Rad::Value_t(value)) }; }

}	// FGC

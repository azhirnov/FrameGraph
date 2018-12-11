// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Utils/Math/GLM.h"

namespace FG
{

	//
	// Radians
	//

	template <typename T>
	struct Radians
	{
	// types
	public:
		using Self		= Radians<T>;
		using Value_t	= T;


	// variables
	private:
		T		_value;


	// methods
	public:
		constexpr Radians () : _value{} {}
		constexpr explicit Radians (T val) : _value{val} {}

		ND_ constexpr explicit operator T ()		const	{ return _value; }

		ND_ constexpr static Self  Pi ()					{ return Self{T( 3.14159265358979323846 )}; }

		ND_ Self   operator - ()					const	{ return Self{ -_value }; }

			Self&  operator += (const Radians<T> rhs)		{ _value += rhs._value;  return *this; }
			Self&  operator -= (const Radians<T> rhs)		{ _value -= rhs._value;  return *this; }
			Self&  operator *= (const Radians<T> rhs)		{ _value *= rhs._value;  return *this; }
			Self&  operator /= (const Radians<T> rhs)		{ _value /= rhs._value;  return *this; }

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


	using Rad = Radians<float>;
	
	inline static constexpr Rad  Pi = Rad::Pi();
	
	ND_ constexpr Rad  operator "" _rad (long double value)			{ return Rad{ Rad::Value_t(value) }; }
	ND_ constexpr Rad  operator "" _rad (unsigned long long value)	{ return Rad{ Rad::Value_t(value) }; }
	
	ND_ constexpr Rad  operator "" _deg (long double value)			{ return Rad{ glm::radians(Rad::Value_t(value)) }; }
	ND_ constexpr Rad  operator "" _deg (unsigned long long value)	{ return Rad{ glm::radians(Rad::Value_t(value)) }; }

}	// FG

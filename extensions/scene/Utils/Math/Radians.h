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
			
		ND_ Self   operator + (const Self rhs)		const	{ return Self{ _value + rhs._value }; }
		ND_ Self   operator - (const Self rhs)		const	{ return Self{ _value - rhs._value }; }
		ND_ Self   operator * (const Self rhs)		const	{ return Self{ _value * rhs._value }; }
		ND_ Self   operator / (const Self rhs)		const	{ return Self{ _value / rhs._value }; }

		ND_ Self   operator + (const T rhs)			const	{ return Self{ _value + rhs }; }
		ND_ Self   operator - (const T rhs)			const	{ return Self{ _value - rhs }; }
		ND_ Self   operator * (const T rhs)			const	{ return Self{ _value * rhs }; }
		ND_ Self   operator / (const T rhs)			const	{ return Self{ _value / rhs }; }

		ND_ bool	operator == (const Self rhs)	const	{ return _value == rhs._value; }
		ND_ bool	operator != (const Self rhs)	const	{ return _value != rhs._value; }
		ND_ bool	operator >  (const Self rhs)	const	{ return _value >  rhs._value; }
		ND_ bool	operator <  (const Self rhs)	const	{ return _value <  rhs._value; }
		ND_ bool	operator >= (const Self rhs)	const	{ return _value >= rhs._value; }
		ND_ bool	operator <= (const Self rhs)	const	{ return _value <= rhs._value; }
		
		ND_ friend Self  operator + (T lhs, Self rhs)		{ return Self{ lhs + rhs._value }; }
		ND_ friend Self  operator - (T lhs, Self rhs)		{ return Self{ lhs - rhs._value }; }
		ND_ friend Self  operator * (T lhs, Self rhs)		{ return Self{ lhs * rhs._value }; }
		ND_ friend Self  operator / (T lhs, Self rhs)		{ return Self{ lhs / rhs._value }; }
	};


	using Rad		= RadiansTempl<float>;
	using RadiansF	= RadiansTempl<float>;
	using RadiansD	= RadiansTempl<double>;

	inline static constexpr Rad  Pi = Rad::Pi();
	
	ND_ constexpr Rad  operator "" _rad (long double value)			{ return Rad{ Rad::Value_t(value) }; }
	ND_ constexpr Rad  operator "" _rad (unsigned long long value)	{ return Rad{ Rad::Value_t(value) }; }
	
	ND_ constexpr Rad  operator "" _deg (long double value)			{ return Rad{ glm::radians(Rad::Value_t(value)) }; }
	ND_ constexpr Rad  operator "" _deg (unsigned long long value)	{ return Rad{ glm::radians(Rad::Value_t(value)) }; }

	
/*
=================================================
	Sin
=================================================
*/
	template <typename T>
	ND_ forceinline T  Sin (const RadiansTempl<T>& x)
	{
		return std::sin( T(x) );
	}
	
/*
=================================================
	Cos
=================================================
*/
	template <typename T>
	ND_ forceinline T  Cos (const RadiansTempl<T>& x)
	{
		return std::cos( T(x) );
	}
	
/*
=================================================
	ASin
=================================================
*/
	template <typename T>
	ND_ inline RadiansTempl<T>  ASin (const T& x)
	{
		STATIC_ASSERT( IsScalarOrEnum<T> );
		ASSERT( x >= T(-1) and x <= T(1) );

		return RadiansTempl<T>{::asin( x )};
	}
	
/*
=================================================
	ACos
=================================================
*/
	template <typename T>
	ND_ inline RadiansTempl<T>  ACos (const T& x)
	{
		STATIC_ASSERT( IsScalarOrEnum<T> );
		ASSERT( x >= T(-1) and x <= T(1) );

		return RadiansTempl<T>{::acos( x )};
	}
	
/*
=================================================
	SinH
=================================================
*/
	template <typename T>
	ND_ inline T  SinH (const RadiansTempl<T>& x)
	{
		return std::sinh( T(x) );
	}
	
/*
=================================================
	CosH
=================================================
*/
	template <typename T>
	ND_ inline T  CosH (const RadiansTempl<T>& x)
	{
		return std::cosh( T(x) );
	}
	
/*
=================================================
	Tan
=================================================
*/
	template <typename T>
	ND_ inline T  Tan (const RadiansTempl<T>& x)
	{
		return std::tan( T(x) );
	}
	
/*
=================================================
	CoTan
=================================================
*/
	template <typename T>
	ND_ inline T  CoTan (const RadiansTempl<T>& x)
	{
		return SafeDiv( T(1), Tan( x ), T(0) );
	}
	
/*
=================================================
	TanH
=================================================
*/
	template <typename T>
	ND_ inline T  TanH (const RadiansTempl<T>& x)
	{
		return std::tanh( T(x) );
	}
	
/*
=================================================
	CoTanH
=================================================
*/
	template <typename T>
	ND_ inline T  CoTanH (const RadiansTempl<T>& x)
	{
		return SafeDiv( T(1), TanH( x ), T(0) );
	}
	
/*
=================================================
	ATan
=================================================
*/
	template <typename T>
	ND_ inline RadiansTempl<T>  ATan (const T& y_over_x)
	{
		STATIC_ASSERT( IsScalarOrEnum<T> );

		return RadiansTempl<T>{::atan( y_over_x )};
	}
	
/*
=================================================
	ATan
=================================================
*/
	template <typename T>
	ND_ inline RadiansTempl<T>  ATan (const T& y, const T& x)
	{
		STATIC_ASSERT( IsScalarOrEnum<T> );

		return RadiansTempl<T>{::atan2( y, x )};
	}
	
/*
=================================================
	ACoTan
=================================================
*/
	template <typename T>
	ND_ inline RadiansTempl<T>  ACoTan (const T& x)
	{
		STATIC_ASSERT( IsScalarOrEnum<T> );

		return RadiansTempl<T>{SafeDiv( T(1), ATan( x ), T(0) )};
	}
	

}	// FGC

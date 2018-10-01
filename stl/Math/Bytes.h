// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/Cast.h"

namespace FG
{

	//
	// Bytes
	//

	template <typename T>
	struct Bytes
	{
		STATIC_ASSERT( std::is_integral_v<T> and std::is_scalar_v<T>, "must be integer scalar" );

	// variables
	private:
		T	_value;


	// methods
	public:
		constexpr Bytes () : _value(0) {}

		explicit constexpr Bytes (T value) : _value(value) {}

		template <typename B>
		explicit constexpr Bytes (const Bytes<B> &other) : _value(T(other)) {}
		
		ND_ explicit constexpr operator int8_t ()	const	{ return static_cast<int8_t>( _value ); }
		ND_ explicit constexpr operator int16_t ()	const	{ return static_cast<int16_t>( _value ); }
		ND_ explicit constexpr operator int ()		const	{ return static_cast<int>( _value ); }
		ND_ explicit constexpr operator int64_t ()	const	{ return static_cast<int64_t>( _value ); }

		ND_ explicit constexpr operator uint8_t ()	const	{ return static_cast<uint8_t>( _value ); }
		ND_ explicit constexpr operator uint16_t ()	const	{ return static_cast<uint16_t>( _value ); }
		ND_ explicit constexpr operator uint32_t ()	const	{ return static_cast<uint32_t>( _value ); }
		ND_ explicit constexpr operator uint64_t ()	const	{ return static_cast<uint64_t>( _value ); }
		
		ND_ constexpr T		Kb ()	const					{ return _value >> 10; }
		ND_ constexpr T		Mb ()	const					{ return _value >> 20; }
		ND_ constexpr T		Gb ()	const					{ return _value >> 30; }
		
		ND_ static constexpr Bytes<T>	FromBits (T value)	{ return Bytes<T>( value >> 3 ); }
		ND_ static constexpr Bytes<T>	FromKb (T value)	{ return Bytes<T>( value << 10 ); }
		ND_ static constexpr Bytes<T>	FromMb (T value)	{ return Bytes<T>( value << 20 ); }
		ND_ static constexpr Bytes<T>	FromGb (T value)	{ return Bytes<T>( value << 30 ); }
		

		template <typename B>	ND_ static constexpr Bytes<T>	SizeOf ()			{ return Bytes<T>( sizeof(B) ); }
		template <typename B>	ND_ static constexpr Bytes<T>	SizeOf (const B &)	{ return Bytes<T>( sizeof(B) ); }
		
		template <typename B>	ND_ static constexpr Bytes<T>	AlignOf ()			{ return Bytes<T>( alignof(B) ); }
		template <typename B>	ND_ static constexpr Bytes<T>	AlignOf (const B &)	{ return Bytes<T>( alignof(B) ); }


		// move any pointer
		template <typename B>
		ND_ friend B *		operator + (B *lhs, const Bytes<T> &rhs)
		{
			return BitCast<B *>( size_t(lhs) + size_t(rhs._value) );
		}
		
		template <typename B>
		ND_ friend B *		operator - (B *lhs, const Bytes<T> &rhs)
		{
			return BitCast<B *>( size_t(lhs) - size_t(rhs._value) );
		}

		ND_ constexpr Bytes<T>	operator ~ () const noexcept						{ return Bytes<T>( ~_value ); }

			Bytes<T>&			operator += (const Bytes<T> &rhs) noexcept			{ _value += rhs._value;  return *this; }
		ND_ constexpr Bytes<T>  operator +  (const Bytes<T> &rhs) const noexcept	{ return Bytes<T>( _value + rhs._value ); }
		
			Bytes<T>&			operator -= (const Bytes<T> &rhs) noexcept			{ _value -= rhs._value;  return *this; }
		ND_ constexpr Bytes<T>  operator -  (const Bytes<T> &rhs) const noexcept	{ return Bytes<T>( _value - rhs._value ); }

			Bytes<T>&			operator *= (const Bytes<T> &rhs) noexcept			{ _value *= rhs._value;  return *this; }
		ND_ constexpr Bytes<T>  operator *  (const Bytes<T> &rhs) const noexcept	{ return Bytes<T>( _value * rhs._value ); }
		
			Bytes<T>&			operator /= (const Bytes<T> &rhs) noexcept			{ _value /= rhs._value;  return *this; }
		ND_ constexpr Bytes<T>  operator /  (const Bytes<T> &rhs) const noexcept	{ return Bytes<T>( _value / rhs._value ); }
		
			Bytes<T>&			operator %= (const Bytes<T> &rhs) noexcept			{ _value %= rhs._value;  return *this; }
		ND_ constexpr Bytes<T>  operator %  (const Bytes<T> &rhs) const noexcept	{ return Bytes<T>( _value % rhs._value ); }
		

			Bytes<T>&			operator += (const T rhs) noexcept					{ _value += rhs;  return *this; }
		ND_ constexpr Bytes<T>  operator +  (const T rhs) const noexcept			{ return Bytes<T>( _value + rhs ); }
		ND_ friend Bytes<T>		operator +  (T lhs, const Bytes<T> &rhs) noexcept	{ return Bytes<T>( lhs + rhs._value ); }
		
			Bytes<T>&			operator -= (const T rhs) noexcept					{ _value -= rhs;  return *this; }
		ND_ constexpr Bytes<T>  operator -  (const T rhs) const noexcept			{ return Bytes<T>( _value - rhs ); }
		ND_ friend Bytes<T>		operator -  (T lhs, const Bytes<T> &rhs) noexcept	{ return Bytes<T>( lhs - rhs._value ); }

			Bytes<T>&			operator *= (const T rhs) noexcept					{ _value *= rhs;  return *this; }
		ND_ constexpr Bytes<T>  operator *  (const T rhs) const noexcept			{ return Bytes<T>( _value * rhs ); }
		ND_ friend Bytes<T>		operator *  (T lhs, const Bytes<T> &rhs) noexcept	{ return Bytes<T>( lhs * rhs._value ); }
		
			Bytes<T>&			operator /= (const T rhs) noexcept					{ _value /= rhs;  return *this; }
		ND_ constexpr Bytes<T>  operator /  (const T rhs) const noexcept			{ return Bytes<T>( _value / rhs ); }
		ND_ friend Bytes<T>		operator /  (T lhs, const Bytes<T> &rhs) noexcept	{ return Bytes<T>( lhs / rhs._value ); }
		
			Bytes<T>&			operator %= (const T rhs) noexcept					{ _value %= rhs;  return *this; }
		ND_ constexpr Bytes<T>  operator %  (const T rhs) const noexcept			{ return Bytes<T>( _value % rhs ); }
		ND_ friend Bytes<T>		operator %  (T lhs, const Bytes<T> &rhs) noexcept	{ return Bytes<T>( lhs % rhs._value ); }


		ND_ constexpr bool		operator == (const Bytes<T> &rhs) const noexcept	{ return _value == rhs._value; }
		ND_ constexpr bool		operator != (const Bytes<T> &rhs) const noexcept	{ return _value != rhs._value; }
		ND_ constexpr bool		operator >  (const Bytes<T> &rhs) const noexcept	{ return _value >  rhs._value; }
		ND_ constexpr bool		operator <  (const Bytes<T> &rhs) const noexcept	{ return _value <  rhs._value; }
		ND_ constexpr bool		operator >= (const Bytes<T> &rhs) const noexcept	{ return _value >= rhs._value; }
		ND_ constexpr bool		operator <= (const Bytes<T> &rhs) const noexcept	{ return _value <= rhs._value; }
		
		ND_ constexpr bool		operator == (const T rhs) const noexcept			{ return _value == rhs; }
		ND_ constexpr bool		operator != (const T rhs) const noexcept			{ return _value != rhs; }
		ND_ constexpr bool		operator >  (const T rhs) const noexcept			{ return _value >  rhs; }
		ND_ constexpr bool		operator <  (const T rhs) const noexcept			{ return _value <  rhs; }
		ND_ constexpr bool		operator >= (const T rhs) const noexcept			{ return _value >= rhs; }
		ND_ constexpr bool		operator <= (const T rhs) const noexcept			{ return _value <= rhs; }

		ND_ friend bool			operator == (T lhs, Bytes<T> rhs) noexcept			{ return lhs == rhs._value; }
		ND_ friend bool			operator != (T lhs, Bytes<T> rhs) noexcept			{ return lhs != rhs._value; }
		ND_ friend bool			operator >  (T lhs, Bytes<T> rhs) noexcept			{ return lhs >  rhs._value; }
		ND_ friend bool			operator <  (T lhs, Bytes<T> rhs) noexcept			{ return lhs <  rhs._value; }
		ND_ friend bool			operator >= (T lhs, Bytes<T> rhs) noexcept			{ return lhs >= rhs._value; }
		ND_ friend bool			operator <= (T lhs, Bytes<T> rhs) noexcept			{ return lhs <= rhs._value; }
	};
	

	using BytesU = Bytes< uint64_t >;
	
	template <typename T>
	inline static constexpr BytesU	SizeOf = BytesU::SizeOf<T>();
	
	template <typename T>
	inline static constexpr BytesU	AlignOf = BytesU::AlignOf<T>();
	

	ND_ constexpr BytesU  operator "" _b (unsigned long long value)		{ return BytesU( value ); }
	ND_ constexpr BytesU  operator "" _Kb (unsigned long long value)	{ return BytesU::FromKb( value ); }
	ND_ constexpr BytesU  operator "" _Mb (unsigned long long value)	{ return BytesU::FromMb( value ); }
	ND_ constexpr BytesU  operator "" _Gb (unsigned long long value)	{ return BytesU::FromGb( value ); }

	
/*
=================================================
	OffsetOf
=================================================
*/
	template <typename A, typename B>
    ND_ constexpr forceinline BytesU  OffsetOf (A B::*member)
	{
		const union U {
			B		b;
			int		tmp;
			U () : tmp(0) {}
			~U () {}
		} u;
		return BytesU( size_t(std::addressof(u.b.*member)) - size_t(std::addressof(u.b)) );
	}

}	// FG


namespace std
{
	template <typename T>
	struct hash< FG::Bytes<T> >
	{
		ND_ size_t  operator () (const FG::Bytes<T> &value) const noexcept
		{
			return size_t(FG::HashOf( T(value) ));
		}
	};

}	// std

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"

namespace FG
{

	template <typename T, uint I>
	struct Vec;
	

	using bool2		= Vec< bool, 2 >;
	using bool3		= Vec< bool, 3 >;
	using bool4		= Vec< bool, 4 >;
	
	using short2	= Vec< int16_t, 2 >;
	using short3	= Vec< int16_t, 3 >;
	using short4	= Vec< int16_t, 4 >;

	using uint2		= Vec< uint, 2 >;
	using uint3		= Vec< uint, 3 >;
	using uint4		= Vec< uint, 4 >;

	using int2		= Vec< int, 2 >;
	using int3		= Vec< int, 3 >;
	using int4		= Vec< int, 4 >;

	using float2	= Vec< float, 2 >;
	using float3	= Vec< float, 3 >;
	using float4	= Vec< float, 4 >;



	//
	// Vec2
	//

	template <typename T>
	struct Vec< T, 2 >
	{
	// types
		using Self			= Vec<T,2>;
		using value_type	= T;

	// variables
		T	x, y;

	// methods
		constexpr Vec () : x{}, y{}
		{
			// check is supported cast Vec to array
			STATIC_ASSERT( offsetof(Self, x) + sizeof(T) == offsetof(Self, y) );
			STATIC_ASSERT( sizeof(T[size()-1]) == (offsetof(Self, y) - offsetof(Self, x)) );
		}

		constexpr Vec (T x, T y) : x{x}, y{y} {}
		
		explicit constexpr Vec (T val) : x{val}, y{val} {}

		template <typename B>
		constexpr Vec (const Vec<B,2> &other) : x{T(other.x)}, y{T(other.y)} {}

		ND_ static constexpr size_t		size ()					{ return 2; }

		ND_ constexpr T *		data ()							{ return std::addressof(x); }
		ND_ constexpr T const *	data ()					const	{ return std::addressof(x); }

		ND_ constexpr T &		operator [] (size_t i)			{ ASSERT( i < size() );  return std::addressof(x)[i]; }
		ND_ constexpr T const&	operator [] (size_t i)	const	{ ASSERT( i < size() );  return std::addressof(x)[i]; }
	};



	//
	// Vec3
	//

	template <typename T>
	struct Vec< T, 3 >
	{
	// types
		using Self			= Vec<T,3>;
		using value_type	= T;

	// variables
		T	x, y, z;

	// methods
		constexpr Vec () : x{}, y{}, z{}
		{
			// check is supported cast Vec to array
			STATIC_ASSERT( offsetof(Self, x) + sizeof(T) == offsetof(Self, y) );
			STATIC_ASSERT( offsetof(Self, y) + sizeof(T) == offsetof(Self, z) );
			STATIC_ASSERT( sizeof(T[size()-1]) == (offsetof(Self, z) - offsetof(Self, x)) );
		}

		explicit constexpr Vec (T val) : x{val}, y{val}, z{val} {}

		constexpr Vec (T x, T y, T z) : x{x}, y{y}, z{z} {}
		constexpr Vec (const Vec<T,2> &xy, T z) : x{xy[0]}, y{xy[1]}, z{z} {}
		
		template <typename B>
		constexpr Vec (const Vec<B,3> &other) : x{T(other.x)}, y{T(other.y)}, z{T(other.z)} {}

		ND_ static constexpr size_t		size ()					{ return 3; }
		
		ND_ constexpr T *		data ()							{ return std::addressof(x); }
		ND_ constexpr T const *	data ()					const	{ return std::addressof(x); }

		ND_ constexpr T &		operator [] (size_t i)			{ ASSERT( i < size() );  return std::addressof(x)[i]; }
		ND_ constexpr T const&	operator [] (size_t i)	const	{ ASSERT( i < size() );  return std::addressof(x)[i]; }

		ND_ const Vec<T,2>		xy ()					const	{ return {x, y}; }
	};



	//
	// Vec4
	//

	template <typename T>
	struct Vec< T, 4 >
	{
	// types
		using Self			= Vec<T,4>;
		using value_type	= T;

	// variables
		T	x, y, z, w;

	// methods
		constexpr Vec () : x{}, y{}, z{}, w{}
		{
			// check is supported cast Vec to array
			STATIC_ASSERT( offsetof(Self, x) + sizeof(T) == offsetof(Self, y) );
			STATIC_ASSERT( offsetof(Self, y) + sizeof(T) == offsetof(Self, z) );
			STATIC_ASSERT( offsetof(Self, z) + sizeof(T) == offsetof(Self, w) );
			STATIC_ASSERT( sizeof(T[size()-1]) == (offsetof(Self, w) - offsetof(Self, x)) );
		}

		explicit constexpr Vec (T val) : x{val}, y{val}, z{val}, w{val} {}

		constexpr Vec (T x, T y, T z, T w) : x{x}, y{y}, z{z}, w{w} {}
		constexpr Vec (const Vec<T,3> &xyz, T w) : x{xyz[0]}, y{xyz[1]}, z{xyz[2]}, w{w} {}
		
		template <typename B>
		constexpr Vec (const Vec<B,4> &other) : x{T(other.x)}, y{T(other.y)}, z{T(other.z)}, w{T(other.w)} {}

		ND_ static constexpr size_t		size ()					{ return 4; }
		
		ND_ constexpr T *		data ()							{ return std::addressof(x); }
		ND_ constexpr T const *	data ()					const	{ return std::addressof(x); }

		ND_ constexpr T &		operator [] (size_t i)			{ ASSERT( i < size() );  return std::addressof(x)[i]; }
		ND_ constexpr T const&	operator [] (size_t i)	const	{ ASSERT( i < size() );  return std::addressof(x)[i]; }
		
		ND_ const Vec<T,2>		xy ()					const	{ return {x, y}; }
		ND_ const Vec<T,3>		xyz ()					const	{ return {x, y, z}; }
	};

	

/*
=================================================
	operator ==
=================================================
*/
	template <typename T>
	ND_ inline constexpr bool2  operator == (const Vec<T,2> &lhs, const Vec<T,2> &rhs)
	{
		return { lhs.x == rhs.x, lhs.y == rhs.y };
	}
		
	template <typename T>
	ND_ inline constexpr bool3  operator == (const Vec<T,3> &lhs, const Vec<T,3> &rhs)
	{
		return { lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z };
	}
		
	template <typename T>
	ND_ inline constexpr bool4  operator == (const Vec<T,4> &lhs, const Vec<T,4> &rhs)
	{
		return { lhs.x == rhs.x, lhs.y == rhs.y, lhs.z == rhs.z, lhs.w == rhs.w };
	}
	
/*
=================================================
	operator !
=================================================
*/
	ND_ inline constexpr bool2  operator ! (const bool2 &value)		{ return { !value.x, !value.y }; }
	ND_ inline constexpr bool3  operator ! (const bool3 &value)		{ return { !value.x, !value.y, !value.z }; }
	ND_ inline constexpr bool4  operator ! (const bool4 &value)		{ return { !value.x, !value.y, !value.z, !value.w }; }

/*
=================================================
	operator !=
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<bool,I>  operator != (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return not (lhs == rhs);
	}

/*
=================================================
	operator >
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<bool,I>  operator >  (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		Vec<bool,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = lhs[i] > rhs[i];
		}
		return res;
	}
	
	template <typename T, uint I, typename S>
	ND_ inline constexpr EnableIf<IsScalar<S>, Vec<bool,I>>  operator >  (const Vec<T,I> &lhs, const S &rhs)
	{
		return lhs > Vec<T,I>( rhs );
	}
	
	template <typename T, uint I, typename S>
	ND_ inline constexpr EnableIf<IsScalar<S>, Vec<bool,I>>  operator >  (const S &lhs, const Vec<T,I> &rhs)
	{
		return Vec<T,I>( lhs ) > rhs;
	}

/*
=================================================
	operator <
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<bool,I>  operator < (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return rhs > lhs;
	}
	
	template <typename T, uint I, typename S>
	ND_ inline constexpr EnableIf<IsScalar<S>, Vec<bool,I>>  operator <  (const Vec<T,I> &lhs, const S &rhs)
	{
		return rhs > lhs;
	}
	
	template <typename T, uint I, typename S>
	ND_ inline constexpr EnableIf<IsScalar<S>, Vec<bool,I>>  operator <  (const S &lhs, const Vec<T,I> &rhs)
	{
		return rhs > lhs;
	}
	
/*
=================================================
	operator >=
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<bool,I>  operator >= (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return not (lhs < rhs);
	}
	
/*
=================================================
	operator <=
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<bool,I>  operator <= (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		return not (lhs > rhs);
	}

/*
=================================================
	operator +
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<T,I>  operator + (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = lhs[i] + rhs[i];
		}
		return res;
	}
	
	template <typename T, uint I, typename S>
	ND_ inline constexpr EnableIf<IsScalar<S>, Vec<T,I>>  operator + (const Vec<T,I> &lhs, const S &rhs)
	{
		return lhs + Vec<T,I>( rhs );
	}

	template <typename T, uint I, typename S>
	ND_ inline constexpr EnableIf<IsScalar<S>, Vec<T,I>>  operator + (const S &lhs, const Vec<T,I> &rhs)
	{
		return Vec<T,I>( lhs ) + rhs;
	}

/*
=================================================
	operator -
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<T,I>  operator - (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = lhs[i] - rhs[i];
		}
		return res;
	}

	template <typename T, uint I, typename S>
	ND_ inline constexpr EnableIf<IsScalar<S>, Vec<T,I>>  operator - (const Vec<T,I> &lhs, const S &rhs)
	{
		return lhs - Vec<T,I>( rhs );
	}

	template <typename T, uint I, typename S>
	ND_ inline constexpr EnableIf<IsScalar<S>, Vec<T,I>>  operator - (const S &lhs, const Vec<T,I> &rhs)
	{
		return Vec<T,I>( lhs ) - rhs;
	}

/*
=================================================
	operator *
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<T,I>  operator * (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = lhs[i] * rhs[i];
		}
		return res;
	}

	template <typename T, uint I, typename S>
	ND_ inline constexpr EnableIf<IsScalar<S>, Vec<T,I>>  operator * (const Vec<T,I> &lhs, const S &rhs)
	{
		return lhs * Vec<T,I>( rhs );
	}

	template <typename T, uint I, typename S>
	ND_ inline constexpr EnableIf<IsScalar<S>, Vec<T,I>>  operator * (const S &lhs, const Vec<T,I> &rhs)
	{
		return Vec<T,I>( lhs ) * rhs;
	}

/*
=================================================
	operator /
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<T,I>  operator / (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = lhs[i] / rhs[i];
		}
		return res;
	}

	template <typename T, uint I, typename S>
	ND_ inline constexpr Vec<T,I>  operator / (const Vec<T,I> &lhs, const S &rhs)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = lhs[i] / rhs;
		}
		return res;
	}

/*
=================================================
	operator <<
=================================================
*/
	template <typename T, uint I, typename S>
	ND_ inline constexpr Vec<T,I>  operator << (const Vec<T,I> &lhs, const S &rhs)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = lhs[i] << rhs;
		}
		return res;
	}

/*
=================================================
	operator >>
=================================================
*/
	template <typename T, uint I, typename S>
	ND_ inline constexpr Vec<T,I>  operator >> (const Vec<T,I> &lhs, const S &rhs)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = lhs[i] >> rhs;
		}
		return res;
	}
	
/*
=================================================
	All
=================================================
*/
	ND_ inline constexpr bool  All (const bool2 &value)
	{
		return value.x and value.y;
	}
	
	ND_ inline constexpr bool  All (const bool3 &value)
	{
		return value.x and value.y and value.z;
	}
	
	ND_ inline constexpr bool  All (const bool4 &value)
	{
		return value.x and value.y and value.z and value.w;
	}
	
/*
=================================================
	Any
=================================================
*/
	ND_ inline constexpr bool  Any (const bool2 &value)
	{
		return value.x or value.y;
	}
	
	ND_ inline constexpr bool  Any (const bool3 &value)
	{
		return value.x or value.y or value.z;
	}
	
	ND_ inline constexpr bool  Any (const bool4 &value)
	{
		return value.x or value.y or value.z or value.w;
	}

/*
=================================================
	Max
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<T,I>  Max (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = Max( lhs[i], rhs[i] );
		}
		return res;
	}
	
	template <typename T, uint I>
	ND_ inline constexpr Vec<T,I>  Max (const Vec<T,I> &lhs, const T &rhs)
	{
		return Max( lhs, Vec<T,I>(rhs) );
	}

	template <typename T, uint I>
	ND_ inline constexpr Vec<T,I>  Max (const T &lhs, const Vec<T,I> &rhs)
	{
		return Max( Vec<T,I>(lhs), rhs );
	}

/*
=================================================
	Min
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<T,I>  Min (const Vec<T,I> &lhs, const Vec<T,I> &rhs)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = Min( lhs[i], rhs[i] );
		}
		return res;
	}
	
	template <typename T, uint I>
	ND_ inline constexpr Vec<T,I>  Min (const Vec<T,I> &lhs, const T &rhs)
	{
		return Min( lhs, Vec<T,I>(rhs) );
	}

	template <typename T, uint I>
	ND_ inline constexpr Vec<T,I>  Min (const T &lhs, const Vec<T,I> &rhs)
	{
		return Min( Vec<T,I>(lhs), rhs );
	}
	
/*
=================================================
	Clamp
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<T,I>  Clamp (const Vec<T,I> &value, const T &minVal, const T &maxVal)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) { 
			res[i] = Clamp( value[i], minVal, maxVal );
		}
		return res;
	}

/*
=================================================
	Equals
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<bool,I>  Equals (const Vec<T,I> &lhs, const Vec<T,I> &rhs, const T &err = std::numeric_limits<T>::epsilon())
	{
		Vec<bool,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = Equals( lhs[i], rhs[i], err );
		}
		return res;
	}
	
/*
=================================================
	Floor
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr EnableIf<IsFloatPoint<T>, Vec<T,I>>  Floor (const Vec<T,I>& x)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = Floor( x[i] );
		}
		return res;
	}
	
/*
=================================================
	Abs
=================================================
*/
	template <typename T, uint I>
	ND_ inline constexpr Vec<T,I>  Abs (const Vec<T,I> &x)
	{
		Vec<T,I>	res;
		for (uint i = 0; i < I; ++i) {
			res[i] = Abs( x[i] );
		}
		return res;
	}

}	// FG


namespace std
{
#if FG_FAST_HASH
	template <typename T, uint I>
	struct hash< FG::Vec<T,I> > {
		ND_ size_t  operator () (const FG::Vec<T,I> &value) const noexcept {
			return size_t(FG::HashOf( value.data(), value.size() * sizeof(T) ));
		}
	};

#else
	template <typename T>
	struct hash< FG::Vec<T,2> > {
		ND_ size_t  operator () (const FG::Vec<T,2> &value) const noexcept {
			return size_t(FG::HashOf( value.x ) + FG::HashOf( value.y ));
		}
	};
	
	template <typename T>
	struct hash< FG::Vec<T,3> > {
		ND_ size_t  operator () (const FG::Vec<T,3> &value) const noexcept {
			return size_t(FG::HashOf( value.x ) + FG::HashOf( value.y ) + FG::HashOf( value.z ));
		}
	};
	
	template <typename T>
	struct hash< FG::Vec<T,4> > {
		ND_ size_t  operator () (const FG::Vec<T,4> &value) const noexcept {
			return size_t(FG::HashOf( value.x ) + FG::HashOf( value.y ) + FG::HashOf( value.z ) + FG::HashOf( value.w ));
		}
	};
#endif

}	// std

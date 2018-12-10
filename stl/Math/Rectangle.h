// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"

namespace FG
{

	template <typename T>
	struct Rectangle
	{
	// types
		using Vec2_t	= Vec<T,2>;
		using Self		= Rectangle<T>;


	// variables
		T	left, top;
		T	right, bottom;


	// methods
		constexpr Rectangle () :
			left{T(0)}, top{T(0)}, right{T(0)}, bottom{T(0)} {}
		
		constexpr Rectangle (T left, T top, T right, T bottom) :
			left{left}, top{top}, right{right}, bottom{bottom} {}

		constexpr Rectangle (const Vec2_t &leftTop, const Vec2_t &rightBottom) :
			left{leftTop.x}, top{leftTop.y}, right{rightBottom.x}, bottom{rightBottom.y} {}

		template <typename B>
		constexpr explicit Rectangle (const Rectangle<B> &other) :
			left{T(other.left)}, top{T(other.top)}, right{T(other.right)}, bottom{T(other.bottom)} {}


		ND_ constexpr const T		Width ()		const	{ return right - left; }
		ND_ constexpr const T		Height ()		const	{ return bottom - top; }

		ND_ constexpr const Vec2_t	LeftTop ()		const	{ return { left, top }; }
		ND_ constexpr const Vec2_t	RightBottom ()	const	{ return { right, bottom }; }

		ND_ constexpr const Vec2_t	Size ()			const	{ return { Width(), Height() }; }
		ND_ constexpr const Vec2_t	Center ()		const	{ return { (right + left) / T(2), (top + bottom) / T(2) }; }

		ND_ constexpr bool			IsEmpty ()		const	{ return Equals( left, right ) or Equals( top, bottom ); }
		ND_ constexpr bool			IsInvalid ()	const	{ return right < left or bottom < top; }
		ND_ constexpr bool			IsValid ()		const	{ return not IsEmpty() and not IsInvalid(); }

		ND_ Self  operator + (const Vec2_t &rhs)	const	{ return Self{*this} += rhs; }
		ND_ Self  operator - (const Vec2_t &rhs)	const	{ return Self{*this} -= rhs; }

		Self&  operator += (const Vec2_t &rhs)
		{
			left += rhs.x;		right  += rhs.x;
			top  += rhs.y;		bottom += rhs.y;
			return *this;
		}
		
		Self&  operator -= (const Vec2_t &rhs)
		{
			left -= rhs.x;		right  -= rhs.x;
			top  -= rhs.y;		bottom -= rhs.y;
			return *this;
		}

		void Merge (const Rectangle<T> &other)
		{
			left	= Min( left,   other.left   );
			top		= Min( top,    other.top    );
			right	= Max( right,  other.right  );
			bottom	= Max( bottom, other.bottom );
		}
	};


	using RectU		= Rectangle< uint >;
	using RectI		= Rectangle< int >;
	using RectF		= Rectangle< float >;


}	// FG


namespace std
{
	template <typename T>
	struct hash< FG::Rectangle<T> >
	{
		ND_ size_t  operator () (const FG::Rectangle<T> &value) const noexcept
		{
		#if FG_FAST_HASH
			return	size_t( FG::HashOf( this, sizeof(*this) ));
		#else
			return	size_t( FG::HashOf( value.left )  + FG::HashOf( value.bottom ) +
							FG::HashOf( value.right ) + FG::HashOf( value.top ) );
		#endif
		}
	};

}	// std

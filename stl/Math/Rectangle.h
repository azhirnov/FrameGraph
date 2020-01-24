// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"

namespace FGC
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
			left{T(0)}, top{T(0)}, right{T(0)}, bottom{T(0)}
		{
			// check is supported cast Rectangle to array
			STATIC_ASSERT( offsetof(Self, left)  + sizeof(T) == offsetof(Self, top) );
			STATIC_ASSERT( offsetof(Self, top)   + sizeof(T) == offsetof(Self, right) );
			STATIC_ASSERT( offsetof(Self, right) + sizeof(T) == offsetof(Self, bottom) );
			STATIC_ASSERT( sizeof(T[3]) == (offsetof(Self, bottom) - offsetof(Self, left)) );
		}
		
		constexpr Rectangle (T left, T top, T right, T bottom) :
			left{left}, top{top}, right{right}, bottom{bottom} {}

		constexpr Rectangle (const Vec2_t &leftTop, const Vec2_t &rightBottom) :
			left{leftTop.x}, top{leftTop.y}, right{rightBottom.x}, bottom{rightBottom.y} {}
		
		constexpr explicit Rectangle (const Vec2_t &size) :
			Rectangle{ Vec2_t{}, size } {}

		constexpr Rectangle (const Self &other) :
			left{other.left}, top{other.top}, right{other.right}, bottom{other.bottom} {}

		template <typename B>
		constexpr explicit Rectangle (const Rectangle<B> &other) :
			left{T(other.left)}, top{T(other.top)}, right{T(other.right)}, bottom{T(other.bottom)} {}

		Self& LeftTop (const Vec2_t& v);
		Self& RightBottom (const Vec2_t& v);

		ND_ constexpr const T		Width ()		const	{ return right - left; }
		ND_ constexpr const T		Height ()		const	{ return bottom - top; }
		ND_ constexpr const T		CenterX ()		const	{ return (right + left) / T(2); }
		ND_ constexpr const T		CenterY ()		const	{ return (top + bottom) / T(2); }

		ND_ constexpr const Vec2_t	LeftTop ()		const	{ return { left, top }; }
		ND_ constexpr const Vec2_t	RightBottom ()	const	{ return { right, bottom }; }
		ND_ constexpr const Vec2_t	LeftBottom ()	const	{ return { left, bottom }; }
		ND_ constexpr const Vec2_t	RightTop ()		const	{ return { right, top }; }

		ND_ T const*				data ()			const	{ return std::addressof( left ); }
		ND_ T *						data ()					{ return std::addressof( left ); }

		ND_ constexpr const Vec2_t	Size ()			const	{ return { Width(), Height() }; }
		ND_ constexpr const Vec2_t	Center ()		const	{ return { CenterX(), CenterY() }; }

		ND_ constexpr bool			IsEmpty ()		const	{ return Equals( left, right ) | Equals( top, bottom ); }
		ND_ constexpr bool			IsInvalid ()	const	{ return (right < left) | (bottom < top); }
		ND_ constexpr bool			IsValid ()		const	{ return (not IsEmpty()) & (not IsInvalid()); }
		
		ND_ constexpr bool  IsNormalized () const;
			Self &			Normalize ();

		ND_ constexpr bool	Intersects (const Vec2_t &point) const;
		ND_ constexpr bool	Intersects (const Self &point) const;
			
		ND_ constexpr Self	Intersection (const Self &other) const;

		ND_ constexpr bool4 operator == (const Self &rhs) const;
		
			Self&	Join (const Self &other);
			Self&	Join (const Vec2_t &point);

			Self&	Stretch (const Self &size);
			Self&	Stretch (const Vec2_t &size);
			Self&	Stretch (T size);
	};


	using RectU		= Rectangle< uint >;
	using RectI		= Rectangle< int >;
	using RectF		= Rectangle< float >;

	
/*
=================================================
	LeftTop / RightBottom
=================================================
*/
	template <typename T>
	inline Rectangle<T>&  Rectangle<T>::LeftTop (const Vec2_t& v)
	{
		left = v.x;
		top = v.y;
		return *this;
	}
	
	template <typename T>
	inline Rectangle<T>&  Rectangle<T>::RightBottom (const Vec2_t& v)
	{
		right = v.x;
		bottom = v.y;
		return *this;
	}

/*
=================================================
	operator +
=================================================
*/
	template <typename T>
	inline constexpr Rectangle<T>&  operator += (Rectangle<T> &lhs, const Vec<T,2> &rhs)
	{
		lhs.left += rhs.x;	lhs.right  += rhs.x;
		lhs.top  += rhs.y;	lhs.bottom += rhs.y;
		return lhs;
	}
	
	template <typename T>
	inline constexpr Rectangle<T>  operator + (const Rectangle<T> &lhs, const Vec<T,2> &rhs)
	{
		return Rectangle<T>{ lhs.left  + rhs.x, lhs.top    + rhs.y,
							 lhs.right + rhs.x, lhs.bottom + rhs.y };
	}

	template <typename T>
	inline constexpr Rectangle<T>&  operator += (Rectangle<T> &lhs, const T &rhs)
	{
		return lhs += Vec<T,2>{rhs};
	}
	
	template <typename T>
	inline constexpr Rectangle<T>  operator + (const Rectangle<T> &lhs, const T &rhs)
	{
		return lhs + Vec<T,2>{rhs};
	}

/*
=================================================
	operator -
=================================================
*/
	template <typename T>
	inline constexpr Rectangle<T>&  operator -= (Rectangle<T> &lhs, const Vec<T,2> &rhs)
	{
		lhs.left -= rhs.x;	lhs.right  -= rhs.x;
		lhs.top  -= rhs.y;	lhs.bottom -= rhs.y;
		return lhs;
	}
	
	template <typename T>
	inline constexpr Rectangle<T>  operator - (const Rectangle<T> &lhs, const Vec<T,2> &rhs)
	{
		return Rectangle<T>{ lhs.left  - rhs.x, lhs.top    - rhs.y,
							 lhs.right - rhs.x, lhs.bottom - rhs.y };
	}

	template <typename T>
	inline constexpr Rectangle<T>&  operator -= (Rectangle<T> &lhs, const T &rhs)
	{
		return lhs -= Vec<T,2>{rhs};
	}
	
	template <typename T>
	inline constexpr Rectangle<T>  operator - (const Rectangle<T> &lhs, const T &rhs)
	{
		return lhs - Vec<T,2>{rhs};
	}

/*
=================================================
	operator *
=================================================
*/
	template <typename T>
	inline constexpr Rectangle<T>&  operator *= (Rectangle<T> &lhs, const Vec<T,2> &rhs)
	{
		lhs.left *= rhs.x;	lhs.right  *= rhs.x;
		lhs.top  *= rhs.y;	lhs.bottom *= rhs.y;
		return lhs;
	}
	
	template <typename T>
	inline constexpr Rectangle<T>  operator * (const Rectangle<T> &lhs, const Vec<T,2> &rhs)
	{
		return Rectangle<T>{ lhs.left  * rhs.x, lhs.top    * rhs.y,
							 lhs.right * rhs.x, lhs.bottom * rhs.y };
	}

	template <typename T>
	inline constexpr Rectangle<T>&  operator *= (Rectangle<T> &lhs, const T &rhs)
	{
		return lhs *= Vec<T,2>{rhs};
	}
	
	template <typename T>
	inline constexpr Rectangle<T>  operator * (const Rectangle<T> &lhs, const T &rhs)
	{
		return lhs * Vec<T,2>{rhs};
	}

/*
=================================================
	operator /
=================================================
*/
	template <typename T>
	inline constexpr Rectangle<T>&  operator /= (Rectangle<T> &lhs, const Vec<T,2> &rhs)
	{
		lhs.left /= rhs.x;	lhs.right  /= rhs.x;
		lhs.top  /= rhs.y;	lhs.bottom /= rhs.y;
		return lhs;
	}
	
	template <typename T>
	inline constexpr Rectangle<T>  operator / (const Rectangle<T> &lhs, const Vec<T,2> &rhs)
	{
		return Rectangle<T>{ lhs.left  / rhs.x, lhs.top    / rhs.y,
							 lhs.right / rhs.x, lhs.bottom / rhs.y };
	}

	template <typename T>
	inline constexpr Rectangle<T>&  operator /= (Rectangle<T> &lhs, const T &rhs)
	{
		return lhs /= Vec<T,2>{rhs};
	}
	
	template <typename T>
	inline constexpr Rectangle<T>  operator / (const Rectangle<T> &lhs, const T &rhs)
	{
		return lhs / Vec<T,2>{rhs};
	}
	
/*
=================================================
	IsNormalized
=================================================
*/
	template <typename T>
	inline constexpr bool  Rectangle<T>::IsNormalized () const
	{
		return (left <= right) & (top <= bottom);
	}
	
/*
=================================================
	Normalize
=================================================
*/
	template <typename T>
	inline Rectangle<T>&  Rectangle<T>::Normalize ()
	{
		if ( left > right )	std::swap( left, right );
		if ( top > bottom )	std::swap( top, bottom );
		return *this;
	}

/*
=================================================
	Intersects
=================================================
*/
	template <typename T>
	inline constexpr bool  Rectangle<T>::Intersects (const Vec2_t &point) const
	{
		return (point.x >= left) & (point.x < right) & (point.y >= top) & (point.y < bottom);
	}
	
	template <typename T>
	inline constexpr bool  Rectangle<T>::Intersects (const Self &other) const
	{
		return	((left < other.right) & (right > other.left) & (bottom > other.top) & (top < other.bottom)) |
				((other.right < left) & (other.left > right) & (other.top > bottom) & (other.bottom < top));
	}

/*
=================================================
	operator ==
=================================================
*/
	template <typename T>
	inline constexpr bool4  Rectangle<T>::operator == (const Self &rhs) const
	{
		return { left == rhs.left, top == rhs.top, right == rhs.right, bottom == rhs.bottom };
	}
	
/*
=================================================
	Intersection
=================================================
*/
	template <typename T>
	inline constexpr Rectangle<T>  Rectangle<T>::Intersection (const Self &other) const
	{
		Rectangle<T>	res;
		res.left	= Max( left,	other.left );
		res.top		= Max( top,		other.top );
		res.right	= Min( right,	other.right );
		res.bottom	= Min( bottom,	other.bottom );
		return res;
	}

/*
=================================================
	Join
=================================================
*/
	template <typename T>
	inline Rectangle<T>&  Rectangle<T>::Join (const Self &other)
	{
		left	= Min( left,	other.left );
		top		= Min( top,		other.top );
		right	= Max( right,	other.right );
		bottom	= Max( bottom,	other.bottom );
		return *this;
	}
	
	template <typename T>
	inline Rectangle<T>&  Rectangle<T>::Join (const Vec2_t &point)
	{
		left	= Min( left,	point.x );
		top		= Min( top,		point.y );
		right	= Max( right,	point.x );
		bottom	= Max( bottom,	point.y );
		return *this;
	}
	
/*
=================================================
	Stretch
=================================================
*/
	template <typename T>
	inline Rectangle<T>&  Rectangle<T>::Stretch (const Self &size)
	{
		left	-= size.left;
		top		-= size.top;
		right	+= size.right;
		bottom	+= size.bottom;
		return *this;
	}

	template <typename T>
	inline Rectangle<T>&  Rectangle<T>::Stretch (const Vec2_t &size)
	{
		const Vec2_t  half_size = size / T(2);

		left	-= half_size.x;
		top		-= half_size.y;
		right	+= half_size.x;
		bottom	+= half_size.y;
		return *this;
	}

	template <typename T>
	inline Rectangle<T>&  Rectangle<T>::Stretch (T size)
	{
		const T  half_size = size / T(2);

		left	-= half_size;
		top		-= half_size;
		right	+= half_size;
		bottom	+= half_size;
		return *this;
	}

/*
=================================================
	Equals
=================================================
*/
	template <typename T>
	ND_ inline constexpr bool4  Equals (const Rectangle<T> &lhs, const Rectangle<T> &rhs, const T &err = std::numeric_limits<T>::epsilon() * T(2))
	{
		bool4	res;
		res[0] = FGC::Equals( lhs.left,   rhs.left,   err );
		res[1] = FGC::Equals( lhs.top,    rhs.top,    err );
		res[2] = FGC::Equals( lhs.right,  rhs.right,  err );
		res[3] = FGC::Equals( lhs.bottom, rhs.bottom, err );
		return res;
	}


}	// FGC


namespace std
{
	template <typename T>
	struct hash< FGC::Rectangle<T> >
	{
		ND_ size_t  operator () (const FGC::Rectangle<T> &value) const
		{
		#if FG_FAST_HASH
			return	size_t( FGC::HashOf( this, sizeof(*this) ));
		#else
			return	size_t( FGC::HashOf( value.left )  + FGC::HashOf( value.bottom ) +
							FGC::HashOf( value.right ) + FGC::HashOf( value.top ) );
		#endif
		}
	};

}	// std

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Vec.h"

namespace FG
{

	//
	// RGBA Color
	//

	template <typename T>
	struct RGBAColor
	{
	// types
		using Self			= RGBAColor<T>;
		using value_type	= T;

	// variables
		T	r, g, b, a;

	// methods
		constexpr RGBAColor () : r{T(0)}, g{T(0)}, b{T(0)}, a{T(0)}
		{
			// check is supported cast Color to array
			STATIC_ASSERT( offsetof(Self, r) + sizeof(T) == offsetof(Self, g) );
			STATIC_ASSERT( offsetof(Self, g) + sizeof(T) == offsetof(Self, b) );
			STATIC_ASSERT( offsetof(Self, b) + sizeof(T) == offsetof(Self, a) );
			STATIC_ASSERT( sizeof(T[size()-1]) == (offsetof(Self, a) - offsetof(Self, r)) );
		}

		constexpr RGBAColor (T r, T g, T b, T a) : r{r}, g{g}, b{b}, a{a}
		{}

		constexpr explicit RGBAColor (T val) : r{val}, g{val}, b{val}, a{val}
		{}

		template <typename B>
		constexpr explicit RGBAColor (const RGBAColor<B> &other);

		explicit RGBAColor (struct HSVColor const& hsv, T alpha = MaxValue());


		ND_ constexpr bool operator == (const RGBAColor<T> &rhs) const
		{
			const T  eps = Epsilon();
			return	Equals( r, rhs.r, eps ) and
					Equals( g, rhs.g, eps ) and
					Equals( b, rhs.b, eps ) and
					Equals( a, rhs.a, eps );
		}
		
		ND_ static constexpr T  MaxValue ()
		{
			if constexpr ( IsFloatPoint<T> )
				return T(1.0);
			else
				return std::numeric_limits<T>::max();
		}

		ND_ static constexpr T  Epsilon ()
		{
			if constexpr ( IsFloatPoint<T> )
				return T(0.001);
			else
				return T(0);
		}

		ND_ static constexpr size_t		size ()			{ return 4; }
		
		ND_ T *			data ()							{ return std::addressof(r); }
		ND_ T const *	data ()					const	{ return std::addressof(r); }

		ND_ T &			operator [] (size_t i)			{ ASSERT( i < size() );  return std::addressof(r)[i]; }
		ND_ T const&	operator [] (size_t i)	const	{ ASSERT( i < size() );  return std::addressof(r)[i]; }
	};
	
	using RGBA32f	= RGBAColor< float >;
	using RGBA32i	= RGBAColor< int >;
	using RGBA32u	= RGBAColor< uint >;
	using RGBA8u	= RGBAColor< uint8_t >;



	//
	// Depth Stencil
	//

	struct DepthStencil
	{
	// variables
		float	depth;
		uint	stencil;

	// mathods
		constexpr DepthStencil () : depth(0.0f), stencil(0) {}
		constexpr explicit DepthStencil (float depth, uint stencil = 0) : depth(depth), stencil(stencil) {}

		ND_ bool operator == (const DepthStencil &rhs) const
		{
			return Equals( depth, rhs.depth ) and stencil == rhs.stencil;
		}
	};



	//
	// HSV Color
	//

	struct HSVColor
	{
	// variables
		float	h, s, v;

	// methods
		constexpr HSVColor () : h{0.0f}, s{0.0f}, v{0.0f} {}
		
		explicit constexpr HSVColor (float h, float s = 1.0f, float v = 1.0f) : h{h}, s{s}, v{v} {}

		explicit HSVColor (const RGBA32f &c)
		{
			// from http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
			float4 K = float4( 0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f );
			float4 p = c.g < c.b ? float4(c.b, c.g, K.w, K.z) : float4(c.g, c.b, K.x, K.y);
			float4 q = c.r < p.x ? float4(p.x, p.y, p.w, c.r) : float4(c.r, p.y, p.z, p.x);
			float  d = q.x - Min(q.w, q.y);
			float  e = 1.0e-10f;
			h = Abs(q.z + (q.w - q.y) / (6.0f * d + e));
			s = d / (q.x + e);
			v = q.x;
		}

		ND_ constexpr bool  operator == (const HSVColor &rhs) const
		{
			const float eps = RGBA32f::Epsilon();

			return Equals( h, rhs.h, eps ) and Equals( s, rhs.s, eps ) and Equals( v, rhs.v, eps );
		}
	};
	

	
/*
=================================================
	RGBA32f
=================================================
*/
	template <>
	inline RGBAColor<float>::RGBAColor (const HSVColor &c, float alpha)
	{
		// from http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
		float4 K = float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
		float3 p = Abs(Fract(c.h + float3(K.x, K.y, K.z)) * 6.0f - K.w);
		r = c.v * Lerp(K.x, Clamp(p.x - K.x, 0.0f, 1.0f), c.s);
		g = c.v * Lerp(K.x, Clamp(p.y - K.x, 0.0f, 1.0f), c.s);
		b = c.v * Lerp(K.x, Clamp(p.z - K.x, 0.0f, 1.0f), c.s);
		a = alpha;
	}

	template <> template <>
	inline constexpr RGBAColor<float>::RGBAColor (const RGBAColor<uint8_t> &other) :
		r{ float(other.r) / 255.0f }, g{ float(other.g) / 255.0f },
		b{ float(other.b) / 255.0f }, a{ float(other.a) / 255.0f }
	{}
	
	template <> template <>
	inline constexpr RGBAColor<float>::RGBAColor (const RGBAColor<uint> &other) :
		r{ float(other.r) / 0xFFFFFFFFu }, g{ float(other.g) / 0xFFFFFFFFu },
		b{ float(other.b) / 0xFFFFFFFFu }, a{ float(other.a) / 0xFFFFFFFFu }
	{}

/*
=================================================
	RGBA32i
=================================================
*/
	template <> template <>
	inline constexpr RGBAColor<int>::RGBAColor (const RGBAColor<uint> &other) :
		r{int(other.r)}, g{int(other.g)}, b{int(other.b)}, a{int(other.a)}
	{}

	template <> template <>
	inline constexpr RGBAColor<int>::RGBAColor (const RGBAColor<uint8_t> &other) :
		r{int(other.r)}, g{int(other.g)}, b{int(other.b)}, a{int(other.a)}
	{}
	
/*
=================================================
	RGBA32u
=================================================
*/
	template <> template <>
	inline constexpr RGBAColor<uint>::RGBAColor (const RGBAColor<int> &other) :
		r{uint(other.r)}, g{uint(other.g)}, b{uint(other.b)}, a{uint(other.a)}
	{}

	template <> template <>
	inline constexpr RGBAColor<uint>::RGBAColor (const RGBAColor<uint8_t> &other) :
		r{uint(other.r)}, g{uint(other.g)}, b{uint(other.b)}, a{uint(other.a)}
	{}
	
/*
=================================================
	RGBA8u
=================================================
*/
	template <> template <>
	inline constexpr RGBAColor<uint8_t>::RGBAColor (const RGBAColor<int> &other) :
		r{uint8_t(other.r)}, g{uint8_t(other.g)}, b{uint8_t(other.b)}, a{uint8_t(other.a)}
	{}

	template <> template <>
	inline constexpr RGBAColor<uint8_t>::RGBAColor (const RGBAColor<uint> &other) :
		r{uint8_t(other.r)}, g{uint8_t(other.g)}, b{uint8_t(other.b)}, a{uint8_t(other.a)}
	{}
	
	template <> template <>
	inline constexpr RGBAColor<uint8_t>::RGBAColor (const RGBAColor<float> &other) :
		r{uint8_t(other.r * 255.0f + 0.5f)},  g{uint8_t(other.g * 255.0f + 0.5f)},
		b{uint8_t(other.b * 255.0f + 0.5f)},  a{uint8_t(other.a * 255.0f + 0.5f)}
	{}

	ND_ inline constexpr RGBA8u  AdjustContrast (const RGBA8u &col, float factor)
	{
		constexpr float	mid = 127.0f;
		RGBA8u			result;
		result.r = uint8_t(mid + factor * (float(col.r) - mid) + 0.5f);
		result.g = uint8_t(mid + factor * (float(col.g) - mid) + 0.5f);
		result.b = uint8_t(mid + factor * (float(col.b) - mid) + 0.5f);
		result.a = col.a;
		return result;
	}
	
	ND_ inline constexpr float  Luminance (const RGBA8u &col)
	{
		constexpr float  scale = 1.0f / (255.0f * 255.0f * 255.0f);
		return (float(col.r) * 0.2126f + float(col.g) * float(col.b) * 0.7152f + 0.0722f) * scale;
	}

	ND_ inline constexpr RGBA8u  AdjustSaturation (const RGBA8u &col, float factor)
	{
		RGBA8u			result;
		const float		lum		= Luminance( col );
		result.r = uint8_t(lum + factor * (float(col.r) - lum) + 0.5f);
		result.g = uint8_t(lum + factor * (float(col.g) - lum) + 0.5f);
		result.b = uint8_t(lum + factor * (float(col.b) - lum) + 0.5f);
		result.a = col.a;
		return result;
	}

	ND_ inline constexpr RGBA8u  Lerp (const RGBA8u &x, const RGBA8u &y, float factor)
	{
		float4 v = Lerp( float4{float(x.r), float(x.g), float(x.b), float(x.a)},
						 float4{float(y.r), float(y.g), float(y.b), float(y.a)}, factor );

		return RGBA8u{ uint8_t(v.x + 0.5f), uint8_t(v.y + 0.5f),
					   uint8_t(v.z + 0.5f), uint8_t(v.w + 0.5f) };
	}
	
/*
=================================================
	Min/Max/Clamp
=================================================
*/
	template <typename T>
	ND_ inline constexpr RGBAColor<T>  Min (const RGBAColor<T> &lhs, const RGBAColor<T> &rhs)
	{
		return RGBAColor<T>{ Min(lhs.r, rhs.r), Min(lhs.g, rhs.g), Min(lhs.b, rhs.b), Min(lhs.a, rhs.a) };
	}

	template <typename T>
	ND_ inline constexpr RGBAColor<T>  Max (const RGBAColor<T> &lhs, const RGBAColor<T> &rhs)
	{
		return RGBAColor<T>{ Max(lhs.r, rhs.r), Max(lhs.g, rhs.g), Max(lhs.b, rhs.b), Max(lhs.a, rhs.a) };
	}
	
	template <typename T>
	ND_ inline constexpr RGBAColor<T>  Clamp (const RGBAColor<T> &value, const RGBAColor<T> &minVal, const RGBAColor<T> &maxVal)
	{
		return Min( maxVal, Max( value, minVal ) );
	}
	
/*
=================================================
	Equals
=================================================
*/
	template <typename T>
	ND_ inline constexpr bool4  Equals (const RGBAColor<T> &lhs, const RGBAColor<T> &rhs, const T &err = std::numeric_limits<T>::epsilon() * T(2))
	{
		return bool4{ Equals( lhs.r, rhs.r, err ), Equals( lhs.g, rhs.g, err ), Equals( lhs.b, rhs.b, err ), Equals( lhs.a, rhs.a, err ) };
	}

/*
=================================================
	HtmlColor
=================================================
*/
	struct HtmlColor
	{
		// see https://www.w3schools.com/colors/colors_names.asp
#		define DEF_COLOR( _name_, _color_ )	static constexpr RGBA8u	_name_ \
											{ (_color_ >> 0) & 0xFF, (_color_ >> 8) & 0xFF, (_color_ >> 16) & 0xFF, 0xFF };

		DEF_COLOR( AliceBlue,				0xF0F8FF )
		DEF_COLOR( AntiqueWhite,			0xFAEBD7 )
		DEF_COLOR( Aqua,					0x00FFFF )
		DEF_COLOR( Aquamarine,				0x7FFFD4 )
		DEF_COLOR( Azure,					0xF0FFFF )
		DEF_COLOR( Beige,					0xF5F5DC )
		DEF_COLOR( Bisque,					0xFFE4C4 )
		DEF_COLOR( Black,					0x000000 )
		DEF_COLOR( BlanchedAlmond,			0xFFEBCD )
		DEF_COLOR( Blue,					0x0000FF )
		DEF_COLOR( BlueViolet,				0x8A2BE2 )
		DEF_COLOR( Brown,					0xA52A2A )
		DEF_COLOR( BurlyWood,				0xDEB887 )
		DEF_COLOR( CadetBlue,				0x5F9EA0 )
		DEF_COLOR( Chartreuse,				0x7FFF00 )
		DEF_COLOR( Chocolate,				0xD2691E )
		DEF_COLOR( Coral,					0xFF7F50 )
		DEF_COLOR( CornflowerBlue,			0x6495ED )
		DEF_COLOR( Cornsilk,				0xFFF8DC )
		DEF_COLOR( Crimson,					0xDC143C )
		DEF_COLOR( Cyan,					0x00FFFF )
		DEF_COLOR( DarkBlue,				0x00008B )
		DEF_COLOR( DarkCyan,				0x008B8B )
		DEF_COLOR( DarkGoldenRod,			0xB8860B )
		DEF_COLOR( DarkGray,				0xA9A9A9 )
		DEF_COLOR( DarkGreen,				0x006400 )
		DEF_COLOR( DarkKhaki,				0xBDB76B )
		DEF_COLOR( DarkMagenta,				0x8B008B )
		DEF_COLOR( DarkOliveGreen,			0x556B2F )
		DEF_COLOR( DarkOrange,				0xFF8C00 )
		DEF_COLOR( DarkOrchid,				0x9932CC )
		DEF_COLOR( DarkRed,					0x8B0000 )
		DEF_COLOR( DarkSalmon,				0xE9967A )
		DEF_COLOR( DarkSeaGreen,			0x8FBC8F )
		DEF_COLOR( DarkSlateBlue,			0x483D8B )
		DEF_COLOR( DarkSlateGray,			0x2F4F4F )
		DEF_COLOR( DarkTurquoise,			0x00CED1 )
		DEF_COLOR( DarkViolet,				0x9400D3 )
		DEF_COLOR( DeepPink,				0xFF1493 )
		DEF_COLOR( DeepSkyBlue,				0x00BFFF )
		DEF_COLOR( DimGray,					0x696969 )
		DEF_COLOR( DodgerBlue,				0x1E90FF )
		DEF_COLOR( FireBrick,				0xB22222 )
		DEF_COLOR( FloralWhite,				0xFFFAF0 )
		DEF_COLOR( ForestGreen,				0x228B22 )
		DEF_COLOR( Fuchsia,					0xFF00FF )
		DEF_COLOR( Gainsboro,				0xDCDCDC )
		DEF_COLOR( GhostWhite,				0xF8F8FF )
		DEF_COLOR( Gold,					0xFFD700 )
		DEF_COLOR( GoldenRod,				0xDAA520 )
		DEF_COLOR( Gray,					0x808080 )
		DEF_COLOR( Green,					0x008000 )
		DEF_COLOR( GreenYellow,				0xADFF2F )
		DEF_COLOR( HoneyDew,				0xF0FFF0 )
		DEF_COLOR( HotPink,					0xFF69B4 )
		DEF_COLOR( IndianRed,				0xCD5C5C )
		DEF_COLOR( Indigo,					0x4B0082 )
		DEF_COLOR( Ivory,					0xFFFFF0 )
		DEF_COLOR( Khaki,					0xF0E68C )
		DEF_COLOR( Lavender,				0xE6E6FA )
		DEF_COLOR( LavenderBlush,			0xFFF0F5 )
		DEF_COLOR( LawnGreen,				0x7CFC00 )
		DEF_COLOR( LemonChiffon,			0xFFFACD )
		DEF_COLOR( LightBlue,				0xADD8E6 )
		DEF_COLOR( LightCoral,				0xF08080 )
		DEF_COLOR( LightCyan,				0xE0FFFF )
		DEF_COLOR( LightGoldenRodYellow,	0xFAFAD2 )
		DEF_COLOR( LightGray,				0xD3D3D3 )
		DEF_COLOR( LightGreen,				0x90EE90 )
		DEF_COLOR( LightPink,				0xFFB6C1 )
		DEF_COLOR( LightSalmon,				0xFFA07A )
		DEF_COLOR( LightSeaGreen,			0x20B2AA )
		DEF_COLOR( LightSkyBlue,			0x87CEFA )
		DEF_COLOR( LightSlateGray,			0x778899 )
		DEF_COLOR( LightSteelBlue,			0xB0C4DE )
		DEF_COLOR( LightYellow,				0xFFFFE0 )
		DEF_COLOR( Lime,					0x00FF00 )
		DEF_COLOR( LimeGreen,				0x32CD32 )
		DEF_COLOR( Linen,					0xFAF0E6 )
		DEF_COLOR( Magenta,					0xFF00FF )
		DEF_COLOR( Maroon,					0x800000 )
		DEF_COLOR( MediumAquaMarine,		0x66CDAA )
		DEF_COLOR( MediumBlue,				0x0000CD )
		DEF_COLOR( MediumOrchid,			0xBA55D3 )
		DEF_COLOR( MediumPurple,			0x9370DB )
		DEF_COLOR( MediumSeaGreen,			0x3CB371 )
		DEF_COLOR( MediumSlateBlue,			0x7B68EE )
		DEF_COLOR( MediumSpringGreen,		0x00FA9A )
		DEF_COLOR( MediumTurquoise,			0x48D1CC )
		DEF_COLOR( MediumVioletRed,			0xC71585 )
		DEF_COLOR( MidnightBlue,			0x191970 )
		DEF_COLOR( MintCream,				0xF5FFFA )
		DEF_COLOR( MistyRose,				0xFFE4E1 )
		DEF_COLOR( Moccasin,				0xFFE4B5 )
		DEF_COLOR( NavajoWhite,				0xFFDEAD )
		DEF_COLOR( Navy,					0x000080 )
		DEF_COLOR( OldLace,					0xFDF5E6 )
		DEF_COLOR( Olive,					0x808000 )
		DEF_COLOR( OliveDrab,				0x6B8E23 )
		DEF_COLOR( Orange,					0xFFA500 )
		DEF_COLOR( OrangeRed,				0xFF4500 )
		DEF_COLOR( Orchid,					0xDA70D6 )
		DEF_COLOR( PaleGoldenRod,			0xEEE8AA )
		DEF_COLOR( PaleGreen,				0x98FB98 )
		DEF_COLOR( PaleTurquoise,			0xAFEEEE )
		DEF_COLOR( PaleVioletRed,			0xDB7093 )
		DEF_COLOR( PapayaWhip,				0xFFEFD5 )
		DEF_COLOR( PeachPuff,				0xFFDAB9 )
		DEF_COLOR( Peru,					0xCD853F )
		DEF_COLOR( Pink,					0xFFC0CB )
		DEF_COLOR( Plum,					0xDDA0DD )
		DEF_COLOR( PowderBlue,				0xB0E0E6 )
		DEF_COLOR( Purple,					0x800080 )
		DEF_COLOR( Red,						0xFF0000 )
		DEF_COLOR( RosyBrown,				0xBC8F8F )
		DEF_COLOR( RoyalBlue,				0x4169E1 )
		DEF_COLOR( SaddleBrown,				0x8B4513 )
		DEF_COLOR( Salmon,					0xFA8072 )
		DEF_COLOR( SandyBrown,				0xF4A460 )
		DEF_COLOR( SeaGreen,				0x2E8B57 )
		DEF_COLOR( SeaShell,				0xFFF5EE )
		DEF_COLOR( Sienna,					0xA0522D )
		DEF_COLOR( Silver,					0xC0C0C0 )
		DEF_COLOR( SkyBlue,					0x87CEEB )
		DEF_COLOR( SlateBlue,				0x6A5ACD )
		DEF_COLOR( SlateGray,				0x708090 )
		DEF_COLOR( Snow,					0xFFFAFA )
		DEF_COLOR( SpringGreen,				0x00FF7F )
		DEF_COLOR( SteelBlue,				0x4682B4 )
		DEF_COLOR( Tan,						0xD2B48C )
		DEF_COLOR( Teal,					0x008080 )
		DEF_COLOR( Thistle,					0xD8BFD8 )
		DEF_COLOR( Tomato,					0xFF6347 )
		DEF_COLOR( Turquoise,				0x40E0D0 )
		DEF_COLOR( Violet,					0xEE82EE )
		DEF_COLOR( Wheat,					0xF5DEB3 )
		DEF_COLOR( White,					0xFFFFFF )
		DEF_COLOR( WhiteSmoke,				0xF5F5F5 )
		DEF_COLOR( Yellow,					0xFFFF00 )
		DEF_COLOR( YellowGreen,				0x9ACD32 )

#		undef DEF_COLOR
	};

}	// FG


namespace std
{
	template <typename T>
	struct hash< FG::RGBAColor<T> >
	{
		ND_ size_t  operator () (const FG::RGBAColor<T> &value) const noexcept
		{
			return	size_t(	FG::HashOf( value.r ) + FG::HashOf( value.g ) +
							FG::HashOf( value.b ) + FG::HashOf( value.a ));
		}
	};
	

	template <>
	struct hash< FG::DepthStencil >
	{
		ND_ size_t  operator () (const FG::DepthStencil &value) const noexcept
		{
			return size_t(FG::HashOf( value.depth ) + FG::HashOf( value.stencil ));
		}
	};

}	// std

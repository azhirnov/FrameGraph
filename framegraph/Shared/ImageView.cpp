// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Public/ImageView.h"

namespace FG
{
namespace {
	struct FloatBits
	{
		uint	m	: 23;	// mantissa bits
		uint	e	: 8;	// exponent bits
		uint	s	: 1;	// sign bit

		FloatBits () : m{0}, e{0}, s{0} {}
	};


	struct HalfBits
	{
		uint16_t	m	: 10;	// mantissa bits
		uint16_t	e	: 5;	// exponent bits
		uint16_t	s	: 1;	// sign bit

		HalfBits () : m{0}, e{0}, s{0} {}

		ND_ float  ToFloat () const;
	};
	
/*
=================================================
	ToFloat
=================================================
*/
	inline float  HalfBits::ToFloat () const
	{
		FloatBits	f;
		f.s = s;
		f.e = e + (127 - 15);
		f.m = m << (23-10);

		float	result;
		std::memcpy( OUT &result, &f, sizeof(result) );
		return result;
	}
}
//-----------------------------------------------------------------------------



/*
=================================================
	ScaleUNorm
=================================================
*/
	template <uint Bits>
	forceinline float ScaleUNorm (uint value)
	{
		STATIC_ASSERT( Bits <= 32 );

		FloatBits	f;
		f.e	= 127 + Bits;

		return float(value) / BitCast<float>(f);
	}

/*
=================================================
	ScaleSNorm
=================================================
*/
	template <uint Bits>
	forceinline float ScaleSNorm (int value)
	{
		STATIC_ASSERT( Bits <= 32 );

		FloatBits	f;
		f.e	= uint(127 + (value >= 0 ? int(Bits) : -int(Bits)));

		return float(value) / BitCast<float>(f);
	}

/*
=================================================
	ReadUIntScalar
=================================================
*/
	template <uint Bits, uint OffsetBits>
	forceinline uint ReadUIntScalar (const StaticArray<uint,4> &data)
	{
		STATIC_ASSERT( Bits <= 32 );
		STATIC_ASSERT( Bits + (OffsetBits & 31) <= 32 );
		
		if constexpr ( Bits == 0 )
		{
			(void)(data);
			return 0;
		}
		else
		{
			constexpr uint	mask	= (~0u >> (32 - Bits));
			constexpr uint	offset	= (OffsetBits % 32);
			constexpr uint	index	= (OffsetBits / 32);

			return (data[index] >> offset) & mask;
		}
	}

/*
=================================================
	ReadIntScalar
=================================================
*/
	template <uint Bits, uint OffsetBits>
	forceinline int ReadIntScalar (const StaticArray<uint,4> &data)
	{
		const uint	value = ReadUIntScalar< Bits, OffsetBits >( data );
		
		if constexpr ( Bits == 0 )
			return 0;
		else
		if constexpr ( Bits == 32 )
			return int(value);
		else
			return (value >> (Bits-1)) ? -int(value) : int(value);	// TODO: check
	}

/*
=================================================
	ReadInt
=================================================
*/
	template <uint R, uint G, uint B, uint A>
	static void ReadInt (ArrayView<ImageView::T> pixel, OUT RGBA32i &result)
	{
		StaticArray< uint, 4 >	bits;
		std::memcpy( bits.data(), pixel.data(), Min( (R+G+B+A+7)/8, size_t(ArraySizeOf(pixel)) ));

		result.r = ReadIntScalar< R, 0 >( bits );
		result.g = ReadIntScalar< G, R >( bits );
		result.b = ReadIntScalar< B, R+G >( bits );
		result.a = ReadIntScalar< A, R+G+B >( bits );
	}

/*
=================================================
	ReadUInt
=================================================
*/
	template <uint R, uint G, uint B, uint A>
	static void ReadUInt (ArrayView<ImageView::T> pixel, OUT RGBA32u &result)
	{
		StaticArray< uint, 4 >	bits;
		std::memcpy( bits.data(), pixel.data(), Min( (R+G+B+A+7)/8, size_t(ArraySizeOf(pixel)) ));

		result.r = ReadUIntScalar< R, 0 >( bits );
		result.g = ReadUIntScalar< G, R >( bits );
		result.b = ReadUIntScalar< B, R+G >( bits );
		result.a = ReadUIntScalar< A, R+G+B >( bits );
	}

/*
=================================================
	ReadUNorm
=================================================
*/
	template <uint R, uint G, uint B, uint A>
	static void ReadUNorm (ArrayView<ImageView::T> pixel, OUT RGBA32f &result)
	{
		RGBA32u		c;
		ReadUInt<R,G,B,A>( pixel, OUT c );

		result.r = ScaleUNorm<R>( c.r );
		result.g = ScaleUNorm<G>( c.g );
		result.b = ScaleUNorm<B>( c.b );
		result.a = ScaleUNorm<A>( c.a );
	}
	
/*
=================================================
	ReadSNorm
=================================================
*/
	template <uint R, uint G, uint B, uint A>
	static void ReadSNorm (ArrayView<ImageView::T> pixel, OUT RGBA32f &result)
	{
		RGBA32i		c;
		ReadInt<R,G,B,A>( pixel, OUT c );
		
		result.r = ScaleSNorm<R>( c.r );
		result.g = ScaleSNorm<G>( c.g );
		result.b = ScaleSNorm<B>( c.b );
		result.a = ScaleSNorm<A>( c.a );
	}
	
/*
=================================================
	ReadFloat
=================================================
*/
	template <uint R, uint G, uint B, uint A>
	static void ReadFloat (ArrayView<ImageView::T> pixel, OUT RGBA32f &result)
	{
		if constexpr ( R == 16 )
		{
			StaticArray< HalfBits, 4 >	src = {};
			std::memcpy( src.data(), pixel.data(), Min( (R+G+B+A+7)/8, size_t(ArraySizeOf(pixel)) ));

			for (size_t i = 0; i < src.size(); ++i)
			{
				result[i] = src[i].ToFloat();
			}
		}
		else
		if constexpr ( R == 32 )
		{
			result = {};
			std::memcpy( result.data(), pixel.data(), Min( (R+G+B+A+7)/8, size_t(ArraySizeOf(pixel)) ));
		}
		else
		{
			ASSERT( !"not supported" );
		}
	}
	
/*
=================================================
	ReadFloat_11_11_10
=================================================
*/
	static void ReadFloat_11_11_10 (ArrayView<ImageView::T> pixel, OUT RGBA32f &result)
	{
		struct RGBBits
		{
			// Blue //
			uint	b_m	: 5;
			uint	b_e	: 5;
			// Green //
			uint	g_m	: 6;
			uint	g_e	: 5;
			// Red //
			uint	r_m	: 6;
			uint	r_e	: 5;
		};
		STATIC_ASSERT( sizeof(RGBBits)*8 == (11+11+10) );

		RGBBits	bits;
		std::memcpy( &bits, pixel.data(), sizeof(bits) );

		FloatBits	f;
		
		f.m = bits.r_m << (23-6);
		f.e = bits.r_e + (127 - 15);
		result.r = BitCast<float>(f);

		f.m = bits.g_m << (23-6);
		f.e = bits.g_e + (127 - 15);
		result.g = BitCast<float>(f);

		f.m = bits.b_m << (23-5);
		f.e = bits.b_e + (127 - 15);
		result.b = BitCast<float>(f);

		result.a = 1.0f;
	}

/*
=================================================
	constructor
=================================================
*/
	ImageView::ImageView (ArrayView<ArrayView<T>> parts, const uint3 &dim, BytesU rowPitch, BytesU slicePitch, EPixelFormat format, EImageAspect aspect) :
		_parts{ parts },				_dimension{ dim },
		_rowPitch{ size_t(rowPitch) },	_slicePitch{ size_t(slicePitch) },
		_format{ format }
	{
		FG_UNUSED( aspect );
		ENABLE_ENUM_CHECKS();
		switch ( _format )
		{
			case EPixelFormat::RGBA4_UNorm :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 4*4;
				_loadF4			= &ReadUNorm<4,4,4,4>;
				_loadI4			= &ReadInt<4,4,4,4>;
				_loadU4			= &ReadUInt<4,4,4,4>;
				break;

			case EPixelFormat::RGB5_A1_UNorm :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 3*5 + 1;
				_loadF4			= &ReadUNorm<5,5,5,1>;
				_loadI4			= &ReadInt<5,5,5,1>;
				_loadU4			= &ReadUInt<5,5,5,1>;
				break;

			case EPixelFormat::RGB_5_6_5_UNorm :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 5+6+5;
				_loadF4			= &ReadUNorm<5,6,5,0>;
				_loadI4			= &ReadInt<5,6,5,0>;
				_loadU4			= &ReadUInt<5,6,5,0>;
				break;

			case EPixelFormat::RGB10_A2_UNorm :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 3*10 + 2;
				_loadF4			= &ReadUNorm<10,10,10,2>;
				_loadI4			= &ReadInt<10,10,10,2>;
				_loadU4			= &ReadUInt<10,10,10,2>;
				break;

			case EPixelFormat::R8_SNorm :
			case EPixelFormat::R8_UNorm :
			case EPixelFormat::R8I :
			case EPixelFormat::R8U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 1*8;
				_loadF4			= (_format == EPixelFormat::R8_SNorm ? &ReadSNorm<8,0,0,0> : _format == EPixelFormat::R8_UNorm ? &ReadUNorm<8,0,0,0> : null);
				_loadI4			= &ReadInt<8,0,0,0>;
				_loadU4			= &ReadUInt<8,0,0,0>;
				break;

			case EPixelFormat::RG8_SNorm :
			case EPixelFormat::RG8_UNorm :
			case EPixelFormat::RG8I :
			case EPixelFormat::RG8U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 2*8;
				_loadF4			= (_format == EPixelFormat::RG8_SNorm ? &ReadSNorm<8,8,0,0> : _format == EPixelFormat::RG8_UNorm ? &ReadUNorm<8,8,0,0> : null);
				_loadI4			= &ReadInt<8,8,0,0>;
				_loadU4			= &ReadUInt<8,8,0,0>;
				break;

			case EPixelFormat::RGB8_SNorm :
			case EPixelFormat::RGB8_UNorm :
			case EPixelFormat::RGB8I :
			case EPixelFormat::RGB8U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 3*8;
				_loadF4			= (_format == EPixelFormat::RGB8_SNorm ? &ReadSNorm<8,8,8,0> : _format == EPixelFormat::RGB8_UNorm ? &ReadUNorm<8,8,8,0> : null);
				_loadI4			= &ReadInt<8,8,8,0>;
				_loadU4			= &ReadUInt<8,8,8,0>;
				break;


			case EPixelFormat::RGBA8_SNorm :
			case EPixelFormat::RGBA8_UNorm :
			case EPixelFormat::RGBA8I :
			case EPixelFormat::RGBA8U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 4*8;
				_loadF4			= (_format == EPixelFormat::RGBA8_SNorm ? &ReadSNorm<8,8,8,8> : _format == EPixelFormat::RGBA8_UNorm ? &ReadUNorm<8,8,8,8> : null);
				_loadI4			= &ReadInt<8,8,8,8>;
				_loadU4			= &ReadUInt<8,8,8,8>;
				break;

			case EPixelFormat::R16_SNorm :
			case EPixelFormat::R16_UNorm :
			case EPixelFormat::R16I :
			case EPixelFormat::R16U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 1*16;
				_loadF4			= (_format == EPixelFormat::R16_SNorm ? &ReadSNorm<16,0,0,0> : _format == EPixelFormat::R16_UNorm ? &ReadUNorm<16,0,0,0> : null);
				_loadI4			= &ReadInt<16,0,0,0>;
				_loadU4			= &ReadUInt<16,0,0,0>;
				break;

			case EPixelFormat::RG16_SNorm :
			case EPixelFormat::RG16_UNorm :
			case EPixelFormat::RG16I :
			case EPixelFormat::RG16U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 2*16;
				_loadF4			= (_format == EPixelFormat::RG16_SNorm ? &ReadSNorm<16,16,0,0> : _format == EPixelFormat::RG16_UNorm ? &ReadUNorm<16,16,0,0> : null);
				_loadI4			= &ReadInt<16,16,0,0>;
				_loadU4			= &ReadUInt<16,16,0,0>;
				break;

			case EPixelFormat::RGB16_SNorm :
			case EPixelFormat::RGB16_UNorm :
			case EPixelFormat::RGB16I :
			case EPixelFormat::RGB16U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 3*16;
				_loadF4			= (_format == EPixelFormat::RGB16_SNorm ? &ReadSNorm<16,16,16,0> : _format == EPixelFormat::RGB16_UNorm ? &ReadUNorm<16,16,16,0> : null);
				_loadI4			= &ReadInt<16,16,16,0>;
				_loadU4			= &ReadUInt<16,16,16,0>;
				break;

			case EPixelFormat::RGBA16_SNorm :
			case EPixelFormat::RGBA16_UNorm :
			case EPixelFormat::RGBA16I :
			case EPixelFormat::RGBA16U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 4*16;
				_loadF4			= (_format == EPixelFormat::RGBA16_SNorm ? &ReadSNorm<16,16,16,16> : _format == EPixelFormat::RGBA16_UNorm ? &ReadUNorm<16,16,16,16> : null);
				_loadI4			= &ReadInt<16,16,16,16>;
				_loadU4			= &ReadUInt<16,16,16,16>;
				break;

			case EPixelFormat::R32I :
			case EPixelFormat::R32U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 1*32;
				_loadI4			= &ReadInt<32,0,0,0>;
				_loadU4			= &ReadUInt<32,0,0,0>;
				break;

			case EPixelFormat::RG32I :
			case EPixelFormat::RG32U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 2*32;
				_loadI4			= &ReadInt<32,32,0,0>;
				_loadU4			= &ReadUInt<32,32,0,0>;
				break;

			case EPixelFormat::RGB32I :
			case EPixelFormat::RGB32U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 3*32;
				_loadI4			= &ReadInt<32,32,32,0>;
				_loadU4			= &ReadUInt<32,32,32,0>;
				break;

			case EPixelFormat::RGBA32I :
			case EPixelFormat::RGBA32U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 4*32;
				_loadI4			= &ReadInt<32,32,32,32>;
				_loadU4			= &ReadUInt<32,32,32,32>;
				break;

			case EPixelFormat::RGB10_A2U :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 3*10 + 2;
				_loadI4			= &ReadInt<10,10,10,2>;
				_loadU4			= &ReadUInt<10,10,10,2>;
				break;

			case EPixelFormat::R16F :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 1*16;
				_loadF4			= &ReadFloat<16,0,0,0>;
				break;

			case EPixelFormat::RG16F :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 2*16;
				_loadF4			= &ReadFloat<16,16,0,0>;
				break;

			case EPixelFormat::RGB16F :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 3*16;
				_loadF4			= &ReadFloat<16,16,16,0>;
				break;

			case EPixelFormat::RGBA16F :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 4*16;
				_loadF4			= &ReadFloat<16,16,16,16>;
				break;

			case EPixelFormat::RGB_11_11_10F :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 11 + 11 + 10;
				_loadF4			= &ReadFloat_11_11_10;
				break;

			case EPixelFormat::R32F :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 1*32;
				_loadF4			= &ReadFloat<32,0,0,0>;
				break;

			case EPixelFormat::RG32F :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 2*32;
				_loadF4			= &ReadFloat<32,32,0,0>;
				break;

			case EPixelFormat::RGB32F :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 3*32;
				_loadF4			= &ReadFloat<32,32,32,0>;
				break;

			case EPixelFormat::RGBA32F :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 4*32;
				_loadF4			= &ReadFloat<32,32,32,32>;
				break;

			case EPixelFormat::Depth16 :
			case EPixelFormat::Depth24 :
			case EPixelFormat::Depth32F :
				ASSERT( aspect == EImageAspect::Depth );
				ASSERT(false);
				break;

			case EPixelFormat::Depth16_Stencil8	:
			case EPixelFormat::Depth24_Stencil8 :
			case EPixelFormat::Depth32F_Stencil8 :
				ASSERT( aspect == EImageAspect::Depth or aspect == EImageAspect::Stencil );
				ASSERT(false);
				break;

			case EPixelFormat::sRGB8 :
			case EPixelFormat::sBGR8 :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 3*8;
				_loadF4			= &ReadUNorm<8,8,8,0>;
				_loadI4			= &ReadInt<8,8,8,0>;
				_loadU4			= &ReadUInt<8,8,8,0>;
				break;

			case EPixelFormat::sRGB8_A8 :
			case EPixelFormat::sBGR8_A8 :
				ASSERT( aspect == EImageAspect::Color );
				_bitsPerPixel	= 4*8;
				_loadF4			= &ReadUNorm<8,8,8,8>;
				_loadI4			= &ReadInt<8,8,8,8>;
				_loadU4			= &ReadUInt<8,8,8,8>;
				break;
				
			case EPixelFormat::BGR8_UNorm :
			case EPixelFormat::BGRA8_UNorm :
			case EPixelFormat::BC1_RGB8_UNorm :
			case EPixelFormat::BC1_sRGB8_UNorm :
			case EPixelFormat::BC1_RGB8_A1_UNorm :
			case EPixelFormat::BC1_sRGB8_A1_UNorm :
			case EPixelFormat::BC2_RGBA8_UNorm :
			case EPixelFormat::BC3_RGBA8_UNorm :
			case EPixelFormat::BC3_sRGB :
			case EPixelFormat::BC4_RED8_SNorm :
			case EPixelFormat::BC4_RED8_UNorm :
			case EPixelFormat::BC5_RG8_SNorm :
			case EPixelFormat::BC5_RG8_UNorm :
			case EPixelFormat::BC7_RGBA8_UNorm :
			case EPixelFormat::BC7_SRGB8_A8 :
			case EPixelFormat::BC6H_RGB16F :
			case EPixelFormat::BC6H_RGB16UF :
			case EPixelFormat::ETC2_RGB8_UNorm :
			case EPixelFormat::ECT2_SRGB8 :
			case EPixelFormat::ETC2_RGB8_A1_UNorm :
			case EPixelFormat::ETC2_SRGB8_A1 :
			case EPixelFormat::ETC2_RGBA8_UNorm :
			case EPixelFormat::ETC2_SRGB8_A8 :
			case EPixelFormat::EAC_R11_SNorm :
			case EPixelFormat::EAC_R11_UNorm :
			case EPixelFormat::EAC_RG11_SNorm :
			case EPixelFormat::EAC_RG11_UNorm :
			case EPixelFormat::ASTC_RGBA_4x4 :
			case EPixelFormat::ASTC_RGBA_5x4 :
			case EPixelFormat::ASTC_RGBA_5x5 :
			case EPixelFormat::ASTC_RGBA_6x5 :
			case EPixelFormat::ASTC_RGBA_6x6 :
			case EPixelFormat::ASTC_RGBA_8x5 :
			case EPixelFormat::ASTC_RGBA_8x6 :
			case EPixelFormat::ASTC_RGBA_8x8 :
			case EPixelFormat::ASTC_RGBA_10x5 :
			case EPixelFormat::ASTC_RGBA_10x6 :
			case EPixelFormat::ASTC_RGBA_10x8 :
			case EPixelFormat::ASTC_RGBA_10x10 :
			case EPixelFormat::ASTC_RGBA_12x10 :
			case EPixelFormat::ASTC_RGBA_12x12 :
			case EPixelFormat::ASTC_SRGB8_A8_4x4 :
			case EPixelFormat::ASTC_SRGB8_A8_5x4 :
			case EPixelFormat::ASTC_SRGB8_A8_5x5 :
			case EPixelFormat::ASTC_SRGB8_A8_6x5 :
			case EPixelFormat::ASTC_SRGB8_A8_6x6 :
			case EPixelFormat::ASTC_SRGB8_A8_8x5 :
			case EPixelFormat::ASTC_SRGB8_A8_8x6 :
			case EPixelFormat::ASTC_SRGB8_A8_8x8 :
			case EPixelFormat::ASTC_SRGB8_A8_10x5 :
			case EPixelFormat::ASTC_SRGB8_A8_10x6 :
			case EPixelFormat::ASTC_SRGB8_A8_10x8 :
			case EPixelFormat::ASTC_SRGB8_A8_10x10 :
			case EPixelFormat::ASTC_SRGB8_A8_12x10 :
			case EPixelFormat::ASTC_SRGB8_A8_12x12 :
				ASSERT( aspect == EImageAspect::Color );
				ASSERT(false);
				break;	// TODO

			case EPixelFormat::_Count :
			case EPixelFormat::Unknown :
				break;	// to shutup warnings
		}
		DISABLE_ENUM_CHECKS();
	}

}	// FG

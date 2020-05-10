// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Common.h"

namespace FG
{
	
	enum DDSPF_FLAGS : uint
	{
		DDPF_ALPHAPIXELS = 0x1,
		DDPF_ALPHA = 0x2,
		DDPF_FOURCC = 0x4,
		DDPF_RGB = 0x40,
		DDPF_YUV = 0x200,
		DDPF_LUMINANCE = 0x20000,
	};

	struct DDS_PIXELFORMAT
	{
		uint		dwSize;
		DDSPF_FLAGS	dwFlags;
		uint		dwFourCC;
		uint		dwRGBBitCount;
		uint		dwRBitMask;
		uint		dwGBitMask;
		uint		dwBBitMask;
		uint		dwABitMask;
	};

	enum DDS_FLAGS : uint
	{
		DDSD_CAPS = 0x1,
		DDSD_HEIGHT = 0x2,
		DDSD_WIDTH = 0x4,
		DDSD_PITCH = 0x8,
		DDSD_PIXELFORMAT = 0x1000,
		DDSD_MIPMAPCOUNT = 0x20000,
		DDSD_LINEARSIZE = 0x80000,
		DDSD_DEPTH = 0x800000,
	};
	
	enum DDS_CAPS : uint
	{
		DDSCAPS_COMPLEX = 0x8,
		DDSCAPS_MIPMAP = 0x400000,
		DDSCAPS_TEXTURE = 0x1000,
	};

	enum DDS_CAPS2 : uint
	{
		DDSCAPS2_CUBEMAP = 0x200,
		DDSCAPS2_CUBEMAP_POSITIVEX = 0x400,
		DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800,
		DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000,
		DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000,
		DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000,
		DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000,
		DDSCAPS2_VOLUME = 0x200000,
	};

	struct DDS_HEADER
	{
		uint            dwMagic;
		uint            dwSize;
		DDS_FLAGS       dwFlags;
		uint            dwHeight;
		uint            dwWidth;
		uint            dwPitchOrLinearSize;
		uint            dwDepth;
		uint            dwMipMapCount;
		uint            dwReserved1[11];
		DDS_PIXELFORMAT ddspf;
		DDS_CAPS        dwCaps;
		DDS_CAPS2       dwCaps2;
		uint            dwCaps3;
		uint            dwCaps4;
		uint            dwReserved2;
	};
	
	enum DXGI_FORMAT : uint
	{
		DXGI_FORMAT_UNKNOWN	                    = 0,
		DXGI_FORMAT_R32G32B32A32_TYPELESS       = 1,
		DXGI_FORMAT_R32G32B32A32_FLOAT          = 2,
		DXGI_FORMAT_R32G32B32A32_UINT           = 3,
		DXGI_FORMAT_R32G32B32A32_SINT           = 4,
		DXGI_FORMAT_R32G32B32_TYPELESS          = 5,
		DXGI_FORMAT_R32G32B32_FLOAT             = 6,
		DXGI_FORMAT_R32G32B32_UINT              = 7,
		DXGI_FORMAT_R32G32B32_SINT              = 8,
		DXGI_FORMAT_R16G16B16A16_TYPELESS       = 9,
		DXGI_FORMAT_R16G16B16A16_FLOAT          = 10,
		DXGI_FORMAT_R16G16B16A16_UNORM          = 11,
		DXGI_FORMAT_R16G16B16A16_UINT           = 12,
		DXGI_FORMAT_R16G16B16A16_SNORM          = 13,
		DXGI_FORMAT_R16G16B16A16_SINT           = 14,
		DXGI_FORMAT_R32G32_TYPELESS             = 15,
		DXGI_FORMAT_R32G32_FLOAT                = 16,
		DXGI_FORMAT_R32G32_UINT                 = 17,
		DXGI_FORMAT_R32G32_SINT                 = 18,
		DXGI_FORMAT_R32G8X24_TYPELESS           = 19,
		DXGI_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
		DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
		DXGI_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
		DXGI_FORMAT_R10G10B10A2_TYPELESS        = 23,
		DXGI_FORMAT_R10G10B10A2_UNORM           = 24,
		DXGI_FORMAT_R10G10B10A2_UINT            = 25,
		DXGI_FORMAT_R11G11B10_FLOAT             = 26,
		DXGI_FORMAT_R8G8B8A8_TYPELESS           = 27,
		DXGI_FORMAT_R8G8B8A8_UNORM              = 28,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
		DXGI_FORMAT_R8G8B8A8_UINT               = 30,
		DXGI_FORMAT_R8G8B8A8_SNORM              = 31,
		DXGI_FORMAT_R8G8B8A8_SINT               = 32,
		DXGI_FORMAT_R16G16_TYPELESS             = 33,
		DXGI_FORMAT_R16G16_FLOAT                = 34,
		DXGI_FORMAT_R16G16_UNORM                = 35,
		DXGI_FORMAT_R16G16_UINT                 = 36,
		DXGI_FORMAT_R16G16_SNORM                = 37,
		DXGI_FORMAT_R16G16_SINT                 = 38,
		DXGI_FORMAT_R32_TYPELESS                = 39,
		DXGI_FORMAT_D32_FLOAT                   = 40,
		DXGI_FORMAT_R32_FLOAT                   = 41,
		DXGI_FORMAT_R32_UINT                    = 42,
		DXGI_FORMAT_R32_SINT                    = 43,
		DXGI_FORMAT_R24G8_TYPELESS              = 44,
		DXGI_FORMAT_D24_UNORM_S8_UINT           = 45,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
		DXGI_FORMAT_X24_TYPELESS_G8_UINT        = 47,
		DXGI_FORMAT_R8G8_TYPELESS               = 48,
		DXGI_FORMAT_R8G8_UNORM                  = 49,
		DXGI_FORMAT_R8G8_UINT                   = 50,
		DXGI_FORMAT_R8G8_SNORM                  = 51,
		DXGI_FORMAT_R8G8_SINT                   = 52,
		DXGI_FORMAT_R16_TYPELESS                = 53,
		DXGI_FORMAT_R16_FLOAT                   = 54,
		DXGI_FORMAT_D16_UNORM                   = 55,
		DXGI_FORMAT_R16_UNORM                   = 56,
		DXGI_FORMAT_R16_UINT                    = 57,
		DXGI_FORMAT_R16_SNORM                   = 58,
		DXGI_FORMAT_R16_SINT                    = 59,
		DXGI_FORMAT_R8_TYPELESS                 = 60,
		DXGI_FORMAT_R8_UNORM                    = 61,
		DXGI_FORMAT_R8_UINT                     = 62,
		DXGI_FORMAT_R8_SNORM                    = 63,
		DXGI_FORMAT_R8_SINT                     = 64,
		DXGI_FORMAT_A8_UNORM                    = 65,
		DXGI_FORMAT_R1_UNORM                    = 66,
		DXGI_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
		DXGI_FORMAT_R8G8_B8G8_UNORM             = 68,
		DXGI_FORMAT_G8R8_G8B8_UNORM             = 69,
		DXGI_FORMAT_BC1_TYPELESS                = 70,
		DXGI_FORMAT_BC1_UNORM                   = 71,
		DXGI_FORMAT_BC1_UNORM_SRGB              = 72,
		DXGI_FORMAT_BC2_TYPELESS                = 73,
		DXGI_FORMAT_BC2_UNORM                   = 74,
		DXGI_FORMAT_BC2_UNORM_SRGB              = 75,
		DXGI_FORMAT_BC3_TYPELESS                = 76,
		DXGI_FORMAT_BC3_UNORM                   = 77,
		DXGI_FORMAT_BC3_UNORM_SRGB              = 78,
		DXGI_FORMAT_BC4_TYPELESS                = 79,
		DXGI_FORMAT_BC4_UNORM                   = 80,
		DXGI_FORMAT_BC4_SNORM                   = 81,
		DXGI_FORMAT_BC5_TYPELESS                = 82,
		DXGI_FORMAT_BC5_UNORM                   = 83,
		DXGI_FORMAT_BC5_SNORM                   = 84,
		DXGI_FORMAT_B5G6R5_UNORM                = 85,
		DXGI_FORMAT_B5G5R5A1_UNORM              = 86,
		DXGI_FORMAT_B8G8R8A8_UNORM              = 87,
		DXGI_FORMAT_B8G8R8X8_UNORM              = 88,
		DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
		DXGI_FORMAT_B8G8R8A8_TYPELESS           = 90,
		DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
		DXGI_FORMAT_B8G8R8X8_TYPELESS           = 92,
		DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
		DXGI_FORMAT_BC6H_TYPELESS               = 94,
		DXGI_FORMAT_BC6H_UF16                   = 95,
		DXGI_FORMAT_BC6H_SF16                   = 96,
		DXGI_FORMAT_BC7_TYPELESS                = 97,
		DXGI_FORMAT_BC7_UNORM                   = 98,
		DXGI_FORMAT_BC7_UNORM_SRGB              = 99,
		DXGI_FORMAT_AYUV                        = 100,
		DXGI_FORMAT_Y410                        = 101,
		DXGI_FORMAT_Y416                        = 102,
		DXGI_FORMAT_NV12                        = 103,
		DXGI_FORMAT_P010                        = 104,
		DXGI_FORMAT_P016                        = 105,
		DXGI_FORMAT_420_OPAQUE                  = 106,
		DXGI_FORMAT_YUY2                        = 107,
		DXGI_FORMAT_Y210                        = 108,
		DXGI_FORMAT_Y216                        = 109,
		DXGI_FORMAT_NV11                        = 110,
		DXGI_FORMAT_AI44                        = 111,
		DXGI_FORMAT_IA44                        = 112,
		DXGI_FORMAT_P8                          = 113,
		DXGI_FORMAT_A8P8                        = 114,
		DXGI_FORMAT_B4G4R4A4_UNORM              = 115,
		DXGI_FORMAT_P208                        = 130,
		DXGI_FORMAT_V208                        = 131,
		DXGI_FORMAT_V408                        = 132,
		DXGI_FORMAT_FORCE_UINT                  = 0xffffffff
	};

#	define DXGI_FG_PAIR( _visit_ ) \
		_visit_( DXGI_FORMAT_UNKNOWN,					EPixelFormat::Unknown ) \
		_visit_( DXGI_FORMAT_R32G32B32A32_FLOAT,		EPixelFormat::RGBA32F ) \
		_visit_( DXGI_FORMAT_R32G32B32A32_UINT,			EPixelFormat::RGBA32U ) \
		_visit_( DXGI_FORMAT_R32G32B32A32_SINT,			EPixelFormat::RGBA32I ) \
		_visit_( DXGI_FORMAT_R32G32B32_FLOAT,			EPixelFormat::RGB32F ) \
		_visit_( DXGI_FORMAT_R32G32B32_UINT,			EPixelFormat::RGB32U ) \
		_visit_( DXGI_FORMAT_R32G32B32_SINT,			EPixelFormat::RGB32I ) \
		_visit_( DXGI_FORMAT_R16G16B16A16_FLOAT,		EPixelFormat::RGBA16F ) \
		_visit_( DXGI_FORMAT_R16G16B16A16_UNORM,		EPixelFormat::RGBA16_UNorm ) \
		_visit_( DXGI_FORMAT_R16G16B16A16_UINT,			EPixelFormat::RGBA16U ) \
		_visit_( DXGI_FORMAT_R16G16B16A16_SNORM,		EPixelFormat::RGBA16_SNorm ) \
		_visit_( DXGI_FORMAT_R16G16B16A16_SINT,			EPixelFormat::RGBA16I ) \
		_visit_( DXGI_FORMAT_R32G32_FLOAT,				EPixelFormat::RG32F ) \
		_visit_( DXGI_FORMAT_R32G32_UINT,				EPixelFormat::RG32U ) \
		_visit_( DXGI_FORMAT_R32G32_SINT,				EPixelFormat::RG32I ) \
		_visit_( DXGI_FORMAT_D32_FLOAT_S8X24_UINT,		EPixelFormat::Depth24 ) \
		_visit_( DXGI_FORMAT_R10G10B10A2_UNORM,			EPixelFormat::RGB10_A2_UNorm ) \
		_visit_( DXGI_FORMAT_R10G10B10A2_UINT,			EPixelFormat::RGB10_A2U ) \
		_visit_( DXGI_FORMAT_R11G11B10_FLOAT,			EPixelFormat::RGB_11_11_10F ) \
		_visit_( DXGI_FORMAT_R8G8B8A8_UNORM,			EPixelFormat::RGBA8_UNorm ) \
		_visit_( DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,		EPixelFormat::sRGB8_A8 ) \
		_visit_( DXGI_FORMAT_R8G8B8A8_UINT,				EPixelFormat::RGBA8U ) \
		_visit_( DXGI_FORMAT_R8G8B8A8_SNORM,			EPixelFormat::RGBA8_SNorm ) \
		_visit_( DXGI_FORMAT_R8G8B8A8_SINT,				EPixelFormat::RGBA8I ) \
		_visit_( DXGI_FORMAT_R16G16_FLOAT,				EPixelFormat::RG16F ) \
		_visit_( DXGI_FORMAT_R16G16_UNORM,				EPixelFormat::RG16_UNorm ) \
		_visit_( DXGI_FORMAT_R16G16_UINT,				EPixelFormat::RG16U ) \
		_visit_( DXGI_FORMAT_R16G16_SNORM,				EPixelFormat::RG16_SNorm ) \
		_visit_( DXGI_FORMAT_R16G16_SINT,				EPixelFormat::RG16I ) \
		_visit_( DXGI_FORMAT_D32_FLOAT,					EPixelFormat::Depth32F ) \
		_visit_( DXGI_FORMAT_R32_FLOAT,					EPixelFormat::R32F ) \
		_visit_( DXGI_FORMAT_R32_UINT,					EPixelFormat::R32U ) \
		_visit_( DXGI_FORMAT_R32_SINT,					EPixelFormat::R32I ) \
		_visit_( DXGI_FORMAT_D24_UNORM_S8_UINT,			EPixelFormat::Depth24_Stencil8 ) \
		_visit_( DXGI_FORMAT_R8G8_UNORM,				EPixelFormat::RG8_UNorm ) \
		_visit_( DXGI_FORMAT_R8G8_UINT,					EPixelFormat::RG8U ) \
		_visit_( DXGI_FORMAT_R8G8_SNORM,				EPixelFormat::RG8_SNorm ) \
		_visit_( DXGI_FORMAT_R8G8_SINT,					EPixelFormat::RG8I ) \
		_visit_( DXGI_FORMAT_R16_FLOAT,					EPixelFormat::R16F ) \
		_visit_( DXGI_FORMAT_D16_UNORM,					EPixelFormat::Depth16 ) \
		_visit_( DXGI_FORMAT_R16_UNORM,					EPixelFormat::R16_UNorm ) \
		_visit_( DXGI_FORMAT_R16_UINT,					EPixelFormat::R16U ) \
		_visit_( DXGI_FORMAT_R16_SNORM,					EPixelFormat::R16_SNorm ) \
		_visit_( DXGI_FORMAT_R16_SINT,					EPixelFormat::R16I ) \
		_visit_( DXGI_FORMAT_R8_UNORM,					EPixelFormat::R8_UNorm ) \
		_visit_( DXGI_FORMAT_R8_UINT,					EPixelFormat::R8U ) \
		_visit_( DXGI_FORMAT_R8_SNORM,					EPixelFormat::R8_SNorm ) \
		_visit_( DXGI_FORMAT_R8_SINT,					EPixelFormat::R8I ) \
		_visit_( DXGI_FORMAT_BC1_UNORM,					EPixelFormat::BC1_RGB8_UNorm ) \
		_visit_( DXGI_FORMAT_BC1_UNORM_SRGB,			EPixelFormat::BC1_sRGB8 ) \
		_visit_( DXGI_FORMAT_BC2_UNORM,					EPixelFormat::BC2_RGBA8_UNorm ) \
		_visit_( DXGI_FORMAT_BC2_UNORM_SRGB,			EPixelFormat::BC2_sRGB8_A8 ) \
		_visit_( DXGI_FORMAT_BC3_UNORM,					EPixelFormat::BC3_RGBA8_UNorm ) \
		_visit_( DXGI_FORMAT_BC3_UNORM_SRGB,			EPixelFormat::BC3_sRGB8 ) \
		_visit_( DXGI_FORMAT_BC4_UNORM,					EPixelFormat::BC4_R8_UNorm ) \
		_visit_( DXGI_FORMAT_BC4_SNORM,					EPixelFormat::BC4_R8_SNorm ) \
		_visit_( DXGI_FORMAT_BC5_UNORM,					EPixelFormat::BC5_RG8_UNorm ) \
		_visit_( DXGI_FORMAT_BC5_SNORM,					EPixelFormat::BC5_RG8_SNorm ) \
		_visit_( DXGI_FORMAT_B5G6R5_UNORM,				EPixelFormat::RGB_5_6_5_UNorm ) \
		_visit_( DXGI_FORMAT_B5G5R5A1_UNORM,			EPixelFormat::RGB5_A1_UNorm ) \
		_visit_( DXGI_FORMAT_BC6H_UF16,					EPixelFormat::BC6H_RGB16UF ) \
		_visit_( DXGI_FORMAT_BC6H_SF16,					EPixelFormat::BC6H_RGB16F ) \
		_visit_( DXGI_FORMAT_BC7_UNORM,					EPixelFormat::BC7_RGBA8_UNorm ) \
		_visit_( DXGI_FORMAT_BC7_UNORM_SRGB,			EPixelFormat::BC7_sRGB8_A8 ) \

	enum D3D11_RESOURCE_DIMENSION : uint
	{
		D3D11_RESOURCE_DIMENSION_UNKNOWN = 0,
		D3D11_RESOURCE_DIMENSION_BUFFER = 1,
		D3D11_RESOURCE_DIMENSION_TEXTURE1D = 2,
		D3D11_RESOURCE_DIMENSION_TEXTURE2D = 3,
		D3D11_RESOURCE_DIMENSION_TEXTURE3D = 4
	};

	enum D3D11_RESOURCE_MISC_FLAG : uint
	{
		D3D11_RESOURCE_MISC_GENERATE_MIPS	= 0x1L,
		D3D11_RESOURCE_MISC_SHARED	= 0x2L,
		D3D11_RESOURCE_MISC_TEXTURECUBE	= 0x4L,
		D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS	= 0x10L,
		D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS	= 0x20L,
		D3D11_RESOURCE_MISC_BUFFER_STRUCTURED	= 0x40L,
		D3D11_RESOURCE_MISC_RESOURCE_CLAMP	= 0x80L,
		D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX	= 0x100L,
		D3D11_RESOURCE_MISC_GDI_COMPATIBLE	= 0x200L,
		D3D11_RESOURCE_MISC_SHARED_NTHANDLE	= 0x800L,
		D3D11_RESOURCE_MISC_RESTRICTED_CONTENT	= 0x1000L,
		D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE	= 0x2000L,
		D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER	= 0x4000L,
		D3D11_RESOURCE_MISC_GUARDED	= 0x8000L,
		D3D11_RESOURCE_MISC_TILE_POOL	= 0x20000L,
		D3D11_RESOURCE_MISC_TILED	= 0x40000L,
		D3D11_RESOURCE_MISC_HW_PROTECTED	= 0x80000L
	};

	enum DDS_MISC_FLAGS2 : uint
	{
		DDS_ALPHA_MODE_UNKNOWN	= 0x0,
		DDS_ALPHA_MODE_STRAIGHT = 0x1,
		DDS_ALPHA_MODE_PREMULTIPLIED = 0x2,
		DDS_ALPHA_MODE_OPAQUE = 0x3,
		DDS_ALPHA_MODE_CUSTOM = 0x4,
	};

	struct DDS_HEADER_DXT10
	{
		DXGI_FORMAT               dxgiFormat;
		D3D11_RESOURCE_DIMENSION  resourceDimension;
		D3D11_RESOURCE_MISC_FLAG  miscFlag;
		uint                      arraySize;
		DDS_MISC_FLAGS2           miscFlags2;
	};

	STATIC_ASSERT( sizeof(DDS_PIXELFORMAT)  == 32 );
	STATIC_ASSERT( sizeof(DDS_HEADER)       == 128 );
	STATIC_ASSERT( sizeof(DDS_HEADER_DXT10) == 20 );

	
/*
=================================================
	MakeFourCC
=================================================
*/
	ND_ inline constexpr uint  MakeFourCC (uint8_t a, uint8_t b, uint8_t c, uint8_t d)
	{
		return (uint(a)) | (uint(b) << 8) | (uint(c) << 16) | (uint(d) << 24);
	}
	
/*
=================================================
	DDSFormatToPixelFormat
=================================================
*/
	ND_ inline EPixelFormat  DDSFormatToPixelFormat (DXGI_FORMAT fmt)
	{
		switch ( fmt )
		{
			#define DDS_TO_FG_VISITOR( _dxgi_, _fg_fmt_ )   case _dxgi_ : return _fg_fmt_;
			DXGI_FG_PAIR( DDS_TO_FG_VISITOR )
			#undef  DDS_TO_FG_VISITOR
		}
		return Default;
	}
	
/*
=================================================
	DDSFormatToPixelFormat
=================================================
*/
	ND_ inline DXGI_FORMAT  PixelFormatToDDSFormat (EPixelFormat fmt)
	{
		switch ( fmt )
		{
			#define FG_TO_DDS_VISITOR( _dxgi_, _fg_fmt_ )   case _fg_fmt_ : return _dxgi_;
			DXGI_FG_PAIR( FG_TO_DDS_VISITOR )
			#undef  FG_TO_DDS_VISITOR
		}
		return DXGI_FORMAT_UNKNOWN;
	}


}	// FG

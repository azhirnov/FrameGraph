// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/ResourceEnums.h"
#include "framegraph/Public/EResourceState.h"
#include "framegraph/Public/ShaderEnums.h"
#include "framegraph/Public/VertexEnums.h"

namespace FG
{

/*
=================================================
	EIndex_SizeOf
=================================================
*/
	ND_ inline BytesU  EIndex_SizeOf (EIndex value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value ) {
			case EIndex::UShort :	return SizeOf<uint16_t>;
			case EIndex::UInt :		return SizeOf<uint32_t>;
			case EIndex::Unknown :	break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown index type!" );
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	EShaderStages_FromShader
=================================================
*/
	ND_ inline EShaderStages  EShaderStages_FromShader (EShader value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case EShader::Vertex :			return EShaderStages::Vertex;
			case EShader::TessControl :		return EShaderStages::TessControl;
			case EShader::TessEvaluation :	return EShaderStages::TessEvaluation;
			case EShader::Geometry :		return EShaderStages::Geometry;
			case EShader::Fragment :		return EShaderStages::Fragment;
			case EShader::Compute :			return EShaderStages::Compute;
			case EShader::MeshTask :		return EShaderStages::MeshTask;
			case EShader::Mesh :			return EShaderStages::Mesh;
			case EShader::RayGen :			return EShaderStages::RayGen;
			case EShader::RayAnyHit :		return EShaderStages::RayAnyHit;
			case EShader::RayClosestHit :	return EShaderStages::RayClosestHit;
			case EShader::RayMiss :			return EShaderStages::RayMiss;
			case EShader::RayIntersection :	return EShaderStages::RayIntersection;
			case EShader::RayCallable :		return EShaderStages::RayCallable;
			case EShader::Unknown :
			case EShader::_Count :			break;	// to shutup warnings
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unsupported shader type!" );
	}
//-----------------------------------------------------------------------------
	
	
/*
=================================================
	EVertexType_SizeOf
=================================================
*/
	ND_ inline BytesU  EVertexType_SizeOf (EVertexType type)
	{
		const EVertexType	scalar_type	= (type & EVertexType::_TypeMask);
		const uint			vec_size	= uint(type & EVertexType::_VecMask) >> uint(EVertexType::_VecOffset);

		switch ( scalar_type )
		{
			case EVertexType::_Byte :
			case EVertexType::_UByte :	return SizeOf<uint8_t> * vec_size;
			case EVertexType::_Short :
			case EVertexType::_UShort :	return SizeOf<uint16_t> * vec_size;
			case EVertexType::_Int :
			case EVertexType::_UInt :	return SizeOf<uint32_t> * vec_size;
			case EVertexType::_Long :
			case EVertexType::_ULong :	return SizeOf<uint64_t> * vec_size;

			case EVertexType::_Half :	return SizeOf<uint16_t> * vec_size;
			case EVertexType::_Float :	return SizeOf<uint32_t> * vec_size;
			case EVertexType::_Double :	return SizeOf<uint64_t> * vec_size;
		}
		RETURN_ERR( "not supported" );
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	EShaderDebugMode_FromLang
=================================================
*/
	ND_ inline EShaderDebugMode  EShaderDebugMode_From (EShaderLangFormat value)
	{
		switch ( value & EShaderLangFormat::_ModeMask )
		{
			case EShaderLangFormat::Unknown :				return EShaderDebugMode::None;
			case EShaderLangFormat::EnableDebugTrace :		return EShaderDebugMode::Trace;
			case EShaderLangFormat::EnableProfiling :		return EShaderDebugMode::Profiling;
			//case EShaderLangFormat::EnableDebugAsserts :	return EShaderDebugMode::Asserts;
			//case EShaderLangFormat::EnableDebugView :		return EShaderDebugMode::View;
			//case EShaderLangFormat::EnableProfiling :		return EShaderDebugMode::None;
			//case EShaderLangFormat::EnableInstrCounter :	return EShaderDebugMode::InstructionCounter;
		}
		RETURN_ERR( "unknown mode" );
	}
//-----------------------------------------------------------------------------


/*
=================================================
	EResourceState_IsReadable
=================================================
*/
	ND_ inline constexpr bool EResourceState_IsReadable (EResourceState value)
	{
		return EnumEq( value, EResourceState::_Read );
	}
	
/*
=================================================
	EResourceState_IsWritable
=================================================
*/
	ND_ inline constexpr bool EResourceState_IsWritable (EResourceState value)
	{
		return EnumEq( value, EResourceState::_Write );
	}

/*
=================================================
	EResourceState_FromShaders
=================================================
*/
	ND_ inline EResourceState  EResourceState_FromShaders (EShaderStages values)
	{
		EResourceState	result = Zero;
		
		for (EShaderStages t = EShaderStages(1 << 0); t < EShaderStages::_Last; t = EShaderStages(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ))
				continue;
			
			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EShaderStages::Vertex :			result |= EResourceState::_VertexShader;			break;
				case EShaderStages::TessControl :		result |= EResourceState::_TessControlShader;		break;
				case EShaderStages::TessEvaluation :	result |= EResourceState::_TessEvaluationShader;	break;
				case EShaderStages::Geometry :			result |= EResourceState::_GeometryShader;			break;
				case EShaderStages::Fragment :			result |= EResourceState::_FragmentShader;			break;
				case EShaderStages::Compute :			result |= EResourceState::_ComputeShader;			break;
				case EShaderStages::MeshTask :			result |= EResourceState::_MeshTaskShader;			break;
				case EShaderStages::Mesh :				result |= EResourceState::_MeshShader;				break;
				case EShaderStages::RayGen :
				case EShaderStages::RayAnyHit :
				case EShaderStages::RayClosestHit :
				case EShaderStages::RayMiss :
				case EShaderStages::RayIntersection :
				case EShaderStages::RayCallable :		result |= EResourceState::_RayTracingShader;		break;
				case EShaderStages::_Last :
				case EShaderStages::All :
				case EShaderStages::AllGraphics :
				case EShaderStages::AllRayTracing :
				case EShaderStages::Unknown :
				default :								RETURN_ERR( "unknown shader type" );
			}
			END_ENUM_CHECKS();
		}
		return result;
	}
	
/*
=================================================
	EResourceState_ToShaderAccess
=================================================
*/
	ND_ inline EShaderAccess  EResourceState_ToShaderAccess (EResourceState state)
	{
		if ( EnumEq( state, EResourceState::ShaderReadWrite ))
			return EShaderAccess::ReadWrite;
		
		if ( EnumEq( state, EResourceState::ShaderWrite | EResourceState::InvalidateBefore ))
			return EShaderAccess::WriteDiscard;
		
		if ( EnumEq( state, EResourceState::ShaderWrite ))
			return EShaderAccess::WriteOnly;
		
		if ( EnumEq( state, EResourceState::ShaderRead ))
			return EShaderAccess::ReadOnly;

		RETURN_ERR( "unsupported shader access", EShaderAccess(~0u) );
	}

/*
=================================================
	EResourceState_FromShaderAccess
=================================================
*/
	ND_ inline EResourceState  EResourceState_FromShaderAccess (EShaderAccess access)
	{
		BEGIN_ENUM_CHECKS();
		switch ( access ) {
			case EShaderAccess::ReadOnly :		return EResourceState::ShaderRead;
			case EShaderAccess::WriteOnly :		return EResourceState::ShaderWrite;
			case EShaderAccess::ReadWrite :		return EResourceState::ShaderReadWrite;
			case EShaderAccess::WriteDiscard :	return EResourceState::ShaderWrite | EResourceState::InvalidateBefore;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unsupported shader access" );
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	EImage_IsMultisampled
=================================================
*/
	ND_ inline constexpr bool EImage_IsMultisampled (EImage value)
	{
		return value == EImage::Tex2DMS or value == EImage::Tex2DMSArray;
	}
	
/*
=================================================
	EImage_IsCube
=================================================
*/
	ND_ inline constexpr bool EImage_IsCube (EImage value)
	{
		return value == EImage::TexCube or value == EImage::TexCubeArray;
	}
	
/*
=================================================
	EImage_IsArray
=================================================
*/
	ND_ inline constexpr bool EImage_IsArray (EImage value)
	{
		return	value == EImage::Tex1DArray		or
				value == EImage::Tex2DArray		or
				value == EImage::Tex2DMSArray	or
				value == EImage::TexCubeArray;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	EPixelFormat_GetInfo
=================================================
*/
	struct PixelFormatInfo
	{
		enum class EType
		{
			SFloat		= 1 << 0,
			UFloat		= 1 << 1,
			UNorm		= 1 << 2,
			SNorm		= 1 << 3,
			Int			= 1 << 4,
			UInt		= 1 << 5,
			Depth		= 1 << 6,
			Stencil		= 1 << 7,
			DepthStencil= Depth | Stencil,
			_ValueMask	= 0xFFFF,

			// flags
			sRGB		= 1 << 16,

			Unknown		= 0
		};

		EPixelFormat		format			= Default;
		uint				bitsPerBlock	= 0;		// for color and depth
		uint				bitsPerBlock2	= 0;		// for stencil
		EImageAspect		aspectMask		= Default;
		EType				valueType		= Default;
		uint2				blockSize		= {1,1};

		constexpr PixelFormatInfo () {}

		constexpr PixelFormatInfo (EPixelFormat fmt, uint bpp, EType type, EImageAspect aspect = EImageAspect::Color) :
			format{fmt}, bitsPerBlock{bpp}, aspectMask{aspect}, valueType{type} {}

		constexpr PixelFormatInfo (EPixelFormat fmt, uint bpp, const uint2 &size, EType type, EImageAspect aspect = EImageAspect::Color) :
			format{fmt}, bitsPerBlock{bpp}, aspectMask{aspect}, valueType{type}, blockSize{size} {}

		constexpr PixelFormatInfo (EPixelFormat fmt, uint depthBPP, uint stencilBPP, EType type = EType::DepthStencil, EImageAspect aspect = EImageAspect::DepthStencil) :
			format{fmt}, bitsPerBlock{depthBPP}, bitsPerBlock2{stencilBPP}, aspectMask{aspect}, valueType{type} {}
	};
	FG_BIT_OPERATORS( PixelFormatInfo::EType );


	ND_ inline PixelFormatInfo const&  EPixelFormat_GetInfo (EPixelFormat value)
	{
		using EType = PixelFormatInfo::EType;

		static const PixelFormatInfo	fmt_infos[] = {
			{ EPixelFormat::RGBA16_SNorm,			16*4,			EType::SNorm },
			{ EPixelFormat::RGBA8_SNorm,			8*4,			EType::SNorm },
			{ EPixelFormat::RGB16_SNorm,			16*3,			EType::SNorm },
			{ EPixelFormat::RGB8_SNorm,				8*3,			EType::SNorm },
			{ EPixelFormat::RG16_SNorm,				16*2,			EType::SNorm },
			{ EPixelFormat::RG8_SNorm,				8*2,			EType::SNorm },
			{ EPixelFormat::R16_SNorm,				16*1,			EType::SNorm },
			{ EPixelFormat::R8_SNorm,				8*1,			EType::SNorm },
			{ EPixelFormat::RGBA16_UNorm,			16*4,			EType::UNorm },
			{ EPixelFormat::RGBA8_UNorm,			8*4,			EType::UNorm },
			{ EPixelFormat::RGB16_UNorm,			16*3,			EType::UNorm },
			{ EPixelFormat::RGB8_UNorm,				8*3,			EType::UNorm },
			{ EPixelFormat::RG16_UNorm,				16*2,			EType::UNorm },
			{ EPixelFormat::RG8_UNorm,				8*2,			EType::UNorm },
			{ EPixelFormat::R16_UNorm,				16*1,			EType::UNorm },
			{ EPixelFormat::R8_UNorm,				8*1,			EType::UNorm },
			{ EPixelFormat::RGB10_A2_UNorm,			10*3+2,			EType::UNorm },
			{ EPixelFormat::RGBA4_UNorm,			4*4,			EType::UNorm },
			{ EPixelFormat::RGB5_A1_UNorm,			5*3+1,			EType::UNorm },
			{ EPixelFormat::RGB_5_6_5_UNorm,		5+6+5,			EType::UNorm },
			{ EPixelFormat::BGR8_UNorm,				8*3,			EType::UNorm },
			{ EPixelFormat::BGRA8_UNorm,			8*4,			EType::UNorm },
			{ EPixelFormat::sRGB8,					8*3,			EType::UNorm | EType::sRGB },
			{ EPixelFormat::sRGB8_A8,				8*4,			EType::UNorm | EType::sRGB },
			{ EPixelFormat::sBGR8,					8*3,			EType::UNorm | EType::sRGB },
			{ EPixelFormat::sBGR8_A8,				8*4,			EType::UNorm | EType::sRGB },
			{ EPixelFormat::R8I,					8*1,			EType::Int },
			{ EPixelFormat::RG8I,					8*2,			EType::Int },
			{ EPixelFormat::RGB8I,					8*3,			EType::Int },
			{ EPixelFormat::RGBA8I,					8*4,			EType::Int },
			{ EPixelFormat::R16I,					16*1,			EType::Int },
			{ EPixelFormat::RG16I,					16*2,			EType::Int },
			{ EPixelFormat::RGB16I,					16*3,			EType::Int },
			{ EPixelFormat::RGBA16I,				16*4,			EType::Int },
			{ EPixelFormat::R32I,					32*1,			EType::Int },
			{ EPixelFormat::RG32I,					32*2,			EType::Int },
			{ EPixelFormat::RGB32I,					32*3,			EType::Int },
			{ EPixelFormat::RGBA32I,				32*4,			EType::Int },
			{ EPixelFormat::R8U,					8*1,			EType::UInt },
			{ EPixelFormat::RG8U,					8*2,			EType::UInt },
			{ EPixelFormat::RGB8U,					8*3,			EType::UInt },
			{ EPixelFormat::RGBA8U,					8*4,			EType::UInt },
			{ EPixelFormat::R16U,					16*1,			EType::UInt },
			{ EPixelFormat::RG16U,					16*2,			EType::UInt },
			{ EPixelFormat::RGB16U,					16*3,			EType::UInt },
			{ EPixelFormat::RGBA16U,				16*4,			EType::UInt },
			{ EPixelFormat::R32U,					32*1,			EType::UInt },
			{ EPixelFormat::RG32U,					32*2,			EType::UInt },
			{ EPixelFormat::RGB32U,					32*3,			EType::UInt },
			{ EPixelFormat::RGBA32U,				32*4,			EType::UInt },
			{ EPixelFormat::RGB10_A2U,				10*3+2,			EType::UInt },
			{ EPixelFormat::R16F,					16*1,			EType::SFloat },
			{ EPixelFormat::RG16F,					16*2,			EType::SFloat },
			{ EPixelFormat::RGB16F,					16*3,			EType::SFloat },
			{ EPixelFormat::RGBA16F,				16*4,			EType::SFloat },
			{ EPixelFormat::R32F,					32*1,			EType::SFloat },
			{ EPixelFormat::RG32F,					32*2,			EType::SFloat },
			{ EPixelFormat::RGB32F,					32*3,			EType::SFloat },
			{ EPixelFormat::RGBA32F,				32*4,			EType::SFloat },
			{ EPixelFormat::RGB_11_11_10F,			11+11+10,		EType::SFloat },
			{ EPixelFormat::Depth16,				16,				EType::UNorm | EType::Depth,			EImageAspect::Depth },
			{ EPixelFormat::Depth24,				24,				EType::UNorm | EType::Depth,			EImageAspect::Depth },
			{ EPixelFormat::Depth32F,				32,				EType::SFloat | EType::Depth,			EImageAspect::Depth },
			{ EPixelFormat::Depth16_Stencil8,		16,		8,		EType::UNorm | EType::DepthStencil,		EImageAspect::DepthStencil },
			{ EPixelFormat::Depth24_Stencil8,		24,		8,		EType::UNorm | EType::DepthStencil,		EImageAspect::DepthStencil },
			{ EPixelFormat::Depth32F_Stencil8,		32,		8,		EType::SFloat | EType::DepthStencil,	EImageAspect::DepthStencil },
			{ EPixelFormat::BC1_RGB8_UNorm,			64,		{4,4},	EType::UNorm },
			{ EPixelFormat::BC1_sRGB8_UNorm,		64,		{4,4},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::BC1_RGB8_A1_UNorm,		64,		{4,4},	EType::UNorm },
			{ EPixelFormat::BC1_sRGB8_A1_UNorm,		64,		{4,4},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::BC2_RGBA8_UNorm,		128,	{4,4},	EType::UNorm },
			{ EPixelFormat::BC3_RGBA8_UNorm,		128,	{4,4},	EType::UNorm },
			{ EPixelFormat::BC3_sRGB,				128,	{4,4},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::BC4_RED8_SNorm,			64,		{4,4},	EType::SNorm },
			{ EPixelFormat::BC4_RED8_UNorm,			64,		{4,4},	EType::UNorm },
			{ EPixelFormat::BC5_RG8_SNorm,			128,	{4,4},	EType::SNorm },
			{ EPixelFormat::BC5_RG8_UNorm,			128,	{4,4},	EType::UNorm },
			{ EPixelFormat::BC7_RGBA8_UNorm,		128,	{4,4},	EType::UNorm },
			{ EPixelFormat::BC7_SRGB8_A8,			128,	{4,4},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::BC6H_RGB16F,			128,	{4,4},	EType::SFloat },
			{ EPixelFormat::BC6H_RGB16UF,			128,	{4,4},	EType::UInt },
			{ EPixelFormat::ETC2_RGB8_UNorm,		64,		{4,4},	EType::UNorm },
			{ EPixelFormat::ECT2_SRGB8,				64,		{4,4},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ETC2_RGB8_A1_UNorm,		64,		{4,4},	EType::UNorm },
			{ EPixelFormat::ETC2_SRGB8_A1,			64,		{4,4},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ETC2_RGBA8_UNorm,		128,	{4,4},	EType::UNorm },
			{ EPixelFormat::ETC2_SRGB8_A8,			128,	{4,4},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::EAC_R11_SNorm,			64,		{4,4},	EType::SNorm },
			{ EPixelFormat::EAC_R11_UNorm,			64,		{4,4},	EType::UNorm },
			{ EPixelFormat::EAC_RG11_SNorm,			128,	{4,4},	EType::SNorm },
			{ EPixelFormat::EAC_RG11_UNorm,			128,	{4,4},	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_4x4,			128,	{4,4},	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_5x4,			128,	{5,4},	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_5x5,			128,	{5,5},	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_6x5,			128,	{6,5},	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_6x6,			128,	{6,6},	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_8x5,			128,	{8,5},	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_8x6,			128,	{8,6},	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_8x8,			128,	{8,8},	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_10x5,			128,	{10,5},	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_10x6,			128,	{10,6},	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_10x8,			128,	{10,8},	EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_10x10,		128,	{10,10},EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_12x10,		128,	{12,10},EType::UNorm },
			{ EPixelFormat::ASTC_RGBA_12x12,		128,	{12,12},EType::UNorm },
			{ EPixelFormat::ASTC_SRGB8_A8_4x4,		128,	{4,4},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_5x4,		128,	{5,4},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_5x5,		128,	{5,5},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_6x5,		128,	{6,5},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_6x6,		128,	{6,6},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_8x5,		128,	{8,5},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_8x6,		128,	{8,6},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_8x8,		128,	{8,8},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_10x5,		128,	{10,5},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_10x6,		128,	{10,6},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_10x8,		128,	{10,8},	EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_10x10,	128,	{10,10},EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_12x10,	128,	{12,10},EType::UNorm | EType::sRGB },
			{ EPixelFormat::ASTC_SRGB8_A8_12x12,	128,	{12,12},EType::UNorm | EType::sRGB },
			{ EPixelFormat::Unknown,				0,				EType::Unknown }
		};
		STATIC_ASSERT( CountOf(fmt_infos) == uint(EPixelFormat::_Count)+1 );

		auto&	result = fmt_infos[ value < EPixelFormat::_Count ? uint(value) : uint(EPixelFormat::_Count) ];
		ASSERT( result.format == value );

		return result;
	}
	
/*
=================================================
	EPixelFormat_BitPerPixel
=================================================
*/
	ND_ inline uint  EPixelFormat_BitPerPixel (EPixelFormat value, EImageAspect aspect)
	{
		auto	info = EPixelFormat_GetInfo( value );
		ASSERT( EnumEq( info.aspectMask, aspect ) );

		if ( aspect != EImageAspect::Stencil )
			return info.bitsPerBlock / (info.blockSize.x * info.blockSize.y);
		else
			return info.bitsPerBlock2 / (info.blockSize.x * info.blockSize.y);
	}

/*
=================================================
	EPixelFormat_Is***
=================================================
*/
	ND_ inline bool  EPixelFormat_IsDepth (EPixelFormat value)
	{
		return EPixelFormat_GetInfo( value ).valueType == PixelFormatInfo::EType::Depth;
	}
	
	ND_ inline bool  EPixelFormat_IsStencil (EPixelFormat value)
	{
		return EPixelFormat_GetInfo( value ).valueType == PixelFormatInfo::EType::Stencil;
	}
	
	ND_ inline bool  EPixelFormat_IsDepthStencil (EPixelFormat value)
	{
		return EPixelFormat_GetInfo( value ).valueType == PixelFormatInfo::EType::DepthStencil;
	}
	
	ND_ inline bool  EPixelFormat_IsColor (EPixelFormat value)
	{
		return not EnumAny( EPixelFormat_GetInfo( value ).valueType, PixelFormatInfo::EType::DepthStencil );
	}

/*
=================================================
	EPixelFormat_Has***
=================================================
*/
	ND_ inline bool  EPixelFormat_HasDepth (EPixelFormat value)
	{
		return EnumEq( EPixelFormat_GetInfo( value ).valueType, PixelFormatInfo::EType::Depth );
	}
	
	ND_ inline bool  EPixelFormat_HasStencil (EPixelFormat value)
	{
		return EnumEq( EPixelFormat_GetInfo( value ).valueType, PixelFormatInfo::EType::Stencil );
	}
	
	ND_ inline bool  EPixelFormat_HasDepthOrStencil (EPixelFormat value)
	{
		return EnumAny( EPixelFormat_GetInfo( value ).valueType, PixelFormatInfo::EType::DepthStencil );
	}
	
/*
=================================================
	EPixelFormat_ToImageAspect
=================================================
*/
	ND_ inline EImageAspect  EPixelFormat_ToImageAspect (EPixelFormat format)
	{
		auto&	fmt_info	= EPixelFormat_GetInfo( format );
		auto	depth_bit	= EnumEq( fmt_info.valueType, PixelFormatInfo::EType::Depth ) ? EImageAspect::Depth : Default;
		auto	stencil_bit	= EnumEq( fmt_info.valueType, PixelFormatInfo::EType::Stencil ) ? EImageAspect::Stencil : Default;
		auto	color_bit	= (not (depth_bit | stencil_bit) ? EImageAspect::Color : Default);

		return depth_bit | stencil_bit | color_bit;
	}


}	// FG

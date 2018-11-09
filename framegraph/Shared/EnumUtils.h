// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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
		ENABLE_ENUM_CHECKS();
		switch ( value ) {
			case EIndex::UShort :	return SizeOf<uint16_t>;
			case EIndex::UInt :		return SizeOf<uint32_t>;
			case EIndex::Unknown :	break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown index type!" );
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
		EResourceState	result = EResourceState(0);
		
		for (EShaderStages t = EShaderStages(1 << 0); t < EShaderStages::_Last; t = EShaderStages(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;
			
			ENABLE_ENUM_CHECKS();
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
				case EShaderStages::Unknown :
				default :								RETURN_ERR( "unknown shader type" );
			}
			DISABLE_ENUM_CHECKS();
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
		if ( EnumEq( state, EResourceState::ShaderReadWrite ) )
			return EShaderAccess::ReadWrite;
		
		if ( EnumEq( state, EResourceState::ShaderWrite | EResourceState::InvalidateBefore ) )
			return EShaderAccess::WriteDiscard;
		
		if ( EnumEq( state, EResourceState::ShaderWrite ) )
			return EShaderAccess::WriteOnly;
		
		if ( EnumEq( state, EResourceState::ShaderRead ) )
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
		ENABLE_ENUM_CHECKS();
		switch ( access ) {
			case EShaderAccess::ReadOnly :		return EResourceState::ShaderRead;
			case EShaderAccess::WriteOnly :		return EResourceState::ShaderWrite;
			case EShaderAccess::ReadWrite :		return EResourceState::ShaderReadWrite;
			case EShaderAccess::WriteDiscard :	return EResourceState::ShaderWrite | EResourceState::InvalidateBefore;
		}
		DISABLE_ENUM_CHECKS();
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
	EPixelFormat_IsDepth
=================================================
*/
	ND_ inline constexpr bool EPixelFormat_IsDepth (EPixelFormat value)
	{
		switch ( value ) {
			case EPixelFormat::Depth16 :
			case EPixelFormat::Depth24 :
			//case EPixelFormat::Depth32 :
			case EPixelFormat::Depth32F :	return true;
		}
		return false;
	}
	
/*
=================================================
	EPixelFormat_IsStencil
=================================================
*/
	ND_ inline constexpr bool EPixelFormat_IsStencil (EPixelFormat)
	{
		// TODO
		return false;
	}
	
/*
=================================================
	EPixelFormat_IsDepthStencil
=================================================
*/
	ND_ inline constexpr bool EPixelFormat_IsDepthStencil (EPixelFormat value)
	{
		switch ( value ) {
			case EPixelFormat::Depth16_Stencil8 :
			case EPixelFormat::Depth24_Stencil8 :
			case EPixelFormat::Depth32F_Stencil8 :	return true;
		}
		return false;
	}
	
/*
=================================================
	EPixelFormat_IsColor
=================================================
*/
	ND_ inline constexpr bool EPixelFormat_IsColor (EPixelFormat value)
	{
		return not (EPixelFormat_IsDepth( value )		or
					EPixelFormat_IsStencil( value )		or
					EPixelFormat_IsDepthStencil( value ));
	}

/*
=================================================
	EPixelFormat_HasDepth
=================================================
*/
	ND_ inline constexpr bool EPixelFormat_HasDepth (EPixelFormat value)
	{
		return EPixelFormat_IsDepth( value ) or EPixelFormat_IsDepthStencil( value );
	}
	
/*
=================================================
	EPixelFormat_HasStencil
=================================================
*/
	ND_ inline constexpr bool EPixelFormat_HasStencil (EPixelFormat value)
	{
		return EPixelFormat_IsStencil( value ) or EPixelFormat_IsDepthStencil( value );
	}
	
/*
=================================================
	EPixelFormat_HasDepthOrStencil
=================================================
*/
	ND_ inline constexpr bool EPixelFormat_HasDepthOrStencil (EPixelFormat value)
	{
		return EPixelFormat_IsDepth( value ) or EPixelFormat_IsStencil( value ) or EPixelFormat_IsDepthStencil( value );
	}

/*
=================================================
	EPixelFormat_BitPerPixel
=================================================
*/
    ND_ inline uint EPixelFormat_BitPerPixel (EPixelFormat value, EImageAspect aspect)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
		// uncompressed color formats:
			case EPixelFormat::R8_SNorm :
			case EPixelFormat::R8_UNorm :
			case EPixelFormat::R8I :
			case EPixelFormat::R8U :
				ASSERT( aspect == EImageAspect::Color );
				return 8;

			case EPixelFormat::R16_SNorm :
			case EPixelFormat::R16_UNorm :
			case EPixelFormat::R16I :
			case EPixelFormat::R16U :
			case EPixelFormat::R16F :
			case EPixelFormat::RG8_SNorm :
			case EPixelFormat::RG8_UNorm :
			case EPixelFormat::RG8I :
			case EPixelFormat::RG8U :
			case EPixelFormat::RGBA4_UNorm :
			case EPixelFormat::RGB5_A1_UNorm :
			case EPixelFormat::RGB_5_6_5_UNorm :
				ASSERT( aspect == EImageAspect::Color );
				return 16;

			case EPixelFormat::RGB8_SNorm :
			case EPixelFormat::RGB8_UNorm :
			case EPixelFormat::BGR8_UNorm :
			case EPixelFormat::sRGB8 :
			case EPixelFormat::RGB8I :
			case EPixelFormat::RGB8U :
				ASSERT( aspect == EImageAspect::Color );
				return 8*3;

			case EPixelFormat::RG16_SNorm :
			case EPixelFormat::RG16_UNorm :
			case EPixelFormat::RG16I :
			case EPixelFormat::RG16U :
			case EPixelFormat::RG16F :
			case EPixelFormat::RGBA8_SNorm :
			case EPixelFormat::RGBA8_UNorm :
			case EPixelFormat::BGRA8_UNorm :
			case EPixelFormat::sRGB8_A8 :
			case EPixelFormat::RGBA8I :
			case EPixelFormat::RGBA8U :
			case EPixelFormat::R32I :
			case EPixelFormat::R32U :
			case EPixelFormat::R32F :
			case EPixelFormat::RGB10_A2_UNorm :
			case EPixelFormat::RGB10_A2U :
			case EPixelFormat::RGB_11_11_10F :
				ASSERT( aspect == EImageAspect::Color );
				return 32;

			case EPixelFormat::RGB16_SNorm :
			case EPixelFormat::RGB16_UNorm :
			case EPixelFormat::RGB16I :
			case EPixelFormat::RGB16U :
			case EPixelFormat::RGB16F :
				ASSERT( aspect == EImageAspect::Color );
				return 16*3;

			case EPixelFormat::RGBA16_SNorm :
			case EPixelFormat::RGBA16_UNorm :
			case EPixelFormat::RGBA16I :
			case EPixelFormat::RGBA16U :
			case EPixelFormat::RGBA16F :
			case EPixelFormat::RG32I :
			case EPixelFormat::RG32U :
			case EPixelFormat::RG32F :
				ASSERT( aspect == EImageAspect::Color );
				return 16*4;

			case EPixelFormat::RGB32I :
			case EPixelFormat::RGB32U :
			case EPixelFormat::RGB32F :
				ASSERT( aspect == EImageAspect::Color );
				return 32*3;

			case EPixelFormat::RGBA32I :
			case EPixelFormat::RGBA32U :
			case EPixelFormat::RGBA32F :
				ASSERT( aspect == EImageAspect::Color );
				return 32*4;

		// uncompressed depth/stencil formats:
			case EPixelFormat::Depth16 :
				ASSERT( aspect == EImageAspect::Depth );
				return 16;

			case EPixelFormat::Depth24 :
			case EPixelFormat::Depth32F :
				ASSERT( aspect == EImageAspect::Depth );
				return 32;

			case EPixelFormat::Depth16_Stencil8	:
				ASSERT( aspect == EImageAspect::Depth or aspect == EImageAspect::Stencil );
				return aspect == EImageAspect::Stencil ? 8 : 16;

			case EPixelFormat::Depth24_Stencil8 :
			case EPixelFormat::Depth32F_Stencil8 :
				ASSERT( aspect == EImageAspect::Depth or aspect == EImageAspect::Stencil );
				return aspect == EImageAspect::Stencil ? 8 : 32;

		// compressed formats:
			case EPixelFormat::BC1_RGB8_UNorm :
			case EPixelFormat::BC1_RGB8_A1_UNorm :
				ASSERT( aspect == EImageAspect::Color );
				return 64 / (4*4);

			case EPixelFormat::BC2_RGBA8_UNorm :
			case EPixelFormat::BC3_RGBA8_UNorm :
			case EPixelFormat::BC4_RED8_SNorm :
			case EPixelFormat::BC4_RED8_UNorm :
			case EPixelFormat::BC5_RG8_SNorm :
			case EPixelFormat::BC5_RG8_UNorm :
			case EPixelFormat::BC7_RGBA8_UNorm :
			case EPixelFormat::BC7_SRGB8_A8_UNorm :
			case EPixelFormat::BC6H_RGB16F :
			case EPixelFormat::BC6H_RGB16F_Unsigned :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (4*4);

			case EPixelFormat::ETC2_RGB8_UNorm :
			case EPixelFormat::ECT2_SRGB8_UNorm :
			case EPixelFormat::ETC2_RGB8_A1_UNorm :
			case EPixelFormat::ETC2_SRGB8_A1_UNorm :
			case EPixelFormat::ETC2_RGBA8_UNorm :
			case EPixelFormat::ETC2_SRGB8_A8_UNorm :
				ASSERT( aspect == EImageAspect::Color );
				return 64 / (4*4);

			case EPixelFormat::EAC_R11_SNorm :
			case EPixelFormat::EAC_R11_UNorm :
				ASSERT( aspect == EImageAspect::Color );
				return 64 / (4*4);

			case EPixelFormat::EAC_RG11_SNorm :
			case EPixelFormat::EAC_RG11_UNorm :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (4*4);

			case EPixelFormat::ASTC_RGBA_4x4 :
			case EPixelFormat::ASTC_SRGB8_A8_4x4 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (4*4);

			case EPixelFormat::ASTC_RGBA_5x4 :
			case EPixelFormat::ASTC_SRGB8_A8_5x4 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (5*4);

			case EPixelFormat::ASTC_RGBA_5x5 :
			case EPixelFormat::ASTC_SRGB8_A8_5x5 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (5*5);

			case EPixelFormat::ASTC_RGBA_6x5 :
			case EPixelFormat::ASTC_SRGB8_A8_6x5 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (6*5);

			case EPixelFormat::ASTC_RGBA_6x6 :
			case EPixelFormat::ASTC_SRGB8_A8_6x6 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (6*6);

			case EPixelFormat::ASTC_RGBA_8x5 :
			case EPixelFormat::ASTC_SRGB8_A8_8x5 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (8*5);

			case EPixelFormat::ASTC_RGBA_8x6 :
			case EPixelFormat::ASTC_SRGB8_A8_8x6 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (8*6);

			case EPixelFormat::ASTC_RGBA_8x8 :
			case EPixelFormat::ASTC_SRGB8_A8_8x8 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (8*8);

			case EPixelFormat::ASTC_RGBA_10x5 :
			case EPixelFormat::ASTC_SRGB8_A8_10x5 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (10*5);

			case EPixelFormat::ASTC_RGBA_10x6 :
			case EPixelFormat::ASTC_SRGB8_A8_10x6 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (10*6);

			case EPixelFormat::ASTC_RGBA_10x8 :
			case EPixelFormat::ASTC_SRGB8_A8_10x8 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (10*8);

			case EPixelFormat::ASTC_RGBA_10x10 :
			case EPixelFormat::ASTC_SRGB8_A8_10x10 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (10*10);

			case EPixelFormat::ASTC_RGBA_12x10 :
			case EPixelFormat::ASTC_SRGB8_A8_12x10 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (12*10);

			case EPixelFormat::ASTC_RGBA_12x12 :
			case EPixelFormat::ASTC_SRGB8_A8_12x12 :
				ASSERT( aspect == EImageAspect::Color );
				return 128 / (12*12);

			case EPixelFormat::Unknown :
				break;	// to shutup warnings
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown pixel format" );
	}


}	// FG

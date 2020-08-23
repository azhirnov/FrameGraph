// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/ResourceEnums.h"
#include "framegraph/Public/EResourceState.h"
#include "framegraph/Public/ShaderEnums.h"
#include "framegraph/Public/VertexEnums.h"
#include "stl/Algorithms/StringUtils.h"

namespace FGC
{
	using EImage			= FG::EImage;
	using EImageUsage		= FG::EImageUsage;
	using EBufferUsage		= FG::EBufferUsage;
	using EImageAspect		= FG::EImageAspect;
	using EResourceState	= FG::EResourceState;
	using EPixelFormat		= FG::EPixelFormat;


/*
=================================================
	ToString (EImage)
=================================================
*/
	ND_ inline String  ToString (const EImage value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case EImage::Tex1D :		return "Image1D";
			case EImage::Tex1DArray :	return "Image1D_Array";
			case EImage::Tex2D :		return "Image2D";
			case EImage::Tex2DArray :	return "Image2D_Array";
			case EImage::Tex2DMS :		return "Image2D_MS";
			case EImage::Tex2DMSArray :	return "Image2D_MS_Array";
			case EImage::TexCube :		return "ImageCube";
			case EImage::TexCubeArray :	return "ImageCube_Array";
			case EImage::Tex3D :		return "Image3D";
			case EImage::Unknown :		break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown image type!" );
	}

/*
=================================================
	ToString (EImageUsage)
=================================================
*/
	ND_ inline String  ToString (const EImageUsage values)
	{
		String	result;

		for (EImageUsage t = EImageUsage(1 << 0); t < EImageUsage::_Last; t = EImageUsage(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;

			if ( not result.empty() )
				result << " | ";
			
			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EImageUsage::TransferSrc				: result << "TransferSrc";				break;
				case EImageUsage::TransferDst				: result << "TransferDst";				break;
				case EImageUsage::Sampled					: result << "Sampled";					break;
				case EImageUsage::Storage					: result << "Storage";					break;
				case EImageUsage::ColorAttachment			: result << "ColorAttachment";			break;
				case EImageUsage::DepthStencilAttachment	: result << "DepthStencilAttachment";	break;
				case EImageUsage::TransientAttachment		: result << "TransientAttachment";		break;
				case EImageUsage::InputAttachment			: result << "InputAttachment";			break;
				case EImageUsage::ShadingRate				: result << "ShadingRate";				break;
				case EImageUsage::StorageAtomic				: result << "StorageAtomic";			break;
				case EImageUsage::ColorAttachmentBlend		: result << "ColorAttachmentBlend";		break;
				case EImageUsage::_Last						:
				case EImageUsage::All						:	// to shutup warnings
				case EImageUsage::Transfer					:
				case EImageUsage::Unknown					:
				default										: RETURN_ERR( "invalid image usage type!" );
			}
			END_ENUM_CHECKS();
		}
		return result;
	}

/*
=================================================
	ToString (EBufferUsage)
=================================================
*/
	ND_ inline String  ToString (const EBufferUsage values)
	{
		String	result;

		for (EBufferUsage t = EBufferUsage(1 << 0); t < EBufferUsage::_Last; t = EBufferUsage(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;

			if ( not result.empty() )
				result << " | ";
			
			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EBufferUsage::TransferSrc		:  result << "TransferSrc";			break;
				case EBufferUsage::TransferDst		:  result << "TransferDst";			break;
				case EBufferUsage::UniformTexel		:  result << "UniformTexel";		break;
				case EBufferUsage::StorageTexel		:  result << "StorageTexel";		break;
				case EBufferUsage::Uniform			:  result << "Uniform";				break;
				case EBufferUsage::Storage			:  result << "Storage";				break;
				case EBufferUsage::Index			:  result << "Index";				break;
				case EBufferUsage::Vertex			:  result << "Vertex";				break;
				case EBufferUsage::Indirect			:  result << "Indirect";			break;
				case EBufferUsage::RayTracing		:  result << "RayTracing";			break;
				case EBufferUsage::VertexPplnStore	:  result << "VertexPplnStore";		break;
				case EBufferUsage::FragmentPplnStore:  result << "FragmentPplnStore";	break;
				case EBufferUsage::StorageTexelAtomic: result << "StorageTexelAtomic";	break;
				case EBufferUsage::_Last			:
				case EBufferUsage::All				:	// to shutup warnings
				case EBufferUsage::Transfer			:
				case EBufferUsage::Unknown			:
				default								: RETURN_ERR( "invalid buffer usage type!" );
			}
			END_ENUM_CHECKS();
		}
		return result;
	}

/*
=================================================
	ToString (EImageAspect)
=================================================
*/
	ND_ inline String  ToString (const EImageAspect values)
	{
		switch ( values ) {
			case EImageAspect::Auto :			return "Auto";
			case EImageAspect::Unknown :		return "None";
			case EImageAspect::DepthStencil :	return "DepthStencil";
		}

		String	result;
		for (EImageAspect t = EImageAspect(1 << 0); t < EImageAspect::_Last; t = EImageAspect(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;

			if ( not result.empty() )
				result << " | ";
			
			BEGIN_ENUM_CHECKS();
			switch ( t )
			{
				case EImageAspect::Color		: result << "Color";	break;
				case EImageAspect::Depth		: result << "Depth";	break;
				case EImageAspect::Stencil		: result << "Stencil";	break;
				case EImageAspect::Metadata		: result << "Metadata";	break;
				case EImageAspect::_Last		:
				case EImageAspect::DepthStencil :	// to shutup warnings
				case EImageAspect::Auto			:
				case EImageAspect::Unknown		:
				default							: RETURN_ERR( "invalid image aspect type!" );
			}
			END_ENUM_CHECKS();
		}
		return result;
	}
	
/*
=================================================
	ToString (EResourceState)
=================================================
*/
	ND_ inline String  ToString (const EResourceState value)
	{
		const auto	mask = EResourceState::_StateMask;

		String	str;
		switch ( value & EResourceState::_StateMask )
		{
			case EResourceState::Unknown :								str << "Unknown";					break;
			case EResourceState::ShaderRead :							str << "Storage-R";					break;
			case EResourceState::ShaderWrite :							str << "Storage-W";					break;
			case EResourceState::ShaderReadWrite :						str << "Storage-RW";				break;
			case EResourceState::UniformRead :							str << "Uniform";					break;
			case EResourceState::ShaderSample :							str << "ShaderSample";				break;
			case EResourceState::InputAttachment :						str << "SubpassInput";				break;
			case EResourceState::TransferSrc :							str << "Transfer-R";				break;
			case EResourceState::TransferDst :							str << "Transfer-W";				break;
			case EResourceState::ColorAttachmentRead :					str << "Color-R";					break;
			case EResourceState::ColorAttachmentWrite :					str << "Color-W";					break;
			case EResourceState::ColorAttachmentReadWrite :				str << "Color-RW";					break;
			case EResourceState::DepthStencilAttachmentRead :			str << "DepthStencil-R";			break;
			case EResourceState::DepthStencilAttachmentWrite :			str << "DepthStencil-W";			break;
			case EResourceState::DepthStencilAttachmentReadWrite :		str << "DepthStencil-RW";			break;
			case EResourceState::HostRead :								str << "Host-R";					break;
			case EResourceState::HostWrite :							str << "Host-W";					break;
			case EResourceState::HostReadWrite :						str << "Host-RW";					break;
			case EResourceState::PresentImage :							str << "PresentImage";				break;
			case EResourceState::IndirectBuffer :						str << "IndirectBuffer";			break;
			case EResourceState::IndexBuffer :							str << "IndexBuffer";				break;
			case EResourceState::VertexBuffer :							str << "VertexBuffer";				break;
			case EResourceState::BuildRayTracingStructWrite :			str << "BuildRTAS-W";				break;
			case EResourceState::BuildRayTracingStructReadWrite :		str << "BuildRTAS-RW";				break;
			case EResourceState::RTASBuildingBufferRead :				str << "RTASBuild-Buffer-R";		break;
			case EResourceState::RTASBuildingBufferReadWrite :			str << "RTASBuild-Buffer-RW";		break;
			case EResourceState::ShadingRateImageRead :					str << "ShadingRate";				break;
			default :													RETURN_ERR( "unknown resource state!" );
		}

		if ( EnumEq( value, EResourceState::_VertexShader ))			str << ", VS";
		if ( EnumEq( value, EResourceState::_TessControlShader ))		str << ", TCS";
		if ( EnumEq( value, EResourceState::_TessEvaluationShader ))	str << ", TES";
		if ( EnumEq( value, EResourceState::_GeometryShader ))			str << ", GS";
		if ( EnumEq( value, EResourceState::_FragmentShader ))			str << ", FS";
		if ( EnumEq( value, EResourceState::_ComputeShader ))			str << ", CS";
		if ( EnumEq( value, EResourceState::_MeshTaskShader ))			str << ", MTS";
		if ( EnumEq( value, EResourceState::_MeshShader ))				str << ", MS";
		if ( EnumEq( value, EResourceState::_RayTracingShader ))		str << ", RTS";
		
		if ( EnumEq( value, EResourceState::InvalidateBefore ))			str << ", InvalidateBefore";
		if ( EnumEq( value, EResourceState::InvalidateAfter ))			str << ", InvalidateAfter";
		if ( EnumEq( value, EResourceState::_BufferDynamicOffset ))		str << ", Dynamic";

		if ( not EnumEq( value, EResourceState::EarlyFragmentTests | EResourceState::LateFragmentTests )) {
			if ( EnumEq( value, EResourceState::EarlyFragmentTests ) )	str << ", EarlyTests";
			if ( EnumEq( value, EResourceState::LateFragmentTests ) )	str << ", LateTests";
		}
		return str;
	}
	
/*
=================================================
	ToString (EPixelFormat)
=================================================
*/
	ND_ inline String  ToString (const EPixelFormat value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case EPixelFormat::RGBA16_SNorm : return "RGBA16_SNorm";
			case EPixelFormat::RGBA8_SNorm : return "RGBA8_SNorm";
			case EPixelFormat::RGB16_SNorm : return "RGB16_SNorm";
			case EPixelFormat::RGB8_SNorm : return "RGB8_SNorm";
			case EPixelFormat::RG16_SNorm : return "RG16_SNorm";
			case EPixelFormat::RG8_SNorm : return "RG8_SNorm";
			case EPixelFormat::R16_SNorm : return "R16_SNorm";
			case EPixelFormat::R8_SNorm : return "R8_SNorm";
			case EPixelFormat::RGBA16_UNorm : return "RGBA16_UNorm";
			case EPixelFormat::RGBA8_UNorm : return "RGBA8_UNorm";
			case EPixelFormat::RGB16_UNorm : return "RGB16_UNorm";
			case EPixelFormat::RGB8_UNorm : return "RGB8_UNorm";
			case EPixelFormat::RG16_UNorm : return "RG16_UNorm";
			case EPixelFormat::RG8_UNorm : return "RG8_UNorm";
			case EPixelFormat::R16_UNorm : return "R16_UNorm";
			case EPixelFormat::R8_UNorm : return "R8_UNorm";
			case EPixelFormat::RGB10_A2_UNorm : return "RGB10_A2_UNorm";
			case EPixelFormat::RGBA4_UNorm : return "RGBA4_UNorm";
			case EPixelFormat::RGB5_A1_UNorm : return "RGB5_A1_UNorm";
			case EPixelFormat::RGB_5_6_5_UNorm : return "RGB_5_6_5_UNorm";
			case EPixelFormat::BGR8_UNorm : return "BGR8_UNorm";
			case EPixelFormat::BGRA8_UNorm : return "BGRA8_UNorm";
			case EPixelFormat::sRGB8 : return "sRGB8";
			case EPixelFormat::sRGB8_A8 : return "sRGB8_A8";
			case EPixelFormat::sBGR8 : return "sBGR8";
			case EPixelFormat::sBGR8_A8 : return "sBGR8_A8";
			case EPixelFormat::R8I : return "R8I";
			case EPixelFormat::RG8I : return "RG8I";
			case EPixelFormat::RGB8I : return "RGB8I";
			case EPixelFormat::RGBA8I : return "RGBA8I";
			case EPixelFormat::R16I : return "R16I";
			case EPixelFormat::RG16I : return "RG16I";
			case EPixelFormat::RGB16I : return "RGB16I";
			case EPixelFormat::RGBA16I : return "RGBA16I";
			case EPixelFormat::R32I : return "R32I";
			case EPixelFormat::RG32I : return "RG32I";
			case EPixelFormat::RGB32I : return "RGB32I";
			case EPixelFormat::RGBA32I : return "RGBA32I";
			case EPixelFormat::R8U : return "R8U";
			case EPixelFormat::RG8U : return "RG8U";
			case EPixelFormat::RGB8U : return "RGB8U";
			case EPixelFormat::RGBA8U : return "RGBA8U";
			case EPixelFormat::R16U : return "R16U";
			case EPixelFormat::RG16U : return "RG16U";
			case EPixelFormat::RGB16U : return "RGB16U";
			case EPixelFormat::RGBA16U : return "RGBA16U";
			case EPixelFormat::R32U : return "R32U";
			case EPixelFormat::RG32U : return "RG32U";
			case EPixelFormat::RGB32U : return "RGB32U";
			case EPixelFormat::RGBA32U : return "RGBA32U";
			case EPixelFormat::RGB10_A2U : return "RGB10_A2U";
			case EPixelFormat::R16F : return "R16F";
			case EPixelFormat::RG16F : return "RG16F";
			case EPixelFormat::RGB16F : return "RGB16F";
			case EPixelFormat::RGBA16F : return "RGBA16F";
			case EPixelFormat::R32F : return "R32F";
			case EPixelFormat::RG32F : return "RG32F";
			case EPixelFormat::RGB32F : return "RGB32F";
			case EPixelFormat::RGBA32F : return "RGBA32F";
			case EPixelFormat::RGB_11_11_10F : return "RGB_11_11_10F";
			case EPixelFormat::Depth16 : return "Depth16";
			case EPixelFormat::Depth24 : return "Depth24";
			case EPixelFormat::Depth32F : return "Depth32F";
			case EPixelFormat::Depth16_Stencil8 : return "Depth16_Stencil8";
			case EPixelFormat::Depth24_Stencil8 : return "Depth24_Stencil8";
			case EPixelFormat::Depth32F_Stencil8 : return "Depth32F_Stencil8";
			case EPixelFormat::BC1_RGB8_UNorm : return "BC1_RGB8_UNorm";
			case EPixelFormat::BC1_sRGB8 : return "BC1_sRGB8";
			case EPixelFormat::BC1_RGB8_A1_UNorm : return "BC1_RGB8_A1_UNorm";
			case EPixelFormat::BC1_sRGB8_A1 : return "BC1_sRGB8_A1";
			case EPixelFormat::BC2_RGBA8_UNorm : return "BC2_RGBA8_UNorm";
			case EPixelFormat::BC2_sRGB8_A8 : return "BC2_sRGB8_A8";
			case EPixelFormat::BC3_RGBA8_UNorm : return "BC3_RGBA8_UNorm";
			case EPixelFormat::BC3_sRGB8 : return "BC3_sRGB8";
			case EPixelFormat::BC4_R8_SNorm : return "BC4_R8_SNorm";
			case EPixelFormat::BC4_R8_UNorm : return "BC4_R8_UNorm";
			case EPixelFormat::BC5_RG8_SNorm : return "BC5_RG8_SNorm";
			case EPixelFormat::BC5_RG8_UNorm : return "BC5_RG8_UNorm";
			case EPixelFormat::BC7_RGBA8_UNorm : return "BC7_RGBA8_UNorm";
			case EPixelFormat::BC7_sRGB8_A8 : return "BC7_sRGB8_A8";
			case EPixelFormat::BC6H_RGB16F : return "BC6H_RGB16F";
			case EPixelFormat::BC6H_RGB16UF : return "BC6H_RGB16UF";
			case EPixelFormat::ETC2_RGB8_UNorm : return "ETC2_RGB8_UNorm";
			case EPixelFormat::ECT2_sRGB8 : return "ECT2_sRGB8";
			case EPixelFormat::ETC2_RGB8_A1_UNorm : return "ETC2_RGB8_A1_UNorm";
			case EPixelFormat::ETC2_sRGB8_A1 : return "ETC2_sRGB8_A1";
			case EPixelFormat::ETC2_RGBA8_UNorm : return "ETC2_RGBA8_UNorm";
			case EPixelFormat::ETC2_sRGB8_A8 : return "ETC2_sRGB8_A8";
			case EPixelFormat::EAC_R11_SNorm : return "EAC_R11_SNorm";
			case EPixelFormat::EAC_R11_UNorm : return "EAC_R11_UNorm";
			case EPixelFormat::EAC_RG11_SNorm : return "EAC_RG11_SNorm";
			case EPixelFormat::EAC_RG11_UNorm : return "EAC_RG11_UNorm";
			case EPixelFormat::ASTC_RGBA_4x4 : return "ASTC_RGBA_4x4";
			case EPixelFormat::ASTC_RGBA_5x4 : return "ASTC_RGBA_5x4";
			case EPixelFormat::ASTC_RGBA_5x5 : return "ASTC_RGBA_5x5";
			case EPixelFormat::ASTC_RGBA_6x5 : return "ASTC_RGBA_6x5";
			case EPixelFormat::ASTC_RGBA_6x6 : return "ASTC_RGBA_6x6";
			case EPixelFormat::ASTC_RGBA_8x5 : return "ASTC_RGBA_8x5";
			case EPixelFormat::ASTC_RGBA_8x6 : return "ASTC_RGBA_8x6";
			case EPixelFormat::ASTC_RGBA_8x8 : return "ASTC_RGBA_8x8";
			case EPixelFormat::ASTC_RGBA_10x5 : return "ASTC_RGBA_10x5";
			case EPixelFormat::ASTC_RGBA_10x6 : return "ASTC_RGBA_10x6";
			case EPixelFormat::ASTC_RGBA_10x8 : return "ASTC_RGBA_10x8";
			case EPixelFormat::ASTC_RGBA_10x10 : return "ASTC_RGBA_10x10";
			case EPixelFormat::ASTC_RGBA_12x10 : return "ASTC_RGBA_12x10";
			case EPixelFormat::ASTC_RGBA_12x12 : return "ASTC_RGBA_12x12";
			case EPixelFormat::ASTC_sRGB8_A8_4x4 : return "ASTC_sRGB8_A8_4x4";
			case EPixelFormat::ASTC_sRGB8_A8_5x4 : return "ASTC_sRGB8_A8_5x4";
			case EPixelFormat::ASTC_sRGB8_A8_5x5 : return "ASTC_sRGB8_A8_5x5";
			case EPixelFormat::ASTC_sRGB8_A8_6x5 : return "ASTC_sRGB8_A8_6x5";
			case EPixelFormat::ASTC_sRGB8_A8_6x6 : return "ASTC_sRGB8_A8_6x6";
			case EPixelFormat::ASTC_sRGB8_A8_8x5 : return "ASTC_sRGB8_A8_8x5";
			case EPixelFormat::ASTC_sRGB8_A8_8x6 : return "ASTC_sRGB8_A8_8x6";
			case EPixelFormat::ASTC_sRGB8_A8_8x8 : return "ASTC_sRGB8_A8_8x8";
			case EPixelFormat::ASTC_sRGB8_A8_10x5 : return "ASTC_sRGB8_A8_10x5";
			case EPixelFormat::ASTC_sRGB8_A8_10x6 : return "ASTC_sRGB8_A8_10x6";
			case EPixelFormat::ASTC_sRGB8_A8_10x8 : return "ASTC_sRGB8_A8_10x8";
			case EPixelFormat::ASTC_sRGB8_A8_10x10 : return "ASTC_sRGB8_A8_10x10";
			case EPixelFormat::ASTC_sRGB8_A8_12x10 : return "ASTC_sRGB8_A8_12x10";
			case EPixelFormat::ASTC_sRGB8_A8_12x12 : return "ASTC_sRGB8_A8_12x12";
			case EPixelFormat::_Count :
			case EPixelFormat::Unknown : break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown pixel format type!" );
	}

}	// FGC

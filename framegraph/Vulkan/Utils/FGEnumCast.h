// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VEnumCast.h"	// for FG_PRIVATE_VKPIXELFORMATS

namespace FG
{

/*
=================================================
	FGEnumCast (VkFormat)
=================================================
*/
	ND_ inline EPixelFormat  FGEnumCast (VkFormat value)
	{
#		define FMT_BUILDER( _engineFmt_, _vkFormat_ ) \
			case _vkFormat_ : return EPixelFormat::_engineFmt_;
		
		switch ( value )
		{
			FG_PRIVATE_VKPIXELFORMATS( FMT_BUILDER )
		}

#		undef FMT_BUILDER

		RETURN_ERR( "invalid pixel format" );
	}
	
/*
=================================================
	FGEnumCast (VkBufferUsageFlagBits)
=================================================
*/
	ND_ inline EBufferUsage  FGEnumCast (VkBufferUsageFlagBits flags)
	{
		EBufferUsage	result = Default;

		for (VkBufferUsageFlags t = 1; t < VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM; t <<= 1)
		{
			if ( not EnumEq( flags, t ) )
				continue;
			
			ENABLE_ENUM_CHECKS();
			switch ( VkBufferUsageFlagBits(t) )
			{
				case VK_BUFFER_USAGE_TRANSFER_SRC_BIT :			result |= EBufferUsage::TransferSrc;	break;
				case VK_BUFFER_USAGE_TRANSFER_DST_BIT :			result |= EBufferUsage::TransferDst;	break;
				case VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT :	result |= EBufferUsage::UniformTexel;	break;
				case VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT :	result |= EBufferUsage::StorageTexel;	break;
				case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT :		result |= EBufferUsage::Uniform;		break;
				case VK_BUFFER_USAGE_STORAGE_BUFFER_BIT :		result |= EBufferUsage::Storage;		break;
				case VK_BUFFER_USAGE_INDEX_BUFFER_BIT :			result |= EBufferUsage::Index;			break;
				case VK_BUFFER_USAGE_VERTEX_BUFFER_BIT :		result |= EBufferUsage::Vertex;			break;
				case VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT :		result |= EBufferUsage::Indirect;		break;
				case VK_BUFFER_USAGE_RAY_TRACING_BIT_NV :		result |= EBufferUsage::RayTracing;		break;
				case VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT :
				case VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT :	// TODO
				case VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT :
				case VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT :
				case VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM :
				default :										RETURN_ERR( "invalid buffer usage" );
			}
			DISABLE_ENUM_CHECKS();
		}
		return result;
	}
	

/*
=================================================
	FGEnumCast (VkImageType)
=================================================
*/
	ND_ inline EImage  FGEnumCast (VkImageType type, VkImageCreateFlags flags, uint arrayLayers, VkSampleCountFlagBits samples)
	{
		const bool	is_array		= arrayLayers > 1;
		const bool	is_multisampled	= samples > VK_SAMPLE_COUNT_1_BIT;
		const bool	is_cube			= is_array and EnumEq( flags, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT );

		ENABLE_ENUM_CHECKS();
		switch ( type )
		{
			case VK_IMAGE_TYPE_1D :
				return is_array ? EImage::Tex1DArray : EImage::Tex1D;

			case VK_IMAGE_TYPE_2D :
				if ( is_multisampled )	return is_array ? EImage::Tex2DMSArray : EImage::Tex2DMS;
				if ( is_cube )			return arrayLayers == 6 ? EImage::TexCube : EImage::TexCubeArray;
				return is_array ? EImage::Tex2DArray : EImage::Tex2D;

			case VK_IMAGE_TYPE_3D :
				ASSERT( arrayLayers == 1 );
				return EImage::Tex3D;

			case VK_IMAGE_TYPE_RANGE_SIZE :
			case VK_IMAGE_TYPE_MAX_ENUM :	break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "not supported" );
	}

/*
=================================================
	FGEnumCast (VkImageUsageFlagBits)
=================================================
*/
	ND_ inline EImageUsage  FGEnumCast (VkImageUsageFlagBits usage)
	{
		EImageUsage		result = Default;

		for (VkImageUsageFlags t = 1; t < VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM; t <<= 1)
		{
			if ( not EnumEq( usage, t ) )
				continue;
			
			ENABLE_ENUM_CHECKS();
			switch ( VkImageUsageFlagBits(t) )
			{
				case VK_IMAGE_USAGE_TRANSFER_SRC_BIT :				result |= EImageUsage::TransferSrc;				break;
				case VK_IMAGE_USAGE_TRANSFER_DST_BIT :				result |= EImageUsage::TransferDst;				break;
				case VK_IMAGE_USAGE_SAMPLED_BIT :					result |= EImageUsage::Sampled;					break;
				case VK_IMAGE_USAGE_STORAGE_BIT :					result |= EImageUsage::Storage;					break;
				case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT :			result |= EImageUsage::ColorAttachment;			break;
				case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :	result |= EImageUsage::DepthStencilAttachment;	break;
				case VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT :		result |= EImageUsage::TransientAttachment;		break;
				case VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT :			result |= EImageUsage::InputAttachment;			break;
				case VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV :
				case VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT :
				case VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM :			RETURN_ERR( "not supported" );
			}
			DISABLE_ENUM_CHECKS();
		}
		return result;
	}

/*
=================================================
	FGEnumCast (VkSampleCountFlagBits)
=================================================
*/
	ND_ inline  MultiSamples  FGEnumCast (VkSampleCountFlagBits samples)
	{
		if ( samples == 0 )
			return 1_samples;

		ASSERT( IsPowerOfTwo( samples ) );
		return MultiSamples{ uint(samples) };
	}
	
/*
=================================================
	ImageAspectFlags
=================================================
*/
	ND_ inline EImageAspect  FGEnumCast (VkImageAspectFlagBits values)
	{
		EImageAspect	result = EImageAspect(0);

		for (VkImageAspectFlags t = 1; t <= VK_IMAGE_ASPECT_METADATA_BIT; t <<= 1)
		{
			if ( not EnumEq( values, t ) )
				continue;
			
			ENABLE_ENUM_CHECKS();
			switch ( VkImageAspectFlagBits(t) )
			{
				case VK_IMAGE_ASPECT_COLOR_BIT :		result |= EImageAspect::Color;		break;
				case VK_IMAGE_ASPECT_DEPTH_BIT :		result |= EImageAspect::Depth;		break;
				case VK_IMAGE_ASPECT_STENCIL_BIT :		result |= EImageAspect::Stencil;		break;
				case VK_IMAGE_ASPECT_METADATA_BIT :		result |= EImageAspect::Metadata;		break;
				case VK_IMAGE_ASPECT_PLANE_0_BIT :
				case VK_IMAGE_ASPECT_PLANE_1_BIT :
				case VK_IMAGE_ASPECT_PLANE_2_BIT :
				case VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT :
				case VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT :
				case VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT :
				case VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT :
				case VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM :	// to shutup warnings
				default :								RETURN_ERR( "invalid image aspect type", EImageAspect::Auto );
			}
			DISABLE_ENUM_CHECKS();
		}
		return result;
	}


}	// FG

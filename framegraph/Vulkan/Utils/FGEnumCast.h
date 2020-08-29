// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
			if ( not AllBits( flags, t ))
				continue;
			
			BEGIN_ENUM_CHECKS();
			switch ( VkBufferUsageFlagBits(t) )
			{
				case VK_BUFFER_USAGE_TRANSFER_SRC_BIT :				result |= EBufferUsage::TransferSrc;	break;
				case VK_BUFFER_USAGE_TRANSFER_DST_BIT :				result |= EBufferUsage::TransferDst;	break;
				case VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT :		result |= EBufferUsage::UniformTexel;	break;
				case VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT :		result |= EBufferUsage::StorageTexel;	break;
				case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT :			result |= EBufferUsage::Uniform;		break;
				case VK_BUFFER_USAGE_STORAGE_BUFFER_BIT :			result |= EBufferUsage::Storage;		break;
				case VK_BUFFER_USAGE_INDEX_BUFFER_BIT :				result |= EBufferUsage::Index;			break;
				case VK_BUFFER_USAGE_VERTEX_BUFFER_BIT :			result |= EBufferUsage::Vertex;			break;
				case VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT :			result |= EBufferUsage::Indirect;		break;
				
				#ifdef VK_NV_ray_tracing
				case VK_BUFFER_USAGE_RAY_TRACING_BIT_NV :			result |= EBufferUsage::RayTracing;		break;
				#endif
				#ifdef VK_KHR_buffer_device_address
				case VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT:	result |= EBufferUsage::ShaderAddress;	break;
				#endif
				#ifdef VK_EXT_transform_feedback
				case VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT :
				case VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT :
				#endif
				#ifdef VK_EXT_conditional_rendering
				case VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT :
				#endif

				case VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM :
				default :											RETURN_ERR( "invalid buffer usage" );
			}
			END_ENUM_CHECKS();
		}
		return result;
	}
	

/*
=================================================
	FGEnumCast (VkImageType)
=================================================
*/
	ND_ inline EImageDim  FGEnumCast (VkImageType value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case VK_IMAGE_TYPE_1D :			return EImageDim_1D;
			case VK_IMAGE_TYPE_2D :			return EImageDim_2D;
			case VK_IMAGE_TYPE_3D :			return EImageDim_3D;
			#ifndef VK_VERSION_1_2
			case VK_IMAGE_TYPE_RANGE_SIZE :
			#endif
			case VK_IMAGE_TYPE_MAX_ENUM :
			default :						break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown vulkan image type" );
	}

/*
=================================================
	FGEnumCast (VkImageCreateFlagBits)
=================================================
*/
	ND_ inline EImageFlags  FGEnumCast (VkImageCreateFlagBits values)
	{
		EImageFlags	result = Zero;

		for (uint t = 1; t <= uint(values); t <<= 1)
		{
			if ( not AllBits( values, t ))
				continue;
		
			BEGIN_ENUM_CHECKS();
			switch ( VkImageCreateFlagBits(t) )
			{
				case VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT :		result |= EImageFlags::CubeCompatible;		break;
				case VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT :	result |= EImageFlags::Array2DCompatible;	break;
				case VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT :		result |= EImageFlags::MutableFormat;		break;

				case VK_IMAGE_CREATE_SPARSE_BINDING_BIT :
				case VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT :
				case VK_IMAGE_CREATE_SPARSE_ALIASED_BIT :
				case VK_IMAGE_CREATE_ALIAS_BIT :
				case VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT :
				case VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT :
				case VK_IMAGE_CREATE_EXTENDED_USAGE_BIT :
				case VK_IMAGE_CREATE_PROTECTED_BIT :
				case VK_IMAGE_CREATE_DISJOINT_BIT :
				case VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT :
				case VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM :

				#ifdef VK_NV_corner_sampled_image
				case VK_IMAGE_CREATE_CORNER_SAMPLED_BIT_NV :
				#endif
				#ifdef VK_EXT_fragment_density_map
				case VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT :
				#endif

				default :
					RETURN_ERR( "unsupported image create flags" );
			}
			END_ENUM_CHECKS();
		}
		return result;
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
			if ( not AllBits( usage, t ))
				continue;
			
			BEGIN_ENUM_CHECKS();
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
				#ifdef VK_NV_shading_rate_image
				case VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV :		result |= EImageUsage::ShadingRate;				break;
				#endif
				#ifdef VK_EXT_fragment_density_map
				case VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT :	result |= EImageUsage::FragmentDensityMap;		break;
				#endif
				case VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM :			RETURN_ERR( "not supported" );
			}
			END_ENUM_CHECKS();
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

		ASSERT( IsPowerOfTwo( samples ));
		return MultiSamples{ uint(samples) };
	}
	
/*
=================================================
	ImageAspectFlags
=================================================
*/
	ND_ inline EImageAspect  FGEnumCast (VkImageAspectFlagBits values)
	{
		EImageAspect	result = Zero;

		for (VkImageAspectFlags t = 1; t <= VK_IMAGE_ASPECT_METADATA_BIT; t <<= 1)
		{
			if ( not AllBits( values, t ))
				continue;
			
			BEGIN_ENUM_CHECKS();
			switch ( VkImageAspectFlagBits(t) )
			{
				case VK_IMAGE_ASPECT_COLOR_BIT :		result |= EImageAspect::Color;		break;
				case VK_IMAGE_ASPECT_DEPTH_BIT :		result |= EImageAspect::Depth;		break;
				case VK_IMAGE_ASPECT_STENCIL_BIT :		result |= EImageAspect::Stencil;		break;
				case VK_IMAGE_ASPECT_METADATA_BIT :		result |= EImageAspect::Metadata;		break;
				case VK_IMAGE_ASPECT_PLANE_0_BIT :
				case VK_IMAGE_ASPECT_PLANE_1_BIT :
				case VK_IMAGE_ASPECT_PLANE_2_BIT :
				
				#ifdef VK_EXT_image_drm_format_modifier
				case VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT :
				case VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT :
				case VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT :
				case VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT :
				#endif

				case VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM :	// to shutup warnings
				default :								RETURN_ERR( "invalid image aspect type", EImageAspect::Auto );
			}
			END_ENUM_CHECKS();
		}
		return result;
	}


}	// FG

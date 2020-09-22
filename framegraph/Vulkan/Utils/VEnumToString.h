// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Shared/EnumToString.h"
#include "VCommon.h"

namespace FG
{

/*
=================================================
	VkPipelineStage_ToString
=================================================
*/
	ND_ inline String  VkPipelineStage_ToString (const VkPipelineStageFlags values)
	{
		String	result;
		for (VkPipelineStageFlags t = 1; t < VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM; t <<= 1)
		{
			if ( not AllBits( values, t ))
				continue;

			if ( not result.empty() )
				result << " | ";
			
			BEGIN_ENUM_CHECKS();
			switch ( VkPipelineStageFlagBits(t) )
			{
				case VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT :					result << "TopOfPipe";					break;
				case VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT :					result << "DrawIndirect";				break;
				case VK_PIPELINE_STAGE_VERTEX_INPUT_BIT :					result << "VertexInput";				break;
				case VK_PIPELINE_STAGE_VERTEX_SHADER_BIT :					result << "VertexShader";				break;
				case VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT :	result << "TessControlShader";			break;
				case VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT :	result << "TessEvaluationShader";		break;
				case VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT :				result << "GeometryShader";				break;
				case VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT :				result << "FragmentShader";				break;
				case VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT :			result << "EarlyFragmentTests";			break;
				case VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT :			result << "LateFragmentTests";			break;
				case VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT :		result << "ColorAttachmentOutput";		break;
				case VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT :					result << "ComputeShader";				break;
				case VK_PIPELINE_STAGE_TRANSFER_BIT :						result << "Transfer";					break;
				case VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT :					result << "BottomOfPipe";				break;
				case VK_PIPELINE_STAGE_HOST_BIT :							result << "Host";						break;
				case VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT :					result << "AllGraphics";				break;
				case VK_PIPELINE_STAGE_ALL_COMMANDS_BIT :					result << "AllCommands";				break;
				#ifdef VK_NV_device_generated_commands
				case VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV :			result << "CommandProcess";				break;
				#elif defined(VK_NVX_device_generated_commands)
				case VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX :			result << "CommandProcess";				break;
				#endif
				#ifdef VK_EXT_transform_feedback
				case VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT :			result << "TransformFeedback";			break;
				#endif
				#ifdef VK_EXT_conditional_rendering
				case VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT :		result << "ConditionalRendering";		break;
				#endif
				#ifdef VK_NV_shading_rate_image
				case VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV :			result << "ShadingRateImage";			break;
				#endif
				#ifdef VK_NV_ray_tracing
				case VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV :			result << "RayTracing";					break;
				case VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV:	result << "AccelerationStructureBuild";	break;
				#endif
				#ifdef VK_NV_mesh_shader
				case VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV :					result << "TaskShader";					break;
				case VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV :					result << "MeshShader";					break;
				#endif
				#ifdef VK_EXT_fragment_density_map
				case VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT :	result << "FragmentDensityProcess";		break;
				#endif
				case VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM :
				default :													RETURN_ERR( "unknown pipeline stage!" );
			}
			END_ENUM_CHECKS();
		}

		if ( result.empty() )
			result << "0";

		return result;
	}
	
/*
=================================================
	VkPipelineStage_ToString
=================================================
*/
	ND_ inline String  VkDependency_ToString (const VkDependencyFlags values)
	{
		String	result;
		for (VkDependencyFlags t = 1; t < VK_DEPENDENCY_FLAG_BITS_MAX_ENUM; t <<= 1)
		{
			if ( not AllBits( values, t ))
				continue;

			if ( not result.empty() )
				result << " | ";
			
			BEGIN_ENUM_CHECKS();
			switch ( VkDependencyFlagBits(t) )
			{
				case VK_DEPENDENCY_BY_REGION_BIT :		result << "ByRegion";		break;
				case VK_DEPENDENCY_DEVICE_GROUP_BIT :	result << "DeviceGroup";	break;
				case VK_DEPENDENCY_VIEW_LOCAL_BIT :		result << "ViewLocal";		break;
				case VK_DEPENDENCY_FLAG_BITS_MAX_ENUM :
				default :								RETURN_ERR( "unknown dependency flags!" );
			}
			END_ENUM_CHECKS();
		}

		if ( result.empty() )
			result << "0";

		return result;
	}
	
/*
=================================================
	VkAccess_ToString
=================================================
*/
	ND_ inline String  VkAccess_ToString (const VkAccessFlags values)
	{
		String	result;
		for (VkAccessFlags t = 1; t < VK_ACCESS_FLAG_BITS_MAX_ENUM; t <<= 1)
		{
			if ( not AllBits( values, t ))
				continue;

			if ( not result.empty() )
				result << " | ";
			
			BEGIN_ENUM_CHECKS();
			switch ( VkAccessFlagBits(t) )
			{
				case VK_ACCESS_INDIRECT_COMMAND_READ_BIT :					result << "IndirectCommandRead";				break;
				case VK_ACCESS_INDEX_READ_BIT :								result << "IndexRead";							break;
				case VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT :					result << "VertexAttributeRead";				break;
				case VK_ACCESS_UNIFORM_READ_BIT :							result << "UniformRead";						break;
				case VK_ACCESS_INPUT_ATTACHMENT_READ_BIT :					result << "InputAttachmentRead";				break;
				case VK_ACCESS_SHADER_READ_BIT :							result << "ShaderRead";							break;
				case VK_ACCESS_SHADER_WRITE_BIT :							result << "ShaderWrite";						break;
				case VK_ACCESS_COLOR_ATTACHMENT_READ_BIT :					result << "ColorAttachmentRead";				break;
				case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT :					result << "ColorAttachmentWrite";				break;
				case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT :			result << "DepthStencilAttachmentRead";			break;
				case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT :			result << "DepthStencilAttachmentWrite";		break;
				case VK_ACCESS_TRANSFER_READ_BIT :							result << "TransferRead";						break;
				case VK_ACCESS_TRANSFER_WRITE_BIT :							result << "TransferWrite";						break;
				case VK_ACCESS_HOST_READ_BIT :								result << "HostRead";							break;
				case VK_ACCESS_HOST_WRITE_BIT :								result << "HostWrite";							break;
				case VK_ACCESS_MEMORY_READ_BIT :							result << "MemoryRead";							break;
				case VK_ACCESS_MEMORY_WRITE_BIT :							result << "MemoryWrite";						break;
				#ifdef VK_NV_device_generated_commands
				case VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV :				result << "CommandProcessRead";					break;
				case VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV :			result << "CommandProcessWrite";				break;
				#elif defined(VK_NVX_device_generated_commands)
				case VK_ACCESS_COMMAND_PROCESS_READ_BIT_NVX :				result << "CommandProcessRead";					break;
				case VK_ACCESS_COMMAND_PROCESS_WRITE_BIT_NVX :				result << "CommandProcessWrite";				break;
				#endif
				#ifdef VK_EXT_blend_operation_advanced
				case VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT :	result << "ColorAttachmentReadNoncoherent";		break;
				#endif
				#ifdef VK_EXT_transform_feedback
				case VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT :			result << "TransformFeedbackReadNoncoherent";	break;
				case VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT :	result << "TransformFeedbackCounterRead";		break;
				case VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT :	result << "TransformFeedbackCounterWrite";		break;
				#endif
				#ifdef VK_EXT_conditional_rendering
				case VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT :			result << "ConditionalRenderingRead";			break;
				#endif
				#ifdef VK_NV_shading_rate_image
				case VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV :				result << "ShadingRateImageRead";				break;
				#endif
				#ifdef VK_NV_ray_tracing
				case VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV :			result << "AccelerationStructureRead";			break;
				case VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV :		result << "AccelerationStructureWrite";			break;
				#endif
				#ifdef VK_EXT_fragment_density_map
				case VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT :			result << "FragmentDensityRead";				break;
				#endif
				case VK_ACCESS_FLAG_BITS_MAX_ENUM :
				default :													RETURN_ERR( "unknown access flags!" );
			}
			END_ENUM_CHECKS();
		}

		if ( result.empty() )
			result << "0";

		return result;
	}
	
/*
=================================================
	VkImageLayout_ToString
=================================================
*/
	ND_ inline String  VkImageLayout_ToString (const VkImageLayout value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case VK_IMAGE_LAYOUT_UNDEFINED :									return "Undefined";
			case VK_IMAGE_LAYOUT_GENERAL :										return "General";
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :						return "ColorAttachmentOptimal";
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :				return "DepthStencilAttachmentOptimal";
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL :				return "DepthStencilReadOnlyOptimal";
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :						return "ShaderReadOnlyOptimal";
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL :							return "TransferSrcOptimal";
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL :							return "TransferDstOptimal";
			case VK_IMAGE_LAYOUT_PREINITIALIZED :								return "Preinitialized";
			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL :	return "DepthReadOnlyStencilAttachmentOptimal";
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL :	return "DepthAttachmentStencilReadOnlyOptimal";
			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :								return "PresentSrc";
			case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR :							return "SharedPresent";	
			#ifdef VK_NV_shading_rate_image
			case VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV :						return "ShadingRateOptimal";
			#endif
			#ifdef VK_EXT_fragment_density_map
			case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT :				return "FragmentDensityMapOptimal";
			#endif
			#ifdef VK_KHR_separate_depth_stencil_layouts
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR :					return "DepthAttachmentOptimal";
			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR :					return "DepthReadOnly";
			case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR :				return "StencilAttachmentOptimal";
			case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR :				return "StencilReadOnly";
			#endif
			#ifndef VK_VERSION_1_2
			case VK_IMAGE_LAYOUT_RANGE_SIZE :
			#endif
			case VK_IMAGE_LAYOUT_MAX_ENUM :										break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown image layout!" );
	}
	
/*
=================================================
	VkImageAspect_ToString
=================================================
*/
	ND_ inline String  VkImageAspect_ToString (const VkImageAspectFlags values)
	{
		String	result;
		for (VkImageAspectFlags t = 1; t < VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM; t <<= 1)
		{
			if ( not AllBits( values, t ))
				continue;

			if ( not result.empty() )
				result << " | ";
			
			BEGIN_ENUM_CHECKS();
			switch ( VkImageAspectFlagBits(t) )
			{
				case VK_IMAGE_ASPECT_COLOR_BIT :				result << "Color";			break;
				case VK_IMAGE_ASPECT_DEPTH_BIT :				result << "Depth";			break;
				case VK_IMAGE_ASPECT_STENCIL_BIT :				result << "Stencil";		break;
				case VK_IMAGE_ASPECT_METADATA_BIT :				result << "Metadata";		break;
				case VK_IMAGE_ASPECT_PLANE_0_BIT :				result << "Plane0";			break;
				case VK_IMAGE_ASPECT_PLANE_1_BIT :				result << "Plane1";			break;
				case VK_IMAGE_ASPECT_PLANE_2_BIT :				result << "Plane2";			break;
				#ifdef VK_EXT_image_drm_format_modifier
				case VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT :	result << "MemoryPlane0";	break;
				case VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT :	result << "MemoryPlane1";	break;
				case VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT :	result << "MemoryPlane2";	break;
				case VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT :	result << "MemoryPlane3";	break;
				#endif
				case VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM :
				default :										RETURN_ERR( "unknown access flags!" );
			}
			END_ENUM_CHECKS();
		}

		if ( result.empty() )
			result << "0";

		return result;
	}
	
/*
=================================================
	VkFilter_ToString
=================================================
*/
	ND_ inline StringView  VkFilter_ToString (const VkFilter value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case VK_FILTER_NEAREST :		return "Nearest";
			case VK_FILTER_LINEAR :			return "Linear";
			case VK_FILTER_CUBIC_IMG :		return "CubicImg";
			#ifndef VK_VERSION_1_2
			case VK_FILTER_RANGE_SIZE :
			#endif
			case VK_FILTER_MAX_ENUM :		break;
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown filter type!" );
	}
	
/*
=================================================
	VkColorSpaceKHR_ToString
=================================================
*/
	ND_ inline StringView  VkColorSpaceKHR_ToString (VkColorSpaceKHR value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR :			return "sRGB Nonlinear";
			#ifdef VK_EXT_swapchain_colorspace
			case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT :		return "Display P3 Nonlinear";
			case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT :		return "Extended sRGB Linear";
			case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT :			return "Display P3 Linear";
			case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT :			return "DCI P3 Nonlinear";
			case VK_COLOR_SPACE_BT709_LINEAR_EXT :				return "BT709 Linear";
			case VK_COLOR_SPACE_BT709_NONLINEAR_EXT :			return "BT709 Nonlinear";
			case VK_COLOR_SPACE_BT2020_LINEAR_EXT :				return "BT2020 Linear";
			case VK_COLOR_SPACE_HDR10_ST2084_EXT :				return "HDR10 ST2084";
			case VK_COLOR_SPACE_DOLBYVISION_EXT :				return "Dolbyvision";
			case VK_COLOR_SPACE_HDR10_HLG_EXT :					return "HDR10 HLG";
			case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT :			return "AdobeRGB Linear";
			case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT :		return "AdobeRGB Nonlinear";
			case VK_COLOR_SPACE_PASS_THROUGH_EXT :				return "Pass through";
			case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT :	return "Extended sRGB Nonlinear";
			#endif
			#ifdef VK_AMD_display_native_hdr
			case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD :			return "Display Native AMD";
			#endif
			case VK_COLOR_SPACE_MAX_ENUM_KHR :					break;
			#ifndef VK_VERSION_1_2
			case VK_COLOR_SPACE_RANGE_SIZE_KHR :				break;
			#endif
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown color space type!" );
	}
	
/*
=================================================
	VkPresentModeKHR_ToString
=================================================
*/
	ND_ inline String  VkPresentModeKHR_ToString (VkPresentModeKHR value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case VK_PRESENT_MODE_IMMEDIATE_KHR :					return "Immediate";
			case VK_PRESENT_MODE_MAILBOX_KHR :						return "Mailbox";
			case VK_PRESENT_MODE_FIFO_KHR :							return "FIFO";
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR :					return "FIFO Relaxed";
			case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR :		return "Shared Dmand Refresh";
			case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR :	return "Shared Continoous Refresh";
			case VK_PRESENT_MODE_MAX_ENUM_KHR :						break;
			#ifndef VK_VERSION_1_2
			case VK_PRESENT_MODE_RANGE_SIZE_KHR :					break;
			#endif
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown present mode!" );
	}
	
/*
=================================================
	VkSurfaceTransformFlagBitsKHR_ToString
=================================================
*/
	ND_ inline String  VkSurfaceTransformFlagBitsKHR_ToString (VkSurfaceTransformFlagBitsKHR value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR :					return "Identity";
			case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR :					return "90";
			case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR :					return "180";
			case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR :					return "270";
			case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR :			return "HorizontalMirror";
			case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR:	return "HorizontalMirror 90";
			case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR:	return "HorizontalMirror 180";
			case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR:	return "HorizontalMirror 270";
			case VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR :						return "Inherit";
			#ifndef VK_VULKAN_1_2
			case VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR :				break;
			#endif
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown surface transform!" );
	}
	
/*
=================================================
	VkCompositeAlphaFlagBitsKHR_ToString
=================================================
*/
	ND_ inline String  VkCompositeAlphaFlagBitsKHR_ToString (VkCompositeAlphaFlagBitsKHR value)
	{
		BEGIN_ENUM_CHECKS();
		switch ( value )
		{
			case VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR :			return "Opaque";
			case VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR :	return "Pre-multiplied";
			case VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR :	return "Post-multiplied";
			case VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR :			return "Inherit";
			#ifndef VK_VULKAN_1_2
			case VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR :	break;
			#endif
		}
		END_ENUM_CHECKS();
		RETURN_ERR( "unknown composite alpha!" );
	}
	
/*
=================================================
	VkImageUsageFlags_ToString
=================================================
*/
	ND_ inline String  VkImageUsageFlags_ToString (VkImageUsageFlags value)
	{
		String	result;
		for (VkImageUsageFlags i = 1; i <= value; i <<= 1)
		{
			if ( not AllBits( value, i ))
				continue;

			if ( result.size() )
				result << " | ";

			BEGIN_ENUM_CHECKS();
			switch ( VkImageUsageFlagBits(i) )
			{
				case VK_IMAGE_USAGE_TRANSFER_SRC_BIT :				result << "TransferSrc";				break;
				case VK_IMAGE_USAGE_TRANSFER_DST_BIT :				result << "TranferDst";					break;
				case VK_IMAGE_USAGE_SAMPLED_BIT :					result << "Sampled";					break;
				case VK_IMAGE_USAGE_STORAGE_BIT :					result << "Storage";					break;
				case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT :			result << "ColorAttachment";			break;
				case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :	result << "DepthStencilAttachment";		break;
				case VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT :		result << "TransientAttachment";		break;
				case VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT :			result << "InputAttachment";			break;
				#ifdef VK_NV_shading_rate_image
				case VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV :
				#endif
				#ifdef VK_EXT_fragment_density_map
				case VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT :
				#endif
				case VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM :			ASSERT(false); break;
			}
			END_ENUM_CHECKS();
		}
		return result;
	}

}	// FG

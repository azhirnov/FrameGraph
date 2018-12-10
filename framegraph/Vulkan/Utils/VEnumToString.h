// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Shared/EnumToString.h"

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
			if ( not EnumEq( values, t ) )
				continue;

			if ( not result.empty() )
				result << " | ";
			
			ENABLE_ENUM_CHECKS();
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
				case VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX :			result << "CommandProcess";				break;
				case VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT :			result << "TransformFeedback";			break;
				case VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT :		result << "ConditionalRendering";		break;
				case VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV :			result << "ShadingRateImage";			break;
				case VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV :			result << "RayTracing";					break;
				case VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV:	result << "AccelerationStructureBuild";	break;
				case VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV :					result << "TaskShader";					break;
				case VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV :					result << "MeshShader";					break;
				case VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM :
				default :													RETURN_ERR( "unknown pipeline stage!" );
			}
			DISABLE_ENUM_CHECKS();
		}
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
			if ( not EnumEq( values, t ) )
				continue;

			if ( not result.empty() )
				result << " | ";
			
			ENABLE_ENUM_CHECKS();
			switch ( VkDependencyFlagBits(t) )
			{
				case VK_DEPENDENCY_BY_REGION_BIT :		result << "ByRegion";		break;
				case VK_DEPENDENCY_DEVICE_GROUP_BIT :	result << "DeviceGroup";	break;
				case VK_DEPENDENCY_VIEW_LOCAL_BIT :		result << "ViewLocal";		break;
				case VK_DEPENDENCY_FLAG_BITS_MAX_ENUM :
				default :								RETURN_ERR( "unknown dependency flags!" );
			}
			DISABLE_ENUM_CHECKS();
		}
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
			if ( not EnumEq( values, t ) )
				continue;

			if ( not result.empty() )
				result << " | ";
			
			ENABLE_ENUM_CHECKS();
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
				case VK_ACCESS_COMMAND_PROCESS_READ_BIT_NVX :				result << "CommandProcessRead";					break;
				case VK_ACCESS_COMMAND_PROCESS_WRITE_BIT_NVX :				result << "CommandProcessWrite";				break;
				case VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT :	result << "ColorAttachmentReadNoncoherent";		break;
				case VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT :			result << "TransformFeedbackReadNoncoherent";	break;
				case VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT :	result << "TransformFeedbackCounterRead";		break;
				case VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT :	result << "TransformFeedbackCounterWrite";		break;
				case VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT :			result << "ConditionalRenderingRead";			break;
				case VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV :				result << "ShadingRateImageRead";				break;
				case VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV :			result << "AccelerationStructureRead";			break;
				case VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV :		result << "AccelerationStructureWrite";			break;
				case VK_ACCESS_FLAG_BITS_MAX_ENUM :
				default :													RETURN_ERR( "unknown access flags!" );
			}
			DISABLE_ENUM_CHECKS();
		}
		return result;
	}
	
/*
=================================================
	VkImageLayout_ToString
=================================================
*/
	ND_ inline String  VkImageLayout_ToString (const VkImageLayout value)
	{
		ENABLE_ENUM_CHECKS();
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
			case VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV :						return "ShadingRateOptimal";
			case VK_IMAGE_LAYOUT_RANGE_SIZE :
			case VK_IMAGE_LAYOUT_MAX_ENUM :										break;
		}
		DISABLE_ENUM_CHECKS();
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
			if ( not EnumEq( values, t ) )
				continue;

			if ( not result.empty() )
				result << " | ";
			
			ENABLE_ENUM_CHECKS();
			switch ( VkImageAspectFlagBits(t) )
			{
				case VK_IMAGE_ASPECT_COLOR_BIT :				result << "Color";			break;
				case VK_IMAGE_ASPECT_DEPTH_BIT :				result << "Depth";			break;
				case VK_IMAGE_ASPECT_STENCIL_BIT :				result << "Stencil";		break;
				case VK_IMAGE_ASPECT_METADATA_BIT :				result << "Metadata";		break;
				case VK_IMAGE_ASPECT_PLANE_0_BIT :				result << "Plane0";			break;
				case VK_IMAGE_ASPECT_PLANE_1_BIT :				result << "Plane1";			break;
				case VK_IMAGE_ASPECT_PLANE_2_BIT :				result << "Plane2";			break;
				case VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT :	result << "MemoryPlane0";	break;
				case VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT :	result << "MemoryPlane1";	break;
				case VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT :	result << "MemoryPlane2";	break;
				case VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT :	result << "MemoryPlane3";	break;
				case VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM :
				default :										RETURN_ERR( "unknown access flags!" );
			}
			DISABLE_ENUM_CHECKS();
		}
		return result;
	}
	
/*
=================================================
	VkFilter_ToString
=================================================
*/
	ND_ inline StringView  VkFilter_ToString (const VkFilter value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case VK_FILTER_NEAREST :		return "Nearest";
			case VK_FILTER_LINEAR :			return "Linear";
			case VK_FILTER_CUBIC_IMG :		return "CubicImg";
			case VK_FILTER_RANGE_SIZE :
			case VK_FILTER_MAX_ENUM :		break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown filter type!" );
	}


}	// FG

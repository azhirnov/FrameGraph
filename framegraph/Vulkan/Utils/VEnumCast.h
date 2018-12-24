// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/ResourceEnums.h"
#include "framegraph/Public/RenderStateEnums.h"
#include "framegraph/Public/ShaderEnums.h"
#include "framegraph/Public/SamplerEnums.h"
#include "framegraph/Public/EResourceState.h"
#include "framegraph/Shared/EnumUtils.h"
#include "framegraph/Public/RayTracingGeometryDesc.h"
#include "framegraph/Public/RayTracingSceneDesc.h"

namespace FG
{

/*
=================================================
	MultiSamples
=================================================
*/
	ND_ inline VkSampleCountFlagBits  VEnumCast (MultiSamples value)
	{
		return Clamp( VkSampleCountFlagBits(value.Get()), VK_SAMPLE_COUNT_1_BIT, VK_SAMPLE_COUNT_64_BIT );
	}
	
/*
=================================================
	LogicOp
=================================================
*/
	ND_ inline VkLogicOp  VEnumCast (ELogicOp value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case ELogicOp::Clear		: return VK_LOGIC_OP_CLEAR;
			case ELogicOp::Set			: return VK_LOGIC_OP_SET;
			case ELogicOp::Copy			: return VK_LOGIC_OP_COPY;
			case ELogicOp::CopyInverted	: return VK_LOGIC_OP_COPY_INVERTED;
			case ELogicOp::NoOp			: return VK_LOGIC_OP_NO_OP;
			case ELogicOp::Invert		: return VK_LOGIC_OP_INVERT;
			case ELogicOp::And			: return VK_LOGIC_OP_AND;
			case ELogicOp::NotAnd		: return VK_LOGIC_OP_NAND;
			case ELogicOp::Or			: return VK_LOGIC_OP_OR;
			case ELogicOp::NotOr		: return VK_LOGIC_OP_NOR;
			case ELogicOp::Xor			: return VK_LOGIC_OP_XOR;
			case ELogicOp::Equiv		: return VK_LOGIC_OP_EQUIVALENT;
			case ELogicOp::AndReverse	: return VK_LOGIC_OP_AND_REVERSE;
			case ELogicOp::AndInverted	: return VK_LOGIC_OP_AND_INVERTED;
			case ELogicOp::OrReverse	: return VK_LOGIC_OP_OR_REVERSE;
			case ELogicOp::OrInverted	: return VK_LOGIC_OP_OR_INVERTED;
			case ELogicOp::None			:
			case ELogicOp::Unknown		: break;	// not supported
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "invalid logical op", VK_LOGIC_OP_MAX_ENUM );
	}
	
/*
=================================================
	BlendFactor
=================================================
*/
	ND_ inline VkBlendFactor  VEnumCast (EBlendFactor value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EBlendFactor::Zero					: return VK_BLEND_FACTOR_ZERO;
			case EBlendFactor::One					: return VK_BLEND_FACTOR_ONE;
			case EBlendFactor::SrcColor				: return VK_BLEND_FACTOR_SRC_COLOR;
			case EBlendFactor::OneMinusSrcColor		: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			case EBlendFactor::DstColor				: return VK_BLEND_FACTOR_DST_COLOR;
			case EBlendFactor::OneMinusDstColor		: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			case EBlendFactor::SrcAlpha				: return VK_BLEND_FACTOR_SRC_ALPHA;
			case EBlendFactor::OneMinusSrcAlpha		: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			case EBlendFactor::DstAlpha				: return VK_BLEND_FACTOR_DST_ALPHA;
			case EBlendFactor::OneMinusDstAlpha		: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			case EBlendFactor::ConstColor			: return VK_BLEND_FACTOR_CONSTANT_COLOR;
			case EBlendFactor::OneMinusConstColor	: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
			case EBlendFactor::ConstAlpha			: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
			case EBlendFactor::OneMinusConstAlpha	: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
			case EBlendFactor::SrcAlphaSaturate		: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			case EBlendFactor::Src1Color			: return VK_BLEND_FACTOR_SRC1_COLOR;
			case EBlendFactor::OneMinusSrc1Color	: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
			case EBlendFactor::Src1Alpha			: return VK_BLEND_FACTOR_SRC1_ALPHA;
			case EBlendFactor::OneMinusSrc1Alpha	: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
			case EBlendFactor::Unknown				: break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "invalid blend func", VK_BLEND_FACTOR_MAX_ENUM );
	}
	
/*
=================================================
	BlendOp
=================================================
*/
	ND_ inline VkBlendOp  VEnumCast (EBlendOp value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EBlendOp::Add		: return VK_BLEND_OP_ADD;
			case EBlendOp::Sub		: return VK_BLEND_OP_SUBTRACT;
			case EBlendOp::RevSub	: return VK_BLEND_OP_REVERSE_SUBTRACT;
			case EBlendOp::Min		: return VK_BLEND_OP_MIN;
			case EBlendOp::Max		: return VK_BLEND_OP_MAX;
			case EBlendOp::Unknown	: break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "invalid blend equation", VK_BLEND_OP_MAX_ENUM );
	}

/*
=================================================
	VertexType
=================================================
*/
	ND_ inline VkFormat  VEnumCast (EVertexType value)
	{
		switch ( value )
		{
			case EVertexType::Byte			: return VK_FORMAT_R8_SINT;
			case EVertexType::Byte2			: return VK_FORMAT_R8G8_SINT;
			case EVertexType::Byte3			: return VK_FORMAT_R8G8B8_SINT;
			case EVertexType::Byte4			: return VK_FORMAT_R8G8B8A8_SINT;
			case EVertexType::Byte_Norm		: return VK_FORMAT_R8_SNORM;
			case EVertexType::Byte2_Norm	: return VK_FORMAT_R8G8_SNORM;
			case EVertexType::Byte3_Norm	: return VK_FORMAT_R8G8B8_SNORM;
			case EVertexType::Byte4_Norm	: return VK_FORMAT_R8G8B8A8_SNORM;
			case EVertexType::Byte_Scaled	: return VK_FORMAT_R8_SSCALED;
			case EVertexType::Byte2_Scaled	: return VK_FORMAT_R8G8_SSCALED;
			case EVertexType::Byte3_Scaled	: return VK_FORMAT_R8G8B8_SSCALED;
			case EVertexType::Byte4_Scaled	: return VK_FORMAT_R8G8B8A8_SSCALED;
			case EVertexType::UByte			: return VK_FORMAT_R8_UINT;
			case EVertexType::UByte2		: return VK_FORMAT_R8G8_UINT;
			case EVertexType::UByte3		: return VK_FORMAT_R8G8B8_UINT;
			case EVertexType::UByte4		: return VK_FORMAT_R8G8B8A8_UINT;
			case EVertexType::UByte_Norm	: return VK_FORMAT_R8_UNORM;
			case EVertexType::UByte2_Norm	: return VK_FORMAT_R8G8_UNORM;
			case EVertexType::UByte3_Norm	: return VK_FORMAT_R8G8B8_UNORM;
			case EVertexType::UByte4_Norm	: return VK_FORMAT_R8G8B8A8_UNORM;
			case EVertexType::UByte_Scaled	: return VK_FORMAT_R8_USCALED;
			case EVertexType::UByte2_Scaled	: return VK_FORMAT_R8G8_USCALED;
			case EVertexType::UByte3_Scaled	: return VK_FORMAT_R8G8B8_USCALED;
			case EVertexType::UByte4_Scaled	: return VK_FORMAT_R8G8B8A8_USCALED;
			case EVertexType::Short			: return VK_FORMAT_R16_SINT;
			case EVertexType::Short2		: return VK_FORMAT_R16G16_SINT;
			case EVertexType::Short3		: return VK_FORMAT_R16G16B16_SINT;
			case EVertexType::Short4		: return VK_FORMAT_R16G16B16A16_SINT;
			case EVertexType::Short_Norm	: return VK_FORMAT_R16_SNORM;
			case EVertexType::Short2_Norm	: return VK_FORMAT_R16G16_SNORM;
			case EVertexType::Short3_Norm	: return VK_FORMAT_R16G16B16_SNORM;
			case EVertexType::Short4_Norm	: return VK_FORMAT_R16G16B16A16_SNORM;
			case EVertexType::Short_Scaled	: return VK_FORMAT_R16_SSCALED;
			case EVertexType::Short2_Scaled	: return VK_FORMAT_R16G16_SSCALED;
			case EVertexType::Short3_Scaled	: return VK_FORMAT_R16G16B16_SSCALED;
			case EVertexType::Short4_Scaled	: return VK_FORMAT_R16G16B16A16_SSCALED;
			case EVertexType::UShort		: return VK_FORMAT_R16_UINT;
			case EVertexType::UShort2		: return VK_FORMAT_R16G16_UINT;
			case EVertexType::UShort3		: return VK_FORMAT_R16G16B16_UINT;
			case EVertexType::UShort4		: return VK_FORMAT_R16G16B16A16_UINT;
			case EVertexType::UShort_Norm	: return VK_FORMAT_R16_UNORM;
			case EVertexType::UShort2_Norm	: return VK_FORMAT_R16G16_UNORM;
			case EVertexType::UShort3_Norm	: return VK_FORMAT_R16G16B16_UNORM;
			case EVertexType::UShort4_Norm	: return VK_FORMAT_R16G16B16A16_UNORM;
			case EVertexType::UShort_Scaled	: return VK_FORMAT_R16_USCALED;
			case EVertexType::UShort2_Scaled: return VK_FORMAT_R16G16_USCALED;
			case EVertexType::UShort3_Scaled: return VK_FORMAT_R16G16B16_USCALED;
			case EVertexType::UShort4_Scaled: return VK_FORMAT_R16G16B16A16_USCALED;
			case EVertexType::Int			: return VK_FORMAT_R32_SINT;
			case EVertexType::Int2			: return VK_FORMAT_R32G32_SINT;
			case EVertexType::Int3			: return VK_FORMAT_R32G32B32_SINT;
			case EVertexType::Int4			: return VK_FORMAT_R32G32B32A32_SINT;
			case EVertexType::UInt			: return VK_FORMAT_R32_UINT;
			case EVertexType::UInt2			: return VK_FORMAT_R32G32_UINT;
			case EVertexType::UInt3			: return VK_FORMAT_R32G32B32_UINT;
			case EVertexType::UInt4			: return VK_FORMAT_R32G32B32A32_UINT;
			case EVertexType::Long			: return VK_FORMAT_R64_SINT;
			case EVertexType::Long2			: return VK_FORMAT_R64G64_SINT;
			case EVertexType::Long3			: return VK_FORMAT_R64G64B64_SINT;
			case EVertexType::Long4			: return VK_FORMAT_R64G64B64A64_SINT;
			case EVertexType::ULong			: return VK_FORMAT_R64_UINT;
			case EVertexType::ULong2		: return VK_FORMAT_R64G64_UINT;
			case EVertexType::ULong3		: return VK_FORMAT_R64G64B64_UINT;
			case EVertexType::ULong4		: return VK_FORMAT_R64G64B64A64_UINT;
			case EVertexType::Float			: return VK_FORMAT_R32_SFLOAT;
			case EVertexType::Float2		: return VK_FORMAT_R32G32_SFLOAT;
			case EVertexType::Float3		: return VK_FORMAT_R32G32B32_SFLOAT;
			case EVertexType::Float4		: return VK_FORMAT_R32G32B32A32_SFLOAT;
			case EVertexType::Double		: return VK_FORMAT_R64_SFLOAT;
			case EVertexType::Double2		: return VK_FORMAT_R64G64_SFLOAT;
			case EVertexType::Double3		: return VK_FORMAT_R64G64B64_SFLOAT;
			case EVertexType::Double4		: return VK_FORMAT_R64G64B64A64_SFLOAT;
		}
		RETURN_ERR( "unknown vertex type!", VK_FORMAT_MAX_ENUM );
	}
	
/*
=================================================
	VertexInputRate
=================================================
*/
	ND_ inline VkVertexInputRate  VEnumCast (EVertexInputRate value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EVertexInputRate::Vertex :		return VK_VERTEX_INPUT_RATE_VERTEX;
			case EVertexInputRate::Instance :	return VK_VERTEX_INPUT_RATE_INSTANCE;
			case EVertexInputRate::Unknown :	break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown vertex input rate", VK_VERTEX_INPUT_RATE_MAX_ENUM );
	}
	
/*
=================================================
	ShaderStage
=================================================
*/
	ND_ inline VkShaderStageFlagBits  VEnumCast (EShader value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EShader::Vertex :			return VK_SHADER_STAGE_VERTEX_BIT;
			case EShader::TessControl :		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			case EShader::TessEvaluation :	return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			case EShader::Geometry :		return VK_SHADER_STAGE_GEOMETRY_BIT;
			case EShader::Fragment :		return VK_SHADER_STAGE_FRAGMENT_BIT;
			case EShader::Compute :			return VK_SHADER_STAGE_COMPUTE_BIT;
				
			case EShader::MeshTask :		return VK_SHADER_STAGE_TASK_BIT_NV;
			case EShader::Mesh :			return VK_SHADER_STAGE_MESH_BIT_NV;
				
			case EShader::RayGen :			return VK_SHADER_STAGE_RAYGEN_BIT_NV;
			case EShader::RayAnyHit :		return VK_SHADER_STAGE_ANY_HIT_BIT_NV;
			case EShader::RayClosestHit :	return VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
			case EShader::RayMiss :			return VK_SHADER_STAGE_MISS_BIT_NV;
			case EShader::RayIntersection :	return VK_SHADER_STAGE_INTERSECTION_BIT_NV;
			case EShader::RayCallable :		return VK_SHADER_STAGE_CALLABLE_BIT_NV;

			case EShader::Unknown :
			case EShader::_Count :			break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown shader type!", VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM );
	}

/*
=================================================
	ShaderStages
=================================================
*/
	ND_ inline VkShaderStageFlags  VEnumCast (EShaderStages values)
	{
		VkShaderStageFlags	flags = 0;

		for (EShaderStages t = EShaderStages(1 << 0); t < EShaderStages::_Last; t = EShaderStages(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;

			ENABLE_ENUM_CHECKS();
			switch ( t )
			{
				case EShaderStages::Vertex :			flags |= VK_SHADER_STAGE_VERTEX_BIT;					break;
				case EShaderStages::TessControl :		flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;		break;
				case EShaderStages::TessEvaluation :	flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;	break;
				case EShaderStages::Geometry :			flags |= VK_SHADER_STAGE_GEOMETRY_BIT;					break;
				case EShaderStages::Fragment :			flags |= VK_SHADER_STAGE_FRAGMENT_BIT;					break;
				case EShaderStages::Compute :			flags |= VK_SHADER_STAGE_COMPUTE_BIT;					break;
				case EShaderStages::MeshTask :			flags |= VK_SHADER_STAGE_TASK_BIT_NV;					break;
				case EShaderStages::Mesh :				flags |= VK_SHADER_STAGE_MESH_BIT_NV;					break;
				case EShaderStages::RayGen :			flags |= VK_SHADER_STAGE_RAYGEN_BIT_NV;					break;
				case EShaderStages::RayAnyHit :			flags |= VK_SHADER_STAGE_ANY_HIT_BIT_NV;				break;
				case EShaderStages::RayClosestHit :		flags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;			break;
				case EShaderStages::RayMiss :			flags |= VK_SHADER_STAGE_MISS_BIT_NV;					break;
				case EShaderStages::RayIntersection :	flags |= VK_SHADER_STAGE_INTERSECTION_BIT_NV;			break;
				case EShaderStages::RayCallable :		flags |= VK_SHADER_STAGE_CALLABLE_BIT_NV;				break;
				case EShaderStages::_Last :
				case EShaderStages::Unknown :
				case EShaderStages::AllGraphics :
				case EShaderStages::All :				// to shutup warnings	
				default :								RETURN_ERR( "unknown shader type!", VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM );
			}
			DISABLE_ENUM_CHECKS();
		}
		return flags;
	}

/*
=================================================
	DynamicState
=================================================
*/
	ND_ inline VkDynamicState  VEnumCast (EPipelineDynamicState value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EPipelineDynamicState::Viewport :			return VK_DYNAMIC_STATE_VIEWPORT;
			case EPipelineDynamicState::Scissor :			return VK_DYNAMIC_STATE_SCISSOR;
			//case EPipelineDynamicState::LineWidth :		return VK_DYNAMIC_STATE_LINE_WIDTH;
			//case EPipelineDynamicState::DepthBias :		return VK_DYNAMIC_STATE_DEPTH_BIAS;
			//case EPipelineDynamicState::BlendConstants :	return VK_DYNAMIC_STATE_BLEND_CONSTANTS;
			//case EPipelineDynamicState::DepthBounds :		return VK_DYNAMIC_STATE_DEPTH_BOUNDS;
			case EPipelineDynamicState::StencilCompareMask:	return VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK;
			case EPipelineDynamicState::StencilWriteMask :	return VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
			case EPipelineDynamicState::StencilReference :	return VK_DYNAMIC_STATE_STENCIL_REFERENCE;
			case EPipelineDynamicState::Unknown :
			case EPipelineDynamicState::All :
			case EPipelineDynamicState::_Last :				break;	// to shutup warnings
			//VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV
			//VK_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT
			//VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown dynamic state type!", VK_DYNAMIC_STATE_MAX_ENUM );
	}

/*
=================================================
	AttachmentLoadOp
=================================================
*/
	ND_ inline VkAttachmentLoadOp  VEnumCast (EAttachmentLoadOp value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EAttachmentLoadOp::Invalidate :	return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			case EAttachmentLoadOp::Load :			return VK_ATTACHMENT_LOAD_OP_LOAD;
			case EAttachmentLoadOp::Clear :			return VK_ATTACHMENT_LOAD_OP_CLEAR;
			case EAttachmentLoadOp::Unknown :		break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "invalid load op type", VK_ATTACHMENT_LOAD_OP_MAX_ENUM );
	}
	
/*
=================================================
	AttachmentStoreOp
=================================================
*/
	ND_ inline VkAttachmentStoreOp  VEnumCast (EAttachmentStoreOp value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EAttachmentStoreOp::Invalidate :	return VK_ATTACHMENT_STORE_OP_DONT_CARE;
			case EAttachmentStoreOp::Store :		return VK_ATTACHMENT_STORE_OP_STORE;
			case EAttachmentStoreOp::Unknown :		break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "invalid store op type", VK_ATTACHMENT_STORE_OP_MAX_ENUM );
	}

/*
=================================================
	CompareOp
=================================================
*/
	ND_ inline VkCompareOp  VEnumCast (ECompareOp value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case ECompareOp::Never :	return VK_COMPARE_OP_NEVER;
			case ECompareOp::Less :		return VK_COMPARE_OP_LESS;
			case ECompareOp::Equal :	return VK_COMPARE_OP_EQUAL;
			case ECompareOp::LEqual :	return VK_COMPARE_OP_LESS_OR_EQUAL;
			case ECompareOp::Greater :	return VK_COMPARE_OP_GREATER;
			case ECompareOp::NotEqual :	return VK_COMPARE_OP_NOT_EQUAL;
			case ECompareOp::GEqual :	return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case ECompareOp::Always :	return VK_COMPARE_OP_ALWAYS;
			case ECompareOp::Unknown :	break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "invalid compare op", VK_COMPARE_OP_MAX_ENUM );
	}

/*
=================================================
	StencilOp
=================================================
*/
	ND_ inline VkStencilOp  VEnumCast (EStencilOp value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EStencilOp::Keep		: return VK_STENCIL_OP_KEEP;
			case EStencilOp::Zero		: return VK_STENCIL_OP_ZERO;
			case EStencilOp::Replace	: return VK_STENCIL_OP_REPLACE;
			case EStencilOp::Incr		: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			case EStencilOp::IncrWrap	: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
			case EStencilOp::Decr		: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			case EStencilOp::DecrWrap	: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
			case EStencilOp::Invert		: return VK_STENCIL_OP_INVERT;
			case EStencilOp::Unknown	: break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "invalid stencil op", VK_STENCIL_OP_MAX_ENUM );
	}

/*
=================================================
	PolygonMode
=================================================
*/
	ND_ inline VkPolygonMode  VEnumCast (EPolygonMode value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EPolygonMode::Point	: return VK_POLYGON_MODE_POINT;
			case EPolygonMode::Line		: return VK_POLYGON_MODE_LINE;
			case EPolygonMode::Fill		: return VK_POLYGON_MODE_FILL;
			case EPolygonMode::Unknown	: break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "invalid polygon mode", VK_POLYGON_MODE_MAX_ENUM );
	}

/*
=================================================
	CullMode
=================================================
*/
	ND_ inline VkCullModeFlags  VEnumCast (ECullMode value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case ECullMode::None :			return VK_CULL_MODE_NONE;
			case ECullMode::Front :			return VK_CULL_MODE_FRONT_BIT;
			case ECullMode::Back :			return VK_CULL_MODE_BACK_BIT;
			case ECullMode::FontAndBack :	return VK_CULL_MODE_FRONT_BIT | VK_CULL_MODE_BACK_BIT;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown cull mode" );
	}

/*
=================================================
	Filter
=================================================
*/
	ND_ inline VkFilter  VEnumCast (EFilter value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EFilter::Nearest :	return VK_FILTER_NEAREST;
			case EFilter::Linear :	return VK_FILTER_LINEAR;
			case EFilter::Unknown :	break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown filter mode", VK_FILTER_MAX_ENUM );
	}

/*
=================================================
	SamplerMipmapMode
=================================================
*/
	ND_ inline VkSamplerMipmapMode  VEnumCast (EMipmapFilter value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EMipmapFilter::Nearest :	return VK_SAMPLER_MIPMAP_MODE_NEAREST;
			case EMipmapFilter::Linear :	return VK_SAMPLER_MIPMAP_MODE_LINEAR;
			case EMipmapFilter::Unknown :	break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown sampler mipmap mode", VK_SAMPLER_MIPMAP_MODE_MAX_ENUM );
	}

/*
=================================================
	SamplerAddressMode
=================================================
*/
	ND_ inline VkSamplerAddressMode  VEnumCast (EAddressMode value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EAddressMode::Repeat :				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case EAddressMode::MirrorRepeat :		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case EAddressMode::ClampToEdge :		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case EAddressMode::ClampToBorder :		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			case EAddressMode::MirrorClampToEdge :	return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
			case EAddressMode::Unknown :			break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown sampler address mode", VK_SAMPLER_ADDRESS_MODE_MAX_ENUM );
	}

/*
=================================================
	BorderColor
=================================================
*/
	ND_ inline VkBorderColor  VEnumCast (EBorderColor value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EBorderColor::FloatTransparentBlack :	return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			case EBorderColor::FloatOpaqueBlack :		return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			case EBorderColor::FloatOpaqueWhite :		return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			case EBorderColor::IntTransparentBlack :	return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
			case EBorderColor::IntOpaqueBlack :			return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			case EBorderColor::IntOpaqueWhite :			return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
			case EBorderColor::Unknown :				break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown border color type", VK_BORDER_COLOR_MAX_ENUM );
	}

/*
=================================================
	ImageViewType
=================================================
*/
	ND_ inline VkImageViewType  VEnumCast (EImage value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EImage::Tex1D			: return VK_IMAGE_VIEW_TYPE_1D;
			case EImage::Tex1DArray		: return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			case EImage::Tex2D			: return VK_IMAGE_VIEW_TYPE_2D;
			case EImage::Tex2DArray		: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			case EImage::TexCube		: return VK_IMAGE_VIEW_TYPE_CUBE;
			case EImage::TexCubeArray	: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			case EImage::Tex3D			: return VK_IMAGE_VIEW_TYPE_3D;
			case EImage::Tex2DMS		:
			case EImage::Tex2DMSArray	:
			case EImage::Buffer			:
			case EImage::Unknown		: break;	// not supported
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unsupported image view type", VK_IMAGE_VIEW_TYPE_MAX_ENUM );
	}

/*
=================================================
	ImageUsage
=================================================
*/
	ND_ inline VkImageUsageFlags  VEnumCast (EImageUsage values)
	{
		VkImageUsageFlags	flags = 0;

		for (EImageUsage t = EImageUsage(1 << 0); t < EImageUsage::_Last; t = EImageUsage(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;
			
			ENABLE_ENUM_CHECKS();
			switch ( t )
			{
				case EImageUsage::TransferSrc				: flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;				break;
				case EImageUsage::TransferDst				: flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;				break;
				case EImageUsage::Sampled					: flags |= VK_IMAGE_USAGE_SAMPLED_BIT;					break;
				case EImageUsage::Storage					: flags |= VK_IMAGE_USAGE_STORAGE_BIT;					break;
				case EImageUsage::ColorAttachment			: flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;			break;
				case EImageUsage::DepthStencilAttachment	: flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;	break;
				case EImageUsage::TransientAttachment		: flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;		break;
				case EImageUsage::InputAttachment			: flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;			break;
				case EImageUsage::ShadingRate				: flags |= VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV;	break;
				case EImageUsage::_Last						:
				case EImageUsage::Unknown					:
				case EImageUsage::Transfer					:
				case EImageUsage::All						: // to shutup warnings
				default										: RETURN_ERR( "invalid image usage type", VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM );
			}
			DISABLE_ENUM_CHECKS();
		}
		return flags;
	}

/*
=================================================
	ImageAspectFlags
=================================================
*/
	ND_ inline VkImageAspectFlags  VEnumCast (EImageAspect values)
	{
		VkImageAspectFlags	flags = 0;
		
		for (EImageAspect t = EImageAspect(1 << 0); t < EImageAspect::_Last; t = EImageAspect(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;
			
			ENABLE_ENUM_CHECKS();
			switch ( t )
			{
				case EImageAspect::Color		: flags |= VK_IMAGE_ASPECT_COLOR_BIT;		break;
				case EImageAspect::Depth		: flags |= VK_IMAGE_ASPECT_DEPTH_BIT;		break;
				case EImageAspect::Stencil		: flags |= VK_IMAGE_ASPECT_STENCIL_BIT;		break;
				case EImageAspect::Metadata		: flags |= VK_IMAGE_ASPECT_METADATA_BIT;	break;
				case EImageAspect::_Last		:
				case EImageAspect::Auto			:
				case EImageAspect::DepthStencil	:
				case EImageAspect::Unknown		: // to shutup warnings
				default							: RETURN_ERR( "invalid image aspect type", VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM );
			}
			DISABLE_ENUM_CHECKS();
		}
		return flags;
	}
	
/*
=================================================
	ImageAspectFlags
=================================================
*/
	ND_ inline VkImageAspectFlags  VEnumCast (EImageAspect values, EPixelFormat format)
	{
		if ( values == EImageAspect::Auto )
		{
			const bool	has_depth	= EPixelFormat_HasDepth( format );
			const bool	has_stencil	= EPixelFormat_HasStencil( format );

			//if ( has_depth and has_stencil )
			//	return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

			if ( has_depth )
				return VK_IMAGE_ASPECT_DEPTH_BIT;

			if ( has_stencil )
				return VK_IMAGE_ASPECT_STENCIL_BIT;

			return VK_IMAGE_ASPECT_COLOR_BIT;
		}

		return VEnumCast( values );
	}
	
/*
=================================================
	BufferUsage
=================================================
*/
	ND_ inline VkBufferUsageFlags  VEnumCast (EBufferUsage values)
	{
		VkBufferUsageFlags	result = 0;
		
		for (EBufferUsage t = EBufferUsage(1 << 0); t < EBufferUsage::_Last; t = EBufferUsage(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;
			
			ENABLE_ENUM_CHECKS();
			switch ( t )
			{
				case EBufferUsage::TransferSrc	:	result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;			break;
				case EBufferUsage::TransferDst	:	result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;			break;
				case EBufferUsage::UniformTexel	:	result |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;	break;
				case EBufferUsage::StorageTexel	:	result |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;	break;
				case EBufferUsage::Uniform		:	result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;		break;
				case EBufferUsage::Storage		:	result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;		break;
				case EBufferUsage::Index		:	result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;			break;
				case EBufferUsage::Vertex		:	result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;		break;
				case EBufferUsage::Indirect		:	result |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;		break;
				case EBufferUsage::RayTracing	:	result |= VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;		break;
				case EBufferUsage::_Last		:
				case EBufferUsage::Transfer		:
				case EBufferUsage::Unknown		:
				case EBufferUsage::All			:	// to shutup warnings
				default							:	RETURN_ERR( "invalid buffer usage", 0 );
			}
			DISABLE_ENUM_CHECKS();
		}
		return result;
	}

/*
=================================================
	IndexType
=================================================
*/
	ND_ inline VkIndexType  VEnumCast (EIndex value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EIndex::UShort		: return VK_INDEX_TYPE_UINT16;
			case EIndex::UInt		: return VK_INDEX_TYPE_UINT32;
			case EIndex::Unknown	: break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "invalid index type", VK_INDEX_TYPE_MAX_ENUM );
	}

/*
=================================================
	PrimitiveTopology
=================================================
*/
	ND_ inline VkPrimitiveTopology  VEnumCast (EPrimitive value)
	{
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			case EPrimitive::Point					: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			case EPrimitive::LineList				: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			case EPrimitive::LineStrip				: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
			case EPrimitive::LineListAdjacency		: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
			case EPrimitive::LineStripAdjacency		: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
			case EPrimitive::TriangleList			: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			case EPrimitive::TriangleStrip			: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			case EPrimitive::TriangleFan			: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
			case EPrimitive::TriangleListAdjacency	: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
			case EPrimitive::TriangleStripAdjacency	: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
			case EPrimitive::Patch					: return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			case EPrimitive::Unknown				:
			case EPrimitive::_Count					: break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "invalid primitive type", VK_PRIMITIVE_TOPOLOGY_MAX_ENUM );
	}
	
/*
=================================================
	GeometryFlags
=================================================
*/
	ND_ inline VkGeometryFlagsNV  VEnumCast (ERayTracingGeometryFlags values)
	{
		VkGeometryFlagsNV	result = 0;
		
		for (ERayTracingGeometryFlags t = ERayTracingGeometryFlags(1 << 0);
			 t < ERayTracingGeometryFlags::_Last;
			 t = ERayTracingGeometryFlags(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;
			
			ENABLE_ENUM_CHECKS();
			switch ( t )
			{
				case ERayTracingGeometryFlags::Opaque						: result |= VK_GEOMETRY_OPAQUE_BIT_NV;							break;
				case ERayTracingGeometryFlags::NoDuplicateAnyHitInvocation	: result |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_NV;	break;
				case ERayTracingGeometryFlags::_Last						:
				case ERayTracingGeometryFlags::Unknown						:
				default														: RETURN_ERR( "invalid geometry flags", 0 );
			}
			DISABLE_ENUM_CHECKS();
		}
		return result;
	}
	
/*
=================================================
	GeometryInstanceFlags
=================================================
*/
	ND_ inline VkGeometryInstanceFlagsNV   VEnumCast (ERayTracingInstanceFlags values)
	{
		VkGeometryInstanceFlagsNV	result = 0;
		
		for (ERayTracingInstanceFlags t = ERayTracingInstanceFlags(1 << 0);
			 t < ERayTracingInstanceFlags::_Last;
			 t = ERayTracingInstanceFlags(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;
		
			ENABLE_ENUM_CHECKS();
			switch ( t )
			{
				case ERayTracingInstanceFlags::TriangleCullDisable	: return VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
				case ERayTracingInstanceFlags::TriangleFrontCCW		: return VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_NV;
				case ERayTracingInstanceFlags::ForceOpaque			: return VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_NV;
				case ERayTracingInstanceFlags::ForceNonOpaque		: return VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_NV;
				case ERayTracingInstanceFlags::_Last				:
				case ERayTracingInstanceFlags::Unknown				:
				default												: RETURN_ERR( "invalid geometry instance flags", 0 );
			}
			DISABLE_ENUM_CHECKS();
		}
		return result;
	}
	
/*
=================================================
	AccelerationStructureFlags
=================================================
*/
	ND_ inline VkBuildAccelerationStructureFlagsNV   VEnumCast (ERayTracingFlags values)
	{
		VkBuildAccelerationStructureFlagsNV	result = 0;
		
		for (ERayTracingFlags t = ERayTracingFlags(1 << 0);
			 t < ERayTracingFlags::_Last;
			 t = ERayTracingFlags(uint(t) << 1)) 
		{
			if ( not EnumEq( values, t ) )
				continue;
		
			ENABLE_ENUM_CHECKS();
			switch ( t )
			{
				case ERayTracingFlags::AllowUpdate		: result |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV;		break;
				case ERayTracingFlags::AllowCompaction	: result |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_NV;	break;
				case ERayTracingFlags::PreferFastTrace	: result |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;	break;
				case ERayTracingFlags::PreferFastBuild	: result |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_NV;	break;
				case ERayTracingFlags::LowMemory		: result |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_NV;			break;
				case ERayTracingFlags::_Last			:
				case ERayTracingFlags::Unknown			:
				default									: RETURN_ERR( "invalid flags", 0 );
			}
			DISABLE_ENUM_CHECKS();
		}
		return result;
	}

/*
=================================================
	EResourceState_ToPipelineStages
=================================================
*/
	ND_ inline VkPipelineStageFlags  EResourceState_ToPipelineStages (EResourceState value)
	{
		switch ( value & EResourceState::_AccessMask )
		{								  
			case EResourceState::Unknown :						return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			case EResourceState::_Access_InputAttachment :		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;	// TODO: check
			//case EResourceState::_Access_TransientAttachment :		
			case EResourceState::_Access_Transfer :				return VK_PIPELINE_STAGE_TRANSFER_BIT;
			case EResourceState::_Access_ColorAttachment :		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			case EResourceState::_Access_Present :				return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			case EResourceState::_Access_IndirectBuffer :		return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
			case EResourceState::_Access_Host :					return VK_PIPELINE_STAGE_HOST_BIT;
			case EResourceState::_Access_IndexBuffer :			return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			case EResourceState::_Access_VertexBuffer :			return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			case EResourceState::_Access_ConditionalRendering :	return VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
			case EResourceState::_Access_CommandProcess :		return VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX;
			case EResourceState::_Access_ShadingRateImage :		return VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV;
			case EResourceState::_Access_BuildRayTracingAS :
			case EResourceState::_Access_RTASBuildingBuffer :	return VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV;

			case EResourceState::_Access_ShaderStorage :
			case EResourceState::_Access_Uniform :
			case EResourceState::_Access_ShaderSample : {
				ASSERT( EnumAny( value, EResourceState::_ShaderMask ));
				VkPipelineStageFlags	result = 0;
				if ( EnumEq( value, EResourceState::_VertexShader ) )			result |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
				if ( EnumEq( value, EResourceState::_TessControlShader ) )		result |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
				if ( EnumEq( value, EResourceState::_TessEvaluationShader ) )	result |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
				if ( EnumEq( value, EResourceState::_GeometryShader ) )			result |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
				if ( EnumEq( value, EResourceState::_FragmentShader ) )			result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				if ( EnumEq( value, EResourceState::_ComputeShader ) )			result |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				if ( EnumEq( value, EResourceState::_MeshTaskShader ) )			result |= VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV;
				if ( EnumEq( value, EResourceState::_MeshShader ) )				result |= VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;
				if ( EnumEq( value, EResourceState::_RayTracingShader ) )		result |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV;
				return result;
			}

			case EResourceState::_Access_DepthStencilAttachment : {
				ASSERT( EnumAny( value, EResourceState::EarlyFragmentTests | EResourceState::LateFragmentTests ));
				VkPipelineStageFlags	result = 0;
				if ( EnumEq( value, EResourceState::EarlyFragmentTests ) )		result |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				if ( EnumEq( value, EResourceState::LateFragmentTests ) )		result |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				return result;
			}
		}
		RETURN_ERR( "unknown resource state!" );
	}
	
/*
=================================================
	EResourceState_ToAccess
=================================================
*/
	ND_ inline VkAccessFlags  EResourceState_ToAccess (EResourceState value)
	{
		switch ( value & EResourceState::_StateMask )
		{
			case EResourceState::Unknown :							return 0;
			case EResourceState::UniformRead :						return VK_ACCESS_UNIFORM_READ_BIT;
			case EResourceState::ShaderSample :
			case EResourceState::ShaderRead :						return VK_ACCESS_SHADER_READ_BIT;
			case EResourceState::ShaderWrite :						return VK_ACCESS_SHADER_WRITE_BIT;
			case EResourceState::ShaderReadWrite :					return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			case EResourceState::InputAttachment :					return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
			case EResourceState::TransferSrc :						return VK_ACCESS_TRANSFER_READ_BIT;
			case EResourceState::TransferDst :						return VK_ACCESS_TRANSFER_WRITE_BIT;
			case EResourceState::ColorAttachmentRead :				return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			case EResourceState::ColorAttachmentWrite :				return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			case EResourceState::ColorAttachmentReadWrite :			return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			case EResourceState::DepthStencilAttachmentRead :		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			case EResourceState::DepthStencilAttachmentWrite :		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			case EResourceState::DepthStencilAttachmentReadWrite :	return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			case EResourceState::HostRead :							return VK_ACCESS_HOST_READ_BIT;
			case EResourceState::HostWrite :						return VK_ACCESS_HOST_WRITE_BIT;
			case EResourceState::HostReadWrite :					return VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
			case EResourceState::IndirectBuffer :					return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
			case EResourceState::IndexBuffer :						return VK_ACCESS_INDEX_READ_BIT;
			case EResourceState::VertexBuffer :						return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			//case EResourceState::TransientAttachment
			case EResourceState::PresentImage :						return VK_ACCESS_MEMORY_READ_BIT;
			case EResourceState::BuildRayTracingStructRead :		return VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
			case EResourceState::BuildRayTracingStructWrite :		return VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
			case EResourceState::BuildRayTracingStructReadWrite :	return VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
			case EResourceState::RTASBuildingBufferRead :
			case EResourceState::RTASBuildingBufferReadWrite :		return 0;	// ceche invalidation is not needed for buffers
		}
		RETURN_ERR( "unknown resource state!" );
	}
	
/*
=================================================
	EResourceState_ToImageLayout
=================================================
*/
	ND_ inline VkImageLayout  EResourceState_ToImageLayout (EResourceState value, VkImageAspectFlags aspect)
	{
		switch ( value & EResourceState::_StateMask )
		{
			case EResourceState::Unknown :							return VK_IMAGE_LAYOUT_UNDEFINED;

			case EResourceState::ShaderSample :
			case EResourceState::InputAttachment :					return EnumEq( aspect, VK_IMAGE_ASPECT_COLOR_BIT ) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :
																														 VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			case EResourceState::ShaderRead :
			case EResourceState::ShaderWrite :
			case EResourceState::ShaderReadWrite :					return VK_IMAGE_LAYOUT_GENERAL;

			case EResourceState::TransferSrc :						return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			case EResourceState::TransferDst :						return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			case EResourceState::TransientAttachment :
			case EResourceState::ColorAttachmentRead :
			case EResourceState::ColorAttachmentWrite :
			case EResourceState::ColorAttachmentReadWrite :			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			case EResourceState::DepthStencilAttachmentRead :
			case EResourceState::DepthStencilAttachmentWrite :
			case EResourceState::DepthStencilAttachmentReadWrite :	return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;	// TODO: use other layouts

			case EResourceState::HostRead :
			case EResourceState::HostWrite :
			case EResourceState::HostReadWrite :					return VK_IMAGE_LAYOUT_GENERAL;

			case EResourceState::PresentImage :						return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			//case EResourceState::SharedPresentImage :				return VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;
		}
		RETURN_ERR( "unsupported state for image!", VK_IMAGE_LAYOUT_MAX_ENUM );
	}

/*
=================================================
	FG_PRIVATE_VKPIXELFORMATS
=================================================
*/
#	define FG_PRIVATE_VKPIXELFORMATS( _builder_ ) \
		_builder_( RGBA16_SNorm,		VK_FORMAT_R16G16B16A16_SNORM ) \
		_builder_( RGBA8_SNorm,			VK_FORMAT_R8G8B8A8_SNORM ) \
		_builder_( RGB16_SNorm,			VK_FORMAT_R16G16B16_SNORM ) \
		_builder_( RGB8_SNorm,			VK_FORMAT_R8G8B8_SNORM ) \
		_builder_( RG16_SNorm,			VK_FORMAT_R16G16_SNORM ) \
		_builder_( RG8_SNorm,			VK_FORMAT_R8G8_SNORM ) \
		_builder_( R16_SNorm,			VK_FORMAT_R16_SNORM ) \
		_builder_( R8_SNorm,			VK_FORMAT_R8_SNORM ) \
		_builder_( RGBA16_UNorm,		VK_FORMAT_R16G16B16A16_UNORM ) \
		_builder_( RGBA8_UNorm,			VK_FORMAT_R8G8B8A8_UNORM ) \
		_builder_( RGB16_UNorm,			VK_FORMAT_R16G16B16_UNORM ) \
		_builder_( RGB8_UNorm,			VK_FORMAT_R8G8B8_UNORM ) \
		_builder_( RG16_UNorm,			VK_FORMAT_R16G16_UNORM ) \
		_builder_( RG8_UNorm,			VK_FORMAT_R8G8_UNORM ) \
		_builder_( R16_UNorm,			VK_FORMAT_R16_UNORM ) \
		_builder_( R8_UNorm,			VK_FORMAT_R8_UNORM ) \
		_builder_( RGB10_A2_UNorm,		VK_FORMAT_A2B10G10R10_UNORM_PACK32 ) \
		_builder_( RGBA4_UNorm,			VK_FORMAT_R4G4B4A4_UNORM_PACK16 ) \
		_builder_( RGB5_A1_UNorm,		VK_FORMAT_R5G5B5A1_UNORM_PACK16 ) \
		_builder_( RGB_5_6_5_UNorm,		VK_FORMAT_R5G6B5_UNORM_PACK16 ) \
		_builder_( BGR8_UNorm,			VK_FORMAT_B8G8R8_UNORM ) \
		_builder_( BGRA8_UNorm,			VK_FORMAT_B8G8R8A8_UNORM ) \
		_builder_( sRGB8,				VK_FORMAT_R8G8B8_SRGB ) \
		_builder_( sRGB8_A8,			VK_FORMAT_R8G8B8A8_SRGB ) \
		_builder_( sBGR8,				VK_FORMAT_B8G8R8_SRGB ) \
		_builder_( sBGR8_A8,			VK_FORMAT_B8G8R8A8_SRGB ) \
		_builder_( R8I,					VK_FORMAT_R8_SINT ) \
		_builder_( RG8I,				VK_FORMAT_R8G8_SINT ) \
		_builder_( RGB8I,				VK_FORMAT_R8G8B8_SINT ) \
		_builder_( RGBA8I,				VK_FORMAT_R8G8B8A8_SINT ) \
		_builder_( R16I,				VK_FORMAT_R16_SINT ) \
		_builder_( RG16I,				VK_FORMAT_R16G16_SINT ) \
		_builder_( RGB16I,				VK_FORMAT_R16G16B16_SINT ) \
		_builder_( RGBA16I,				VK_FORMAT_R16G16B16A16_SINT ) \
		_builder_( R32I,				VK_FORMAT_R32_SINT ) \
		_builder_( RG32I,				VK_FORMAT_R32G32_SINT ) \
		_builder_( RGB32I,				VK_FORMAT_R32G32B32_SINT ) \
		_builder_( RGBA32I,				VK_FORMAT_R32G32B32A32_UINT ) \
		_builder_( R8U,					VK_FORMAT_R8_UINT ) \
		_builder_( RG8U,				VK_FORMAT_R8G8_UINT ) \
		_builder_( RGB8U,				VK_FORMAT_R8G8B8_UINT ) \
		_builder_( RGBA8U,				VK_FORMAT_R8G8B8A8_UINT ) \
		_builder_( R16U,				VK_FORMAT_R16_UINT ) \
		_builder_( RG16U,				VK_FORMAT_R16G16_UINT ) \
		_builder_( RGB16U,				VK_FORMAT_R16G16B16_UINT ) \
		_builder_( RGBA16U,				VK_FORMAT_R16G16B16A16_UINT ) \
		_builder_( R32U,				VK_FORMAT_R32_UINT ) \
		_builder_( RG32U,				VK_FORMAT_R32G32_UINT ) \
		_builder_( RGB32U,				VK_FORMAT_R32G32B32_UINT ) \
		_builder_( RGBA32U,				VK_FORMAT_R32G32B32A32_SINT ) \
		_builder_( RGB10_A2U,			VK_FORMAT_A2B10G10R10_UINT_PACK32 ) \
		_builder_( R16F,				VK_FORMAT_R16_SFLOAT ) \
		_builder_( RG16F,				VK_FORMAT_R16G16_SFLOAT ) \
		_builder_( RGB16F,				VK_FORMAT_R16G16B16_SFLOAT ) \
		_builder_( RGBA16F,				VK_FORMAT_R16G16B16A16_SFLOAT ) \
		_builder_( R32F,				VK_FORMAT_R32_SFLOAT ) \
		_builder_( RG32F,				VK_FORMAT_R32G32_SFLOAT ) \
		_builder_( RGB32F,				VK_FORMAT_R32G32B32_SFLOAT ) \
		_builder_( RGBA32F,				VK_FORMAT_R32G32B32A32_SFLOAT ) \
		_builder_( RGB_11_11_10F,		VK_FORMAT_B10G11R11_UFLOAT_PACK32 ) \
		_builder_( Depth16,				VK_FORMAT_D16_UNORM ) \
		_builder_( Depth24,				VK_FORMAT_X8_D24_UNORM_PACK32 ) \
		/*_builder_( Depth32,			VK_FORMAT_D32_SFLOAT ) */ \
		_builder_( Depth32F,			VK_FORMAT_D32_SFLOAT ) \
		_builder_( Depth16_Stencil8,	VK_FORMAT_D16_UNORM_S8_UINT ) \
		_builder_( Depth24_Stencil8,	VK_FORMAT_D24_UNORM_S8_UINT ) \
		_builder_( Depth32F_Stencil8,	VK_FORMAT_D32_SFLOAT_S8_UINT ) \
		_builder_( BC1_RGB8_UNorm,		VK_FORMAT_BC1_RGB_UNORM_BLOCK ) \
		_builder_( BC1_RGB8_A1_UNorm,	VK_FORMAT_BC1_RGBA_UNORM_BLOCK ) \
		_builder_( BC2_RGBA8_UNorm,		VK_FORMAT_BC2_UNORM_BLOCK ) \
		_builder_( BC3_RGBA8_UNorm,		VK_FORMAT_BC3_UNORM_BLOCK ) \
		_builder_( BC3_sRGB,			VK_FORMAT_BC3_SRGB_BLOCK ) \
		_builder_( BC4_RED8_SNorm,		VK_FORMAT_BC4_SNORM_BLOCK ) \
		_builder_( BC4_RED8_UNorm,		VK_FORMAT_BC4_UNORM_BLOCK ) \
		_builder_( BC5_RG8_SNorm,		VK_FORMAT_BC5_SNORM_BLOCK ) \
		_builder_( BC5_RG8_UNorm,		VK_FORMAT_BC5_UNORM_BLOCK ) \
		_builder_( BC7_RGBA8_UNorm,		VK_FORMAT_BC7_UNORM_BLOCK ) \
		_builder_( BC7_SRGB8_A8,		VK_FORMAT_BC7_SRGB_BLOCK ) \
		_builder_( BC6H_RGB16F,			VK_FORMAT_BC6H_SFLOAT_BLOCK ) \
		_builder_( BC6H_RGB16UF,		VK_FORMAT_BC6H_UFLOAT_BLOCK ) \
		_builder_( ETC2_RGB8_UNorm,		VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK ) \
		_builder_( ECT2_SRGB8,			VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK ) \
		_builder_( ETC2_RGB8_A1_UNorm,	VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK ) \
		_builder_( ETC2_SRGB8_A1,		VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK ) \
		_builder_( ETC2_RGBA8_UNorm,	VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK ) \
		_builder_( ETC2_SRGB8_A8,		VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK ) \
		_builder_( EAC_R11_SNorm,		VK_FORMAT_EAC_R11_SNORM_BLOCK ) \
		_builder_( EAC_R11_UNorm,		VK_FORMAT_EAC_R11_UNORM_BLOCK ) \
		_builder_( EAC_RG11_SNorm,		VK_FORMAT_EAC_R11G11_SNORM_BLOCK ) \
		_builder_( EAC_RG11_UNorm,		VK_FORMAT_EAC_R11G11_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_4x4,		VK_FORMAT_ASTC_4x4_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_5x4,		VK_FORMAT_ASTC_5x4_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_5x5,		VK_FORMAT_ASTC_5x5_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_6x5,		VK_FORMAT_ASTC_6x5_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_6x6,		VK_FORMAT_ASTC_6x6_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_8x5,		VK_FORMAT_ASTC_8x5_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_8x6,		VK_FORMAT_ASTC_8x6_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_8x8,		VK_FORMAT_ASTC_8x8_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_10x5,		VK_FORMAT_ASTC_10x5_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_10x6,		VK_FORMAT_ASTC_10x6_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_10x8,		VK_FORMAT_ASTC_10x8_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_10x10,		VK_FORMAT_ASTC_10x10_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_12x10,		VK_FORMAT_ASTC_12x10_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_12x12,		VK_FORMAT_ASTC_12x12_UNORM_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_4x4,	VK_FORMAT_ASTC_4x4_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_5x4,	VK_FORMAT_ASTC_5x4_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_5x5,	VK_FORMAT_ASTC_5x5_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_6x5,	VK_FORMAT_ASTC_6x5_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_6x6,	VK_FORMAT_ASTC_6x6_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_8x5,	VK_FORMAT_ASTC_8x5_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_8x6,	VK_FORMAT_ASTC_8x6_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_8x8,	VK_FORMAT_ASTC_8x8_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_10x5,	VK_FORMAT_ASTC_10x5_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_10x6,	VK_FORMAT_ASTC_10x6_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_10x8,	VK_FORMAT_ASTC_10x8_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_10x10,	VK_FORMAT_ASTC_10x10_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_12x10,	VK_FORMAT_ASTC_12x10_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_12x12,	VK_FORMAT_ASTC_12x12_SRGB_BLOCK )
			
/*
=================================================
	Format
=================================================
*/
	ND_ inline VkFormat  VEnumCast (EPixelFormat value)
	{
#		define FMT_BUILDER( _engineFmt_, _vkFormat_ ) \
			case EPixelFormat::_engineFmt_ : return _vkFormat_;
		
		ENABLE_ENUM_CHECKS();
		switch ( value )
		{
			FG_PRIVATE_VKPIXELFORMATS( FMT_BUILDER )
			case EPixelFormat::_Count :
			case EPixelFormat::Unknown :	break;
		}
		DISABLE_ENUM_CHECKS();

#		undef FMT_BUILDER

		RETURN_ERR( "invalid pixel format", VK_FORMAT_MAX_ENUM );
	}

}	// FG

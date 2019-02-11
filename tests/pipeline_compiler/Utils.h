// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "pipeline_compiler/VPipelineCompiler.h"
#include "framegraph/Shared/EnumUtils.h"
using namespace FG;

#define TEST	CHECK_FATAL


/*
=================================================
	FindVertexInput
=================================================
*/
inline const GraphicsPipelineDesc::VertexAttrib*  FindVertexInput (const GraphicsPipelineDesc &ppln, const VertexID &id)
{
	for (auto& attr : ppln._vertexAttribs)
	{
		if ( attr.id == id )
			return &attr;
	}
	return null;
}

/*
=================================================
	TestVertexInput
=================================================
*/
inline bool  TestVertexInput (const GraphicsPipelineDesc &ppln, const VertexID &id, EVertexType type, uint index)
{
	auto	ptr = FindVertexInput( ppln, id );
	if ( not ptr )
		return false;

	return	ptr->index	== index	and
			ptr->type	== type;
}

/*
=================================================
	FindFragmentOutput
=================================================
*/
inline const GraphicsPipelineDesc::FragmentOutput*  FindFragmentOutput (const GraphicsPipelineDesc &ppln, const RenderTargetID &id)
{
	for (auto& frag : ppln._fragmentOutput)
	{
		if ( frag.id == id )
			return &frag;
	}
	return null;
}

/*
=================================================
	TestFragmentOutput
=================================================
*/
inline bool  TestFragmentOutput (const GraphicsPipelineDesc &ppln, const RenderTargetID &id, EFragOutput type, uint index)
{
	auto	ptr = FindFragmentOutput( ppln, id );
	if ( not ptr )
		return false;

	return	ptr->index	== index	and
			ptr->type	== type;
}

/*
=================================================
	FindDescriptorSet
=================================================
*/
inline const PipelineDescription::DescriptorSet*  FindDescriptorSet (const PipelineDescription &ppln, const DescriptorSetID &id)
{
	for (auto& ds : ppln._pipelineLayout.descriptorSets)
	{
		if ( ds.id == id )
			return &ds;
	}
	return null;
}

/*
=================================================
	FindUniform
=================================================
*/
template <typename T>
inline Pair<const PipelineDescription::Uniform*, const T*>
	FindUniform (const PipelineDescription::DescriptorSet &ds, const UniformID &id)
{
	for (auto& un : *ds.uniforms)
	{
		if ( un.first == id )
		{
			if ( auto* ptr = UnionGetIf<T>( &un.second.data ) )
			{
				return {&un.second, ptr};
			}
			break;
		}
	}
	return {null, null};
}

/*
=================================================
	TestTextureUniform
=================================================
*/
inline bool TestTextureUniform (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
								EImage textureType, uint bindingIndex, EShaderStages stageFlags, uint arraySize = 1)
{
	auto[un, ptr] = FindUniform< PipelineDescription::Texture >( ds, id );
	if ( not ptr )
		return false;

	EResourceState	state = EResourceState::ShaderSample | EResourceState_FromShaders( stageFlags );

	return	ptr->textureType		== textureType	and
			ptr->state				== state		and
			un->index.VKBinding()	== bindingIndex	and
			un->stageFlags			== stageFlags	and
			un->arraySize			== arraySize;
}

/*
=================================================
	TestImageUniform
=================================================
*/
inline bool TestImageUniform (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
							  EImage imageType, EPixelFormat format, EShaderAccess access,
							  uint bindingIndex, EShaderStages stageFlags, uint arraySize = 1)
{
	auto[un, ptr] = FindUniform< PipelineDescription::Image >( ds, id );
	if ( not ptr )
		return false;

	EResourceState	state = EResourceState_FromShaderAccess( access ) | EResourceState_FromShaders( stageFlags );

	return	ptr->imageType			== imageType	and
			ptr->format				== format		and
			ptr->state				== state		and
			un->index.VKBinding()	== bindingIndex	and
			un->stageFlags			== stageFlags	and
			un->arraySize			== arraySize;
}

/*
=================================================
	TestSamplerUniform
=================================================
*/
inline bool TestSamplerUniform (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
								uint bindingIndex, EShaderStages stageFlags, uint arraySize = 1)
{
	auto[un, ptr] = FindUniform< PipelineDescription::Sampler >( ds, id );

	return	un->index.VKBinding()	== bindingIndex	and
			un->stageFlags			== stageFlags	and
			un->arraySize			== arraySize;
}

/*
=================================================
	TestSubpassInputUniform
=================================================
*/
inline bool TestSubpassInputUniform (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
									 uint attachmentIndex, bool isMultisample,
									 uint bindingIndex, EShaderStages stageFlags, uint arraySize = 1)
{
	auto[un, ptr] = FindUniform< PipelineDescription::SubpassInput >( ds, id );
	if ( not ptr )
		return false;
	
	EResourceState	state = EResourceState::InputAttachment | EResourceState_FromShaders( stageFlags );

	return	ptr->attachmentIndex	== attachmentIndex	and
			ptr->isMultisample		== isMultisample	and
			ptr->state				== state			and
			un->index.VKBinding()	== bindingIndex		and
			un->stageFlags			== stageFlags		and
			un->arraySize			== arraySize;
}

/*
=================================================
	TestUniformBuffer
=================================================
*/
inline bool TestUniformBuffer (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
								BytesU size, uint bindingIndex, EShaderStages stageFlags, uint arraySize = 1,
								uint dynamicOffsetIndex = PipelineDescription::STATIC_OFFSET)
{
	auto[un, ptr] = FindUniform< PipelineDescription::UniformBuffer >( ds, id );
	if ( not ptr )
		return false;
	
	EResourceState	state = EResourceState::UniformRead | EResourceState_FromShaders( stageFlags ) |
							(dynamicOffsetIndex == PipelineDescription::STATIC_OFFSET ? EResourceState::Unknown : EResourceState::_BufferDynamicOffset);

	return	ptr->size				== size					and
			ptr->state				== state				and
			ptr->dynamicOffsetIndex	== dynamicOffsetIndex	and
			un->index.VKBinding()	== bindingIndex			and
			un->stageFlags			== stageFlags			and
			un->arraySize			== arraySize;
}

/*
=================================================
	TestStorageBuffer
=================================================
*/
inline bool TestStorageBuffer (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
								BytesU staticSize, BytesU arrayStride, EShaderAccess access,
								uint bindingIndex, EShaderStages stageFlags, uint arraySize = 1,
								uint dynamicOffsetIndex = PipelineDescription::STATIC_OFFSET)
{
	auto[un, ptr] = FindUniform< PipelineDescription::StorageBuffer >( ds, id );
	if ( not ptr )
		return false;
	
	EResourceState	state = EResourceState_FromShaderAccess( access ) | EResourceState_FromShaders( stageFlags ) |
							(dynamicOffsetIndex == PipelineDescription::STATIC_OFFSET ? EResourceState::Unknown : EResourceState::_BufferDynamicOffset);

	return	ptr->staticSize			== staticSize			and
			ptr->arrayStride		== arrayStride			and
			ptr->state				== state				and
			ptr->dynamicOffsetIndex	== dynamicOffsetIndex	and
			un->index.VKBinding()	== bindingIndex			and
			un->stageFlags			== stageFlags			and
			un->arraySize			== arraySize;
}

/*
=================================================
	TestRayTracingScene
=================================================
*/
inline bool TestRayTracingScene (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
								 uint bindingIndex, EShaderStages stageFlags, uint arraySize = 1)
{
	auto[un, ptr] = FindUniform< PipelineDescription::RayTracingScene >( ds, id );
	if ( not ptr )
		return false;

	EResourceState	state = EResourceState::_RayTracingShader | EResourceState::ShaderRead;

	return	ptr->state				== state		and
			un->index.VKBinding()	== bindingIndex	and
			un->stageFlags			== stageFlags	and
			un->arraySize			== arraySize;
}

/*
=================================================
	TestSpecializationConstant
=================================================
*/
inline bool TestSpecializationConstant (const PipelineDescription::Shader &shader, const SpecializationID &id, uint index)
{
	auto	iter = shader.specConstants.find( id );

	if ( iter == shader.specConstants.end() )
		return false;

	return	iter->second == index;
}

/*
=================================================
	TestPushConstant
=================================================
*/
inline bool TestPushConstant (const PipelineDescription &desc, const PushConstantID &id, EShaderStages stageFlags, BytesU offset, BytesU size)
{
	auto	iter = desc._pipelineLayout.pushConstants.find( id );

	if ( iter == desc._pipelineLayout.pushConstants.end() )
		return false;

	return	iter->second.offset		== uint16_t(offset)	and
			iter->second.size		== uint16_t(size)	and
			iter->second.stageFlags	== stageFlags;
}

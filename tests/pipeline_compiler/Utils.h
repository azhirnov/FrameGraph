// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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
inline const T*  FindUniform (const PipelineDescription::DescriptorSet &ds, const UniformID &id)
{
	for (auto& un : *ds.uniforms)
	{
		if ( un.first == id )
		{
			if ( auto* tex = std::get_if<T>( &un.second.data ) )
			{
				return tex;
			}
			break;
		}
	}
	return null;
}

/*
=================================================
	FindUniform
=================================================
*/
template <typename T>
inline const T*  FindUniform (const PipelineDescription::DescriptorSet &ds, const UniformID &id, uint bindingIndex, EShaderStages stageFlags)
{
	for (auto& un : *ds.uniforms)
	{
		if ( un.first == id )
		{
			if ( un.second.index.VKBinding() == bindingIndex and
				 un.second.stageFlags == stageFlags )
			{
				if ( auto* tex = std::get_if<T>( &un.second.data ) )
				{
					return tex;
				}
			}
			break;
		}
	}
	return null;
}

/*
=================================================
	TestTextureUniform
=================================================
*/
inline bool TestTextureUniform (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
								EImage textureType, uint bindingIndex, EShaderStages stageFlags)
{
	auto	ptr = FindUniform< PipelineDescription::Texture >( ds, id, bindingIndex, stageFlags );
	if ( not ptr )
		return false;

	EResourceState	state = EResourceState::ShaderSample | EResourceState_FromShaders( stageFlags );

	return	ptr->textureType	== textureType	and
			ptr->state			== state;
}

/*
=================================================
	TestImageUniform
=================================================
*/
inline bool TestImageUniform (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
							  EImage imageType, EPixelFormat format, EShaderAccess access,
							  uint bindingIndex, EShaderStages stageFlags)
{
	auto	ptr = FindUniform< PipelineDescription::Image >( ds, id, bindingIndex, stageFlags );
	if ( not ptr )
		return false;

	EResourceState	state = EResourceState_FromShaderAccess( access ) | EResourceState_FromShaders( stageFlags );

	return	ptr->imageType	== imageType	and
			ptr->format		== format		and
			ptr->state		== state;
}

/*
=================================================
	TestSamplerUniform
=================================================
*/
inline bool TestSamplerUniform (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
								uint bindingIndex, EShaderStages stageFlags)
{
	auto	ptr = FindUniform< PipelineDescription::Sampler >( ds, id, bindingIndex, stageFlags );
	return !!ptr;
}

/*
=================================================
	TestSubpassInputUniform
=================================================
*/
inline bool TestSubpassInputUniform (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
									 uint attachmentIndex, bool isMultisample,
									 uint bindingIndex, EShaderStages stageFlags)
{
	auto	ptr = FindUniform< PipelineDescription::SubpassInput >( ds, id, bindingIndex, stageFlags );
	if ( not ptr )
		return false;
	
	EResourceState	state = EResourceState::InputAttachment | EResourceState_FromShaders( stageFlags );

	return	ptr->attachmentIndex	== attachmentIndex	and
			ptr->isMultisample		== isMultisample	and
			ptr->state				== state;
}

/*
=================================================
	TestUniformBuffer
=================================================
*/
inline bool TestUniformBuffer (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
								BytesU size, uint bindingIndex, EShaderStages stageFlags)
{
	auto	ptr = FindUniform< PipelineDescription::UniformBuffer >( ds, id, bindingIndex, stageFlags );
	if ( not ptr )
		return false;
	
	EResourceState	state = EResourceState::UniformRead | EResourceState_FromShaders( stageFlags );

	return	ptr->size	== size		and
			ptr->state	== state;
}

/*
=================================================
	TestStorageBuffer
=================================================
*/
inline bool TestStorageBuffer (const PipelineDescription::DescriptorSet &ds, const UniformID &id,
								BytesU staticSize, BytesU arrayStride, EShaderAccess access,
								uint bindingIndex, EShaderStages stageFlags)
{
	auto	ptr = FindUniform< PipelineDescription::StorageBuffer >( ds, id, bindingIndex, stageFlags );
	if ( not ptr )
		return false;
	
	EResourceState	state = EResourceState_FromShaderAccess( access ) | EResourceState_FromShaders( stageFlags );

	return	ptr->staticSize		== staticSize	and
			ptr->arrayStride	== arrayStride	and
			ptr->state			== state;
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

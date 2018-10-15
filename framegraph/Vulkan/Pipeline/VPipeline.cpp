// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VPipeline.h"
#include "VPipelineResources.h"
#include "VFrameGraph.h"

namespace FG
{

/*
=================================================
	FragmentOutputInstance
=================================================
*/
	VGraphicsPipeline::FragmentOutputInstance::FragmentOutputInstance (ArrayView<FragmentOutput> values) :
		_values{ values }
	{
		// sort by index
		std::sort( _values.begin(), _values.end(), [] (auto& lhs, auto &rhs) { return lhs.index < rhs.index; });

		// calc hash
		_hash = HashOf( _values.size() );

		for (auto& fo : _values)
		{
			_hash << HashOf( fo.id );
			_hash << HashOf( fo.index );
			_hash << HashOf( fo.type );
		}
	}
			
/*
=================================================
	FragmentOutputInstance::operator ==
=================================================
*/
	bool VGraphicsPipeline::FragmentOutputInstance::operator == (const FragmentOutputInstance &rhs) const
	{
		if ( _hash != rhs._hash )
			return false;

		return _values == rhs._values;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	VkVertexInputAttributeDescription::operator ==
=================================================
*/
	ND_ inline bool operator == (const VkVertexInputAttributeDescription &lhs, const VkVertexInputAttributeDescription &rhs)
	{
		return	lhs.location	== rhs.location		and
				lhs.binding		== rhs.binding		and
				lhs.format		== rhs.format		and
				lhs.offset		== rhs.offset;
	}
	
/*
=================================================
	VkVertexInputBindingDescription::operator ==
=================================================
*/
	ND_ inline bool operator == (const VkVertexInputBindingDescription &lhs, const VkVertexInputBindingDescription &rhs)
	{
		return	lhs.binding		== rhs.binding	and
				lhs.stride		== rhs.stride	and
				lhs.inputRate	== rhs.inputRate;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	PipelineInstance
=================================================
*/
	VGraphicsPipeline::PipelineInstance::PipelineInstance () :
		_hash{ 0 },
		subpassIndex{ 0 },
		dynamicState{ EPipelineDynamicState::None },
		flags{ 0 },
		viewportCount{ 0 }
	{}
	
/*
=================================================
	PipelineInstance::operator ==
=================================================
*/
	bool VGraphicsPipeline::PipelineInstance::operator == (const PipelineInstance &rhs) const
	{
		return	_hash			== rhs._hash		and
				renderPass		== rhs.renderPass	and
				subpassIndex	== rhs.subpassIndex	and
				renderState		== rhs.renderState	and
				vertexInput		== rhs.vertexInput	and
				dynamicState	== rhs.dynamicState	and
				flags			== rhs.flags		and
				viewportCount	== rhs.viewportCount;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VGraphicsPipeline::VGraphicsPipeline (const VFrameGraphWeak &fg) :
		_frameGraph{ fg }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VGraphicsPipeline::~VGraphicsPipeline ()
	{
		_Destroy();
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void VGraphicsPipeline::_Destroy ()
	{
		VFrameGraphPtr	fg = _frameGraph.lock();

		if ( fg )
		{
			for (auto& inst : _instances) {
				fg->GetDevice().DeletePipeline( inst.second );
			}
		}

		_layout				= null;
		_fragmentOutput		= null;
		_patchControlPoints	= 0;
		_earlyFragmentTests	= false;

		_supportedTopology.reset();
		_vertexAttribs.clear();
		_shaders.clear();
		_instances.clear();
		_frameGraph.reset();
	}

/*
=================================================
	CreateResources
=================================================
*/
	PipelineResourcesPtr  VGraphicsPipeline::CreateResources (const DescriptorSetID &id)
	{
		return MakeShared< VPipelineResources >( _layout->GetDescriptorSet( id ) );
	}
	
/*
=================================================
	GetDescriptorSet
=================================================
*/
	DescriptorSetLayoutPtr  VGraphicsPipeline::GetDescriptorSet (const DescriptorSetID &id) const
	{
		return Cast< DescriptorSetLayout >( _layout->GetDescriptorSet( id ) );
	}
	
/*
=================================================
	GetBindingIndex
=================================================
*/
	uint VGraphicsPipeline::GetBindingIndex (const DescriptorSetID &id) const
	{
		return _layout->GetBindingIndex( id );
	}

/*
=================================================
	GetShaderStages
=================================================
*/
	EShaderStages  VGraphicsPipeline::GetShaderStages () const
	{
		return EShaderStages::AllGraphics;	// TODO
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	PipelineInstance
=================================================
*/
	VComputePipeline::PipelineInstance::PipelineInstance () :
		_hash{ 0 },
		flags{ 0 }
	{}
	
/*
=================================================
	PipelineInstance::operator ==
=================================================
*/
	bool VComputePipeline::PipelineInstance::operator == (const PipelineInstance &rhs) const
	{
		return	localGroupSize.x	== rhs.localGroupSize.x		and
				localGroupSize.y	== rhs.localGroupSize.y		and
				localGroupSize.z	== rhs.localGroupSize.z;
	}

/*
=================================================
	PipelineInstance::operator >
=================================================
*
	bool VComputePipeline::PipelineInstance::operator >  (const PipelineInstance &rhs) const
	{
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	VComputePipeline::VComputePipeline (const VFrameGraphWeak &fg) :
		_frameGraph{ fg },
		_shader{ VK_NULL_HANDLE },
		_localSizeSpec{ ComputePipelineDesc::UNDEFINED_SPECIALIZATION }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VComputePipeline::~VComputePipeline ()
	{
		_Destroy();
	}
	
/*
=================================================
	_Destroy
=================================================
*/
	void VComputePipeline::_Destroy ()
	{
		VFrameGraphPtr	fg = _frameGraph.lock();

		if ( fg )
		{
			for (auto& inst : _instances) {
				fg->GetDevice().DeletePipeline( inst.second );
			}
		}

		_layout					= null;
		_shader					= null;
		_defaultLocalGroupSize	= Default;
        _localSizeSpec			= uint3{ ComputePipelineDesc::UNDEFINED_SPECIALIZATION };

		_instances.clear();
		_frameGraph.reset();
	}

/*
=================================================
	CreateResources
=================================================
*/
	PipelineResourcesPtr  VComputePipeline::CreateResources (const DescriptorSetID &id)
	{
		return MakeShared< VPipelineResources >( _layout->GetDescriptorSet( id ) );
	}
	
/*
=================================================
	GetDescriptorSet
=================================================
*/
	DescriptorSetLayoutPtr  VComputePipeline::GetDescriptorSet (const DescriptorSetID &id) const
	{
		return Cast< DescriptorSetLayout >( _layout->GetDescriptorSet( id ) );
	}
	
/*
=================================================
	GetBindingIndex
=================================================
*/
	uint VComputePipeline::GetBindingIndex (const DescriptorSetID &id) const
	{
		return _layout->GetBindingIndex( id );
	}
	
/*
=================================================
	GetShaderStages
=================================================
*/
	EShaderStages  VComputePipeline::GetShaderStages () const
	{
		return EShaderStages::Compute;
	}

}	// FG

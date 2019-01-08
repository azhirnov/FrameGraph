// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VGraphicsPipeline.h"
#include "VEnumCast.h"

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
	destructor
=================================================
*/
	VGraphicsPipeline::~VGraphicsPipeline ()
	{
		CHECK( _instances.empty() );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VGraphicsPipeline::Create (const GraphicsPipelineDesc &desc, RawPipelineLayoutID layoutId, FragmentOutputPtr fragOutput, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		
		for (auto& sh : desc._shaders)
		{
			CHECK_ERR( sh.second.data.size() == 1 );

			auto*	vk_shader = std::get_if< PipelineDescription::VkShaderPtr >( &sh.second.data.begin()->second );
			CHECK_ERR( vk_shader );

			_shaders.push_back(ShaderModule{ VEnumCast(sh.first), *vk_shader });
		}

		_layoutId			= PipelineLayoutID{ layoutId };
		_supportedTopology	= desc._supportedTopology;
		_fragmentOutput		= fragOutput;
		_vertexAttribs		= desc._vertexAttribs;
		_patchControlPoints	= desc._patchControlPoints;
		_earlyFragmentTests	= desc._earlyFragmentTests;
		_debugName			= dbgName;
		
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VGraphicsPipeline::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		SCOPELOCK( _rcCheck );

		for (auto& ppln : _instances) {
			readyToDelete.emplace_back( VK_OBJECT_TYPE_PIPELINE, uint64_t(ppln.second) );
		}
		
		if ( _layoutId ) {
			unassignIDs.push_back( _layoutId.Release() );
		}

		_shaders.clear();
		_instances.clear();
		_vertexAttribs.clear();
		_debugName.clear();

		_layoutId			= Default;
		_supportedTopology	= Default;
		_fragmentOutput		= null;
		_patchControlPoints	= 0;
		_earlyFragmentTests	= false;
	}


}	// FG

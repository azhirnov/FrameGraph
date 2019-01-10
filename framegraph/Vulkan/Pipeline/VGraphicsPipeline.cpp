// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VGraphicsPipeline.h"
#include "VEnumCast.h"
#include "Shared/EnumUtils.h"

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
	PipelineInstance::UpdateHash
=================================================
*/
	void VGraphicsPipeline::PipelineInstance::UpdateHash ()
	{
#	if FG_FAST_HASH
		_hash	= FG::HashOf( &_hash, sizeof(*this) - sizeof(_hash) );
#	else
		_hash	= HashOf( layoutId )		+
				  HashOf( renderPassId )	+ HashOf( subpassIndex )	+
				  HashOf( renderState )		+ HashOf( vertexInput )		+
				  HashOf( dynamicState )	+ HashOf( viewportCount )	+
				  HashOf( debugMode );		  //HashOf( flags );
#	endif
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
		
		for (auto& stage : desc._shaders)
		{
			const auto	vk_stage = VEnumCast( stage.first );

			for (auto& sh : stage.second.data)
			{
				auto*	vk_shader = std::get_if< PipelineDescription::VkShaderPtr >( &sh.second );
				CHECK_ERR( vk_shader );

				_shaders.push_back(ShaderModule{ vk_stage, *vk_shader, EShaderDebugMode_From(sh.first) });
			}
		}
		CHECK_ERR( _shaders.size() );

		_baseLayoutId		= PipelineLayoutID{ layoutId };
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
			unassignIDs.push_back( const_cast<PipelineInstance &>(ppln.first).layoutId );
		}
		
		if ( _baseLayoutId ) {
			unassignIDs.push_back( _baseLayoutId.Release() );
		}

		_shaders.clear();
		_instances.clear();
		_vertexAttribs.clear();
		_debugName.clear();

		_baseLayoutId		= Default;
		_supportedTopology	= Default;
		_fragmentOutput		= null;
		_patchControlPoints	= 0;
		_earlyFragmentTests	= false;
	}


}	// FG

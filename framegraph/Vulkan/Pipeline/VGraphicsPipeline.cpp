// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VGraphicsPipeline.h"
#include "VEnumCast.h"
#include "VResourceManager.h"
#include "VDevice.h"
#include "Shared/EnumUtils.h"

namespace FG
{

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
		_hash	= FGC::HashOf( &_hash, sizeof(*this) - sizeof(_hash) );
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
	bool VGraphicsPipeline::Create (const GraphicsPipelineDesc &desc, RawPipelineLayoutID layoutId, StringView dbgName)
	{
		EXLOCK( _drCheck );
		
		for (auto& stage : desc._shaders)
		{
			const auto	vk_stage = VEnumCast( stage.first );

			for (auto& sh : stage.second.data)
			{
				auto*	vk_shader = UnionGetIf< PipelineDescription::VkShaderPtr >( &sh.second );
				CHECK_ERR( vk_shader );

				_shaders.push_back(ShaderModule{ vk_stage, *vk_shader, EShaderDebugMode_From(sh.first) });
			}
		}
		CHECK_ERR( _shaders.size() );

		_baseLayoutId		= PipelineLayoutID{ layoutId };
		_supportedTopology	= desc._supportedTopology;
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
	void VGraphicsPipeline::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		auto&	dev = resMngr.GetDevice();

		for (auto& ppln : _instances) {
			dev.vkDestroyPipeline( dev.GetVkDevice(), ppln.second, null );
			resMngr.ReleaseResource( const_cast<PipelineInstance &>(ppln.first).layoutId );
		}
		
		if ( _baseLayoutId ) {
			resMngr.ReleaseResource( _baseLayoutId.Release() );
		}

		_shaders.clear();
		_instances.clear();
		_vertexAttribs.clear();
		_debugName.clear();

		_baseLayoutId		= Default;
		_supportedTopology	= Default;
		_patchControlPoints	= 0;
		_earlyFragmentTests	= false;
	}


}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VMeshPipeline.h"
#include "VEnumCast.h"

namespace FG
{

/*
=================================================
	PipelineInstance::UpdateHash
=================================================
*/
	void VMeshPipeline::PipelineInstance::UpdateHash ()
	{
#	if FG_FAST_HASH
		_hash	= FG::HashOf( &_hash, sizeof(*this) - sizeof(_hash) );
#	else
		_hash	= HashOf( layoutId )		+
				  HashOf( renderPassId )	+ HashOf( subpassIndex )	+
				  HashOf( renderState )		+ HashOf( dynamicState )	+
				  HashOf( viewportCount )	+ HashOf( debugMode );		//+ HashOf( flags );
#	endif
	}
//-----------------------------------------------------------------------------



/*
=================================================
	destructor
=================================================
*/
	VMeshPipeline::~VMeshPipeline ()
	{
		CHECK( _instances.empty() );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VMeshPipeline::Create (const MeshPipelineDesc &desc, RawPipelineLayoutID layoutId, FragmentOutputPtr fragOutput, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		
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
		_topology			= desc._topology;
		_fragmentOutput		= fragOutput;
		_earlyFragmentTests	= desc._earlyFragmentTests;
		_debugName			= dbgName;
		
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VMeshPipeline::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
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
		_debugName.clear();
		_baseLayoutId		= Default;
		_topology			= Default;
		_fragmentOutput		= null;
		_earlyFragmentTests	= false;
	}


}	// FG

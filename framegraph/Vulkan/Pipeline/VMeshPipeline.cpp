// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VMeshPipeline.h"
#include "VEnumCast.h"

namespace FG
{

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
			readyToDelete.emplace_back( VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, uint64_t(ppln.second) );
		}

		if ( _layoutId ) {
			unassignIDs.push_back( _layoutId.Release() );
		}

		_shaders.clear();
		_instances.clear();
		_debugName.clear();
		_layoutId			= Default;
		_supportedTopology	= Default;
		_fragmentOutput		= null;
		_earlyFragmentTests	= false;
	}


}	// FG

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRayTracingPipeline.h"
#include "VEnumCast.h"

namespace FG
{
	
/*
=================================================
	destructor
=================================================
*/
	VRayTracingPipeline::~VRayTracingPipeline ()
	{
		CHECK( _instances.empty() );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VRayTracingPipeline::Create (const RayTracingPipelineDesc &desc, RawPipelineLayoutID layoutId, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		
		for (auto& sh : desc._shaders)
		{
			CHECK_ERR( sh.second.data.size() == 1 );

			auto*	vk_shader = std::get_if< PipelineDescription::VkShaderPtr >( &sh.second.data.begin()->second );
			CHECK_ERR( vk_shader );

			_shaders.push_back(ShaderModule{ VEnumCast(sh.first), *vk_shader });
		}
		
		_layoutId	= PipelineLayoutID{ layoutId };
		_debugName	= dbgName;

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VRayTracingPipeline::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
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
		
		_layoutId	= Default;
	}


}	// FG

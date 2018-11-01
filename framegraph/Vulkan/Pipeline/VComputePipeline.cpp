// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VComputePipeline.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VComputePipeline::VComputePipeline () :
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
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VComputePipeline::Create (const ComputePipelineDesc &desc, RawPipelineLayoutID layoutId)
	{
		CHECK_ERR( GetState() == EState::Initial );
		CHECK_ERR( desc._shader.data.size() == 1 );
		
		auto*	vk_shader = std::get_if< PipelineDescription::VkShaderPtr >( &desc._shader.data.begin()->second );
		CHECK_ERR( vk_shader );

		_shader					= *vk_shader;
		_layoutId				= PipelineLayoutID{ layoutId };
		_defaultLocalGroupSize	= desc._defaultLocalGroupSize;
		_localSizeSpec			= desc._localSizeSpec;
		
		_OnCreate();
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VComputePipeline::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		for (auto& ppln : _instances) {
			readyToDelete.emplace_back( VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, uint64_t(ppln.second) );
		}
		
		if ( _layoutId ) {
			unassignIDs.push_back( _layoutId.Release() );
		}

		_instances.clear();
		_shader					= null;
		_layoutId				= Default;
		_defaultLocalGroupSize	= Default;
        _localSizeSpec			= uint3{ ComputePipelineDesc::UNDEFINED_SPECIALIZATION };
		
		_OnDestroy();
	}


}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
	bool VComputePipeline::Create (const ComputePipelineDesc &desc, RawPipelineLayoutID layoutId, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( desc._shader.data.size() == 1 );
		
		auto*	vk_shader = std::get_if< PipelineDescription::VkShaderPtr >( &desc._shader.data.begin()->second );
		CHECK_ERR( vk_shader );

		_shader					= *vk_shader;
		_layoutId				= PipelineLayoutID{ layoutId };
		_defaultLocalGroupSize	= desc._defaultLocalGroupSize;
		_localSizeSpec			= desc._localSizeSpec;
		_debugName				= dbgName;
		
		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VComputePipeline::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		SCOPELOCK( _rcCheck );

		for (auto& ppln : _instances) {
			readyToDelete.emplace_back( VK_OBJECT_TYPE_PIPELINE, uint64_t(ppln.second) );
		}
		
		if ( _layoutId ) {
			unassignIDs.push_back( _layoutId.Release() );
		}

		_instances.clear();
		_debugName.clear();
		_shader					= null;
		_layoutId				= Default;
		_defaultLocalGroupSize	= Default;
		_localSizeSpec			= uint3{ ComputePipelineDesc::UNDEFINED_SPECIALIZATION };
	}


}	// FG

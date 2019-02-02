// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VComputePipeline.h"
#include "Shared/EnumUtils.h"

namespace FG
{

/*
=================================================
	PipelineInstance::UpdateHash
=================================================
*/
	void VComputePipeline::PipelineInstance::UpdateHash ()
	{
#	if FG_FAST_HASH
		_hash	= FG::HashOf( &_hash, sizeof(*this) - sizeof(_hash) );
#	else
		_hash	= HashOf( layoutId )	+ HashOf( localGroupSize ) +
				  HashOf( flags )		+ HashOf( debugMode );
#	endif
	}
//-----------------------------------------------------------------------------



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
		
		for (auto& sh : desc._shader.data)
		{
			auto*	vk_shader = UnionGetIf< PipelineDescription::VkShaderPtr >( &sh.second );
			CHECK_ERR( vk_shader );

			_shaders.push_back(ShaderModule{ *vk_shader, EShaderDebugMode_From(sh.first) });
		}
		CHECK_ERR( _shaders.size() );

		_baseLayoutId			= PipelineLayoutID{ layoutId };
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
			unassignIDs.push_back( const_cast<PipelineInstance &>(ppln.first).layoutId );
		}
		
		if ( _baseLayoutId ) {
			unassignIDs.push_back( _baseLayoutId.Release() );
		}

		_instances.clear();
		_debugName.clear();
		_shaders.clear();

		_baseLayoutId			= Default;
		_defaultLocalGroupSize	= Default;
		_localSizeSpec			= uint3{ ComputePipelineDesc::UNDEFINED_SPECIALIZATION };
	}


}	// FG

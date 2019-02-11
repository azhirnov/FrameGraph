// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRayTracingPipeline.h"
#include "VResourceManagerThread.h"
#include "VEnumCast.h"
#include "VDevice.h"

namespace FG
{

/*
=================================================
	destructor
=================================================
*/
	VRayTracingPipeline::~VRayTracingPipeline ()
	{
		CHECK( not _baseLayoutId );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VRayTracingPipeline::Create (const RayTracingPipelineDesc &desc, RawPipelineLayoutID layoutId)
	{
		EXLOCK( _rcCheck );
		
		_shaders.reserve( desc._shaders.size() * 2 );

		for (auto& stage : desc._shaders)
		{
			const auto	vk_stage = VEnumCast( stage.second.shaderType );
			
			for (auto& sh : stage.second.data)
			{
				auto*	vk_shader = UnionGetIf< PipelineDescription::VkShaderPtr >( &sh.second );
				CHECK_ERR( vk_shader );

				_shaders.push_back(ShaderModule{ stage.first, vk_stage, *vk_shader, EShaderDebugMode_From(sh.first), stage.second.specConstants });
			}
		}

		std::sort( _shaders.begin(), _shaders.end(),
				   [] (auto& lhs, auto& rhs) { return lhs.shaderId < rhs.shaderId; });

		_baseLayoutId = PipelineLayoutID{layoutId};
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VRayTracingPipeline::Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t unassignIDs)
	{
		EXLOCK( _rcCheck );

		if ( _baseLayoutId ) {
			unassignIDs.emplace_back( _baseLayoutId.Release() );
		}

		_shaders.clear();

		_baseLayoutId = Default;
	}


}	// FG

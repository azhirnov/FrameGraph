// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRayTracingPipeline.h"
#include "VResourceManager.h"
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
		EXLOCK( _drCheck );
		CHECK_ERR( _shaders.empty() );
		
		_shaders.reserve( desc._shaders.size() * 2 );
		_debugModeBits = Default;

		for (auto& stage : desc._shaders)
		{
			const auto	vk_stage = VEnumCast( stage.second.shaderType );
			
			for (auto& sh : stage.second.data)
			{
				auto*	vk_shader = UnionGetIf< PipelineDescription::VkShaderPtr >( &sh.second );
				CHECK_ERR( vk_shader );

				_shaders.push_back(ShaderModule{ stage.first, vk_stage, *vk_shader, EShaderDebugMode_From(sh.first), stage.second.specConstants });

				_debugModeBits.set( uint(_shaders.back().debugMode) );
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
	void VRayTracingPipeline::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		if ( _baseLayoutId ) {
			resMngr.ReleaseResource( _baseLayoutId.Release() );
		}

		// clear and release memory
		{
			ShaderModules_t	tmp;
			std::swap( tmp, _shaders );
		}

		_debugModeBits = Default;
		_baseLayoutId  = Default;
	}


}	// FG

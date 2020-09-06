// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VMeshPipeline.h"
#include "VEnumCast.h"
#include "VResourceManager.h"
#include "VDevice.h"

#ifdef VK_NV_mesh_shader

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
		_hash	= FGC::HashOf( &_hash, sizeof(*this) - sizeof(_hash) );
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
	bool VMeshPipeline::Create (const MeshPipelineDesc &desc, RawPipelineLayoutID layoutId, StringView dbgName)
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
		_topology			= desc._topology;
		_earlyFragmentTests	= desc._earlyFragmentTests;
		_debugName			= dbgName;
		
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VMeshPipeline::Destroy (VResourceManager &resMngr)
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
		_debugName.clear();
		_baseLayoutId		= Default;
		_topology			= Default;
		_earlyFragmentTests	= false;
	}


}	// FG

#endif	// VK_NV_mesh_shader

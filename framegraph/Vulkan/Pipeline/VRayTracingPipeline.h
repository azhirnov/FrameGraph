// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VPipelineLayout.h"

namespace FG
{

	//
	// Ray Tracing Pipeline
	//

	class VRayTracingPipeline final
	{
		friend class VPipelineCache;
		
	// types
	private:
		struct PipelineInstance
		{
		// variables
			HashVal					_hash;
			VkPipelineCreateFlags	flags;

		// methods
			PipelineInstance ();

			ND_ bool  operator == (const PipelineInstance &rhs) const;
		};

		struct PipelineInstanceHash {
			ND_ size_t	operator () (const PipelineInstance &value) const noexcept	{ return size_t(value._hash); }
		};
		
		struct ShaderModule
		{
			VkShaderStageFlagBits				stage;
			PipelineDescription::VkShaderPtr	module;
		};

		using Instances_t			= HashMap< PipelineInstance, VkPipeline, PipelineInstanceHash >;
		using VkShaderPtr			= PipelineDescription::VkShaderPtr;
		using ShaderModules_t		= FixedArray< ShaderModule, 8 >;


	// variables
	private:
		PipelineLayoutID		_layoutId;
		Instances_t				_instances;
		
		ShaderModules_t			_shaders;
		
		DebugName_t				_debugName;
		
		RWRaceConditionCheck	_rcCheck;


	// methods
	public:
		VRayTracingPipeline () {}
		VRayTracingPipeline (VRayTracingPipeline &&) = default;
		~VRayTracingPipeline ();
		
		bool Create (const RayTracingPipelineDesc &desc, RawPipelineLayoutID layoutId, StringView dbgName);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		
		ND_ RawPipelineLayoutID		GetLayoutID ()	const	{ SHAREDLOCK( _rcCheck );  return _layoutId.Get(); }
	};

	
	
/*
=================================================
	PipelineInstance
=================================================
*/
	inline VRayTracingPipeline::PipelineInstance::PipelineInstance () :
		_hash{ 0 },		flags{ 0 }
	{}
	
/*
=================================================
	PipelineInstance::operator ==
=================================================
*/
	inline bool VRayTracingPipeline::PipelineInstance::operator == (const PipelineInstance &rhs) const
	{
		return	flags	== rhs.flags;
	}


}	// FG

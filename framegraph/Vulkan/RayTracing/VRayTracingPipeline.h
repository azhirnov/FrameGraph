// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
	public:
		using SpecConstants_t	= PipelineDescription::SpecConstants_t;

		struct ShaderModule
		{
			RTShaderID							shaderId;
			VkShaderStageFlagBits				stage;
			PipelineDescription::VkShaderPtr	module;
			EShaderDebugMode					debugMode	= Default;
			SpecConstants_t						specConstants;

			ND_ bool  operator == (const RTShaderID &rhs) const	{ return shaderId == rhs; }
			ND_ bool  operator <  (const RTShaderID &rhs) const	{ return shaderId <  rhs; }
		};

		using ShaderModules_t	= Array< ShaderModule >;


	// variables
	private:
		PipelineLayoutID		_baseLayoutId;
		ShaderModules_t			_shaders;

		RWRaceConditionCheck	_rcCheck;


	// methods
	public:
		VRayTracingPipeline () {}
		VRayTracingPipeline (VRayTracingPipeline &&) = default;
		~VRayTracingPipeline ();
		
		bool Create (const RayTracingPipelineDesc &desc, RawPipelineLayoutID layoutId);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		
		ND_ ArrayView<ShaderModule>	GetShaderModules ()	const	{ SHAREDLOCK( _rcCheck );  return _shaders; }
		ND_ RawPipelineLayoutID		GetLayoutID ()		const	{ SHAREDLOCK( _rcCheck );  return _baseLayoutId.Get(); }
	};


}	// FG

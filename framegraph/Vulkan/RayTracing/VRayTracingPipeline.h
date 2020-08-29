// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VPipelineLayout.h"

#ifdef VK_NV_ray_tracing

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
		using DebugModeBits_t	= BitSet< uint(EShaderDebugMode::_Count) >;

		struct ShaderModule
		{
			RTShaderID							shaderId;
			VkShaderStageFlagBits				stage		= VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
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
		DebugModeBits_t			_debugModeBits;
		
		DebugName_t				_debugName;

		RWDataRaceCheck			_drCheck;


	// methods
	public:
		VRayTracingPipeline () {}
		~VRayTracingPipeline ();
		
		bool Create (const RayTracingPipelineDesc &desc, RawPipelineLayoutID layoutId, StringView dbgName);
		void Destroy (VResourceManager &);
		
		ND_ ArrayView<ShaderModule>	GetShaderModules ()	const	{ SHAREDLOCK( _drCheck );  return _shaders; }
		ND_ DebugModeBits_t			GetDebugModes ()	const	{ SHAREDLOCK( _drCheck );  return _debugModeBits; }
		ND_ RawPipelineLayoutID		GetLayoutID ()		const	{ SHAREDLOCK( _drCheck );  return _baseLayoutId.Get(); }
	};


}	// FG

#endif	// VK_NV_ray_tracing

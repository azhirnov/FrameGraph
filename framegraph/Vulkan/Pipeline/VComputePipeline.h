// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VPipelineLayout.h"

namespace FG
{

	//
	// Compute Pipeline
	//

	class VComputePipeline final
	{
		friend class VPipelineCache;
		
	// types
	private:
		struct PipelineInstance
		{
		// variables
			HashVal					_hash;
			RawPipelineLayoutID		layoutId;		// strong reference
			uint3					localGroupSize;
			VkPipelineCreateFlags	flags		= 0;
			uint					debugMode	= 0;

		// methods
			PipelineInstance () {}

			void UpdateHash ();

			ND_ bool  operator == (const PipelineInstance &rhs) const;
		};

		struct PipelineInstanceHash {
			ND_ size_t	operator () (const PipelineInstance &value) const	{ return size_t(value._hash); }
		};

		struct ShaderModule
		{
			PipelineDescription::VkShaderPtr	module;
			EShaderDebugMode					debugMode	= Default;
		};

		using Instances_t			= HashMap< PipelineInstance, VkPipeline, PipelineInstanceHash >;
		using VkShaderPtr			= PipelineDescription::VkShaderPtr;
		using ShaderModules_t		= FixedArray< ShaderModule, 4 >;


	// variables
	private:
		mutable std::shared_mutex	_instanceGuard;
		mutable Instances_t			_instances;

		PipelineLayoutID			_baseLayoutId;
		ShaderModules_t				_shaders;

		uint3						_defaultLocalGroupSize;
		uint3						_localSizeSpec;

		DebugName_t					_debugName;
		
		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VComputePipeline ();
		VComputePipeline (VComputePipeline &&) = default;
		~VComputePipeline ();
		
		bool Create (const ComputePipelineDesc &desc, RawPipelineLayoutID layoutId, StringView dbgName);
		void Destroy (VResourceManager &);

		ND_ RawPipelineLayoutID		GetLayoutID ()		const	{ SHAREDLOCK( _drCheck );  return _baseLayoutId.Get(); }
		
		ND_ StringView				GetDebugName ()		const	{ SHAREDLOCK( _drCheck );  return _debugName; }
	};

	
/*
=================================================
	PipelineInstance::operator ==
=================================================
*/
	inline bool VComputePipeline::PipelineInstance::operator == (const PipelineInstance &rhs) const
	{
		return	layoutId			== rhs.layoutId				and
				localGroupSize.x	== rhs.localGroupSize.x		and
				localGroupSize.y	== rhs.localGroupSize.y		and
				localGroupSize.z	== rhs.localGroupSize.z		and
				flags				== rhs.flags				and
				debugMode			== rhs.debugMode;
	}


}	// FG

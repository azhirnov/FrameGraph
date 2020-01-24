// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VPipelineLayout.h"

namespace FG
{

	//
	// Graphics Pipeline
	//

	class VGraphicsPipeline final
	{
		friend class VPipelineCache;
		
	// types
	public:
		struct ShaderModule
		{
			VkShaderStageFlagBits				stage		= VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			PipelineDescription::VkShaderPtr	module;
			EShaderDebugMode					debugMode	= Default;
		};


	private:
		struct PipelineInstance
		{
		// variables
			HashVal						_hash;
			RawPipelineLayoutID			layoutId;		// strong reference
			RawRenderPassID				renderPassId;
			RenderState					renderState;
			VertexInputState			vertexInput;
			EPipelineDynamicState		dynamicState	= Default;
			//VkPipelineCreateFlags		flags			= 0;
			uint8_t						subpassIndex	= 0;
			uint8_t						viewportCount	= 0;
			uint						debugMode		= 0;

		// methods
			PipelineInstance () {}

			void UpdateHash ();

			ND_ bool  operator == (const PipelineInstance &rhs) const;
		};

		struct PipelineInstanceHash {
			ND_ size_t	operator () (const PipelineInstance &value) const	{ return size_t(value._hash); }
		};

		using Instances_t			= HashMap< PipelineInstance, VkPipeline, PipelineInstanceHash >;
		using ShaderModules_t		= FixedArray< ShaderModule, 8 >;
		using TopologyBits_t		= GraphicsPipelineDesc::TopologyBits_t;
		using VertexAttrib			= VertexInputState::VertexAttrib;
		using VertexAttribs_t		= GraphicsPipelineDesc::VertexAttribs_t;


	// variables
	private:
		mutable std::shared_mutex	_instanceGuard;
		mutable Instances_t			_instances;

		PipelineLayoutID			_baseLayoutId;
		ShaderModules_t				_shaders;

		TopologyBits_t				_supportedTopology;
		VertexAttribs_t				_vertexAttribs;
		uint						_patchControlPoints		= 0;
		bool						_earlyFragmentTests		= true;
		
		DebugName_t					_debugName;
		
		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VGraphicsPipeline () {}
		VGraphicsPipeline (VGraphicsPipeline &&) = default;
		~VGraphicsPipeline ();

		bool Create (const GraphicsPipelineDesc &desc, RawPipelineLayoutID layoutId, StringView dbgName);
		void Destroy (VResourceManager &);
		
		ND_ RawPipelineLayoutID		GetLayoutID ()			const	{ SHAREDLOCK( _drCheck );  return _baseLayoutId.Get(); }
		ND_ ArrayView<VertexAttrib>	GetVertexAttribs ()		const	{ SHAREDLOCK( _drCheck );  return _vertexAttribs; }

		ND_ bool					IsEarlyFragmentTests ()	const	{ SHAREDLOCK( _drCheck );  return _earlyFragmentTests; }
		
		ND_ StringView				GetDebugName ()			const	{ SHAREDLOCK( _drCheck );  return _debugName; }
	};

	
/*
=================================================
	PipelineInstance::operator ==
=================================================
*/
	inline bool VGraphicsPipeline::PipelineInstance::operator == (const PipelineInstance &rhs) const
	{
		return	_hash			== rhs._hash			and
				layoutId		== rhs.layoutId			and
				renderPassId	== rhs.renderPassId		and
				subpassIndex	== rhs.subpassIndex		and
				renderState		== rhs.renderState		and
				vertexInput		== rhs.vertexInput		and
				dynamicState	== rhs.dynamicState		and
				//flags			== rhs.flags			and
				viewportCount	== rhs.viewportCount	and
				debugMode		== rhs.debugMode;
	}


}	// FG

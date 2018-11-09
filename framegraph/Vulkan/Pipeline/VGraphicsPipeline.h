// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VPipelineLayout.h"

namespace FG
{

	//
	// Graphics Pipeline
	//

	class VGraphicsPipeline final : public ResourceBase
	{
		friend class VPipelineCache;
		
	// types
	public:
		struct FragmentOutputInstance
		{
		// types
		private:
			using FragmentOutput	= GraphicsPipelineDesc::FragmentOutput;
			using FragmentOutputs_t = GraphicsPipelineDesc::FragmentOutputs_t;

		// variables
		private:
			HashVal				_hash;
			FragmentOutputs_t	_values;

		// methods
		public:
			explicit FragmentOutputInstance (ArrayView<FragmentOutput> values);

			ND_ bool operator == (const FragmentOutputInstance &rhs) const;

			ND_ ArrayView<FragmentOutput>	Get ()		const	{ return _values; }
			ND_ HashVal						GetHash ()	const	{ return _hash; }
		};


	private:
		struct PipelineInstance
		{
		// variables
			HashVal						_hash;
			RawRenderPassID				renderPassId;
			uint						subpassIndex;
			RenderState					renderState;
			VertexInputState			vertexInput;
			EPipelineDynamicState		dynamicState;
			VkPipelineCreateFlags		flags;
			uint						viewportCount;
			// TODO: specialization constants

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
		using ShaderModules_t		= FixedArray< ShaderModule, 8 >;
		using TopologyBits_t		= GraphicsPipelineDesc::TopologyBits_t;
		using VertexAttrib			= VertexInputState::VertexAttrib;
		using VertexAttribs_t		= GraphicsPipelineDesc::VertexAttribs_t;
		using FragmentOutputPtr		= const FragmentOutputInstance *;


	// variables
	private:
		PipelineLayoutID		_layoutId;
		Instances_t				_instances;

		ShaderModules_t			_shaders;
		TopologyBits_t			_supportedTopology;
		FragmentOutputPtr		_fragmentOutput			= null;
		VertexAttribs_t			_vertexAttribs;
		uint					_patchControlPoints		= 0;
		bool					_earlyFragmentTests		= true;
		
		DebugName_t				_debugName;


	// methods
	public:
		VGraphicsPipeline () {}
		~VGraphicsPipeline ();

		bool Create (const GraphicsPipelineDesc &desc, RawPipelineLayoutID layoutId, FragmentOutputPtr fragOutput, StringView dbgName);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		
		ND_ RawPipelineLayoutID		GetLayoutID ()		const	{ SHAREDLOCK( _rcCheck );  return _layoutId.Get(); }
		ND_ ArrayView<VertexAttrib>	GetVertexAttribs ()	const	{ SHAREDLOCK( _rcCheck );  return _vertexAttribs; }
	};

	
/*
=================================================
	PipelineInstance
=================================================
*/
	inline VGraphicsPipeline::PipelineInstance::PipelineInstance () :
		_hash{ 0 },
		subpassIndex{ 0 },
		dynamicState{ EPipelineDynamicState::None },
		flags{ 0 },
		viewportCount{ 0 }
	{}
	
/*
=================================================
	PipelineInstance::operator ==
=================================================
*/
	inline bool VGraphicsPipeline::PipelineInstance::operator == (const PipelineInstance &rhs) const
	{
		return	_hash			== rhs._hash		and
				renderPassId	== rhs.renderPassId	and
				subpassIndex	== rhs.subpassIndex	and
				renderState		== rhs.renderState	and
				vertexInput		== rhs.vertexInput	and
				dynamicState	== rhs.dynamicState	and
				flags			== rhs.flags		and
				viewportCount	== rhs.viewportCount;
	}


}	// FG

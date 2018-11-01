// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VGraphicsPipeline.h"

namespace FG
{

	//
	// Mesh Pipeline
	//

	class VMeshPipeline final : public ResourceBase
	{
		friend class VPipelineCache;
		
	// types
	private:
		struct PipelineInstance
		{
		// variables
			HashVal						_hash;
			RawRenderPassID				renderPassId;
			uint						subpassIndex;
			RenderState					renderState;
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
		using ShaderModules_t		= FixedArray< ShaderModule, 4 >;
		using TopologyBits_t		= GraphicsPipelineDesc::TopologyBits_t;
		using FragmentOutputPtr		= const VGraphicsPipeline::FragmentOutputInstance *;


	// variables
	private:
		PipelineLayoutID		_layoutId;
		Instances_t				_instances;

		ShaderModules_t			_shaders;
		TopologyBits_t			_supportedTopology;		// TODO: use
		FragmentOutputPtr		_fragmentOutput;
		bool					_earlyFragmentTests		= true;


	// methods
	public:
		VMeshPipeline () {}
		~VMeshPipeline ();

		bool Create (const MeshPipelineDesc &desc, RawPipelineLayoutID layoutId, FragmentOutputPtr fragOutput);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		
		ND_ RawPipelineLayoutID		GetLayoutID ()	const	{ return _layoutId.Get(); }
	};

	
/*
=================================================
	PipelineInstance
=================================================
*/
	inline VMeshPipeline::PipelineInstance::PipelineInstance () :
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
	inline bool VMeshPipeline::PipelineInstance::operator == (const PipelineInstance &rhs) const
	{
		return	_hash			== rhs._hash		and
				renderPassId	== rhs.renderPassId	and
				subpassIndex	== rhs.subpassIndex	and
				renderState		== rhs.renderState	and
				dynamicState	== rhs.dynamicState	and
				flags			== rhs.flags		and
				viewportCount	== rhs.viewportCount;
	}


}	// FG

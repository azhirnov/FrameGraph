// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VGraphicsPipeline.h"

namespace FG
{

	//
	// Mesh Pipeline
	//

	class VMeshPipeline final
	{
		friend class VPipelineCache;
		
	// types
	private:
		struct PipelineInstance
		{
		// variables
			HashVal						_hash;
			RawRenderPassID				renderPassId;
			uint						subpassIndex	= 0;
			RenderState					renderState;
			EPipelineDynamicState		dynamicState	= EPipelineDynamicState::None;
			VkPipelineCreateFlags		flags			= 0;
			uint						viewportCount	= 0;
			// TODO: specialization constants

		// methods
			PipelineInstance () {}

			ND_ bool  operator == (const PipelineInstance &rhs) const;
		};

		struct PipelineInstanceHash {
			ND_ size_t	operator () (const PipelineInstance &value) const noexcept	{ return size_t(value._hash); }
		};

		using Instances_t			= HashMap< PipelineInstance, VkPipeline, PipelineInstanceHash >;
		using ShaderModule			= VGraphicsPipeline::ShaderModule;
		using ShaderModules_t		= FixedArray< ShaderModule, 4 >;
		using TopologyBits_t		= GraphicsPipelineDesc::TopologyBits_t;
		using FragmentOutputPtr		= const VGraphicsPipeline::FragmentOutputInstance *;


	// variables
	private:
		PipelineLayoutID		_layoutId;
		Instances_t				_instances;

		ShaderModules_t			_shaders;
		TopologyBits_t			_supportedTopology;		// TODO: use
		FragmentOutputPtr		_fragmentOutput			= null;
		bool					_earlyFragmentTests		= true;
		
		DebugName_t				_debugName;
		
		RWRaceConditionCheck	_rcCheck;


	// methods
	public:
		VMeshPipeline () {}
		VMeshPipeline (VMeshPipeline &&) = default;
		~VMeshPipeline ();

		bool Create (const MeshPipelineDesc &desc, RawPipelineLayoutID layoutId, FragmentOutputPtr fragOutput, StringView dbgName);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		
		ND_ RawPipelineLayoutID		GetLayoutID ()			const	{ SHAREDLOCK( _rcCheck );  return _layoutId.Get(); }

		ND_ FragmentOutputPtr		GetFragmentOutput ()	const	{ SHAREDLOCK( _rcCheck );  return _fragmentOutput; }
		ND_ bool					IsEarlyFragmentTests ()	const	{ SHAREDLOCK( _rcCheck );  return _earlyFragmentTests; }
	};

	
	
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

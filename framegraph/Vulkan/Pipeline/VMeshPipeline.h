// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
			RawPipelineLayoutID			layoutId;		// strong reference
			RawRenderPassID				renderPassId;
			RenderState					renderState;
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
			ND_ size_t	operator () (const PipelineInstance &value) const noexcept	{ return size_t(value._hash); }
		};

		using Instances_t			= HashMap< PipelineInstance, VkPipeline, PipelineInstanceHash >;
		using ShaderModule			= VGraphicsPipeline::ShaderModule;
		using ShaderModules_t		= FixedArray< ShaderModule, 8 >;
		using TopologyBits_t		= GraphicsPipelineDesc::TopologyBits_t;
		using FragmentOutputPtr		= const VGraphicsPipeline::FragmentOutputInstance *;


	// variables
	private:
		PipelineLayoutID		_baseLayoutId;
		Instances_t				_instances;

		ShaderModules_t			_shaders;
		EPrimitive				_topology				= Default;
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
		
		ND_ RawPipelineLayoutID		GetLayoutID ()			const	{ SHAREDLOCK( _rcCheck );  return _baseLayoutId.Get(); }

		ND_ FragmentOutputPtr		GetFragmentOutput ()	const	{ SHAREDLOCK( _rcCheck );  return _fragmentOutput; }
		ND_ bool					IsEarlyFragmentTests ()	const	{ SHAREDLOCK( _rcCheck );  return _earlyFragmentTests; }
		
		ND_ StringView				GetDebugName ()			const	{ SHAREDLOCK( _rcCheck );  return _debugName; }
	};

	
	
/*
=================================================
	PipelineInstance::operator ==
=================================================
*/
	inline bool VMeshPipeline::PipelineInstance::operator == (const PipelineInstance &rhs) const
	{
		return	_hash			== rhs._hash			and
				layoutId		== rhs.layoutId			and
				renderPassId	== rhs.renderPassId		and
				subpassIndex	== rhs.subpassIndex		and
				renderState		== rhs.renderState		and
				dynamicState	== rhs.dynamicState		and
				//flags			== rhs.flags			and
				viewportCount	== rhs.viewportCount	and
				debugMode		== rhs.debugMode;
	}


}	// FG

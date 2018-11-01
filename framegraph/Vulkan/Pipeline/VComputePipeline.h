// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VPipelineLayout.h"

namespace FG
{

	//
	// Compute Pipeline
	//

	class VComputePipeline final : public ResourceBase
	{
		friend class VPipelineCache;
		
	// types
	private:
		struct PipelineInstance
		{
		// variables
			HashVal					_hash;
			uint3					localGroupSize;
			VkPipelineCreateFlags	flags;
			// TODO: specialization constants

		// methods
			PipelineInstance ();

			ND_ bool  operator == (const PipelineInstance &rhs) const;
		};

		struct PipelineInstanceHash {
			ND_ size_t	operator () (const PipelineInstance &value) const noexcept	{ return size_t(value._hash); }
		};

		using Instances_t			= HashMap< PipelineInstance, VkPipeline, PipelineInstanceHash >;
		using VkShaderPtr			= PipelineDescription::VkShaderPtr;


	// variables
	private:
		PipelineLayoutID		_layoutId;
		Instances_t				_instances;

		VkShaderPtr				_shader;

		uint3					_defaultLocalGroupSize;
		uint3					_localSizeSpec;


	// methods
	public:
		VComputePipeline ();
		~VComputePipeline ();
		
		bool Create (const ComputePipelineDesc &desc, RawPipelineLayoutID layoutId);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);

		ND_ RawPipelineLayoutID		GetLayoutID ()	const	{ return _layoutId.Get(); }
	};

	
/*
=================================================
	PipelineInstance
=================================================
*/
	inline VComputePipeline::PipelineInstance::PipelineInstance () :
		_hash{ 0 },		flags{ 0 }
	{}
	
/*
=================================================
	PipelineInstance::operator ==
=================================================
*/
	inline bool VComputePipeline::PipelineInstance::operator == (const PipelineInstance &rhs) const
	{
		return	localGroupSize.x	== rhs.localGroupSize.x		and
				localGroupSize.y	== rhs.localGroupSize.y		and
				localGroupSize.z	== rhs.localGroupSize.z		and
				flags				== rhs.flags;
	}


}	// FG

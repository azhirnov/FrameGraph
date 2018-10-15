// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/Pipeline.h"
#include "VPipelineLayout.h"

namespace FG
{

	//
	// Graphics Pipeline
	//

	class VGraphicsPipeline final : public Pipeline
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
			VRenderPassPtr				renderPass;
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

		using Instances_t		= HashMap< PipelineInstance, VkPipeline, PipelineInstanceHash >;
		using ShaderModules_t	= FixedArray< ShaderModule, 8 >;
		using TopologyBits_t	= GraphicsPipelineDesc::TopologyBits_t;
		using VertexAttrib		= VertexInputState::VertexAttrib;
		using VertexAttribs_t	= GraphicsPipelineDesc::VertexAttribs_t;
		using FragmentOutputPtr	= const FragmentOutputInstance *;


	// variables
	private:
		VPipelineLayoutPtr		_layout;
		Instances_t				_instances;
		VFrameGraphWeak			_frameGraph;

		ShaderModules_t			_shaders;
		TopologyBits_t			_supportedTopology;		// TODO: use
		FragmentOutputPtr		_fragmentOutput;
		VertexAttribs_t			_vertexAttribs;
		uint					_patchControlPoints;
		bool					_earlyFragmentTests	= true;


	// methods
	private:
		void _Destroy ();

	public:
		explicit VGraphicsPipeline (const VFrameGraphWeak &fg);
		~VGraphicsPipeline ();

		ND_ PipelineResourcesPtr	CreateResources (const DescriptorSetID &id) FG_OVERRIDE;
		
		ND_ DescriptorSetLayoutPtr	GetDescriptorSet (const DescriptorSetID &id) const FG_OVERRIDE;
		ND_ uint					GetBindingIndex (const DescriptorSetID &id) const;

		ND_ EShaderStages			GetShaderStages () const FG_OVERRIDE;
		
		ND_ VPipelineLayoutPtr		GetLayout ()				const	{ return _layout; }

		ND_ ArrayView<VertexAttrib>	GetVertexAttribs ()			const	{ return _vertexAttribs; }
		ND_ FragmentOutputPtr		GetFragmentOutput ()		const	{ return _fragmentOutput; }

		ND_ uint					GetPatchControlPoints ()	const	{ return _patchControlPoints; }
		ND_ bool					IsEarlyFragmentTests ()		const	{ return _earlyFragmentTests; }
	};



	//
	// Mesh Pipeline
	//

	class VMeshPipeline final : public Pipeline
	{
		friend class VPipelineCache;
		
	// types
	public:
		using FragmentOutputInstance = VGraphicsPipeline::FragmentOutputInstance;

	private:
		struct PipelineInstance
		{
		// variables
			HashVal						_hash;
			VRenderPassPtr				renderPass;
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

		using Instances_t		= HashMap< PipelineInstance, VkPipeline, PipelineInstanceHash >;
		using ShaderModules_t	= FixedArray< ShaderModule, 8 >;
		using FragmentOutputPtr	= const FragmentOutputInstance *;


	// variables
	private:
		VPipelineLayoutPtr		_layout;
		Instances_t				_instances;
		VFrameGraphWeak			_frameGraph;
		
		ShaderModules_t			_shaders;
		FragmentOutputPtr		_fragmentOutput;
		bool					_earlyFragmentTests	= true;


	// methods
	private:
		void _Destroy ();

	public:
		explicit VMeshPipeline (const VFrameGraphWeak &fg);
		~VMeshPipeline ();

		ND_ PipelineResourcesPtr	CreateResources (const DescriptorSetID &id) FG_OVERRIDE;
		
		ND_ DescriptorSetLayoutPtr	GetDescriptorSet (const DescriptorSetID &id) const FG_OVERRIDE;
		ND_ uint					GetBindingIndex (const DescriptorSetID &id) const;
		
		ND_ EShaderStages			GetShaderStages () const FG_OVERRIDE;

		ND_ VPipelineLayoutPtr		GetLayout ()				const	{ return _layout; }
		ND_ FragmentOutputPtr		GetFragmentOutput ()		const	{ return _fragmentOutput; }
		ND_ bool					IsEarlyFragmentTests ()		const	{ return _earlyFragmentTests; }
	};



	//
	// Compute Pipeline
	//

	class VComputePipeline final : public Pipeline
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

		using Instances_t		= HashMap< PipelineInstance, VkPipeline, PipelineInstanceHash >;
		using VkShaderPtr		= PipelineDescription::VkShaderPtr;


	// variables
	private:
		VPipelineLayoutPtr		_layout;
		Instances_t				_instances;
		VFrameGraphWeak			_frameGraph;

		VkShaderPtr				_shader;

		uint3					_defaultLocalGroupSize;
		uint3					_localSizeSpec;


	// methods
	private:
		void _Destroy ();

	public:
		explicit VComputePipeline (const VFrameGraphWeak &fg);
		~VComputePipeline ();

		ND_ PipelineResourcesPtr	CreateResources (const DescriptorSetID &id) FG_OVERRIDE;
		
		ND_ DescriptorSetLayoutPtr	GetDescriptorSet (const DescriptorSetID &id) const FG_OVERRIDE;
		ND_ uint					GetBindingIndex (const DescriptorSetID &id) const;
		
		ND_ EShaderStages			GetShaderStages ()		const FG_OVERRIDE;

		ND_ VPipelineLayoutPtr		GetLayout ()			const	{ return _layout; }

		ND_ uint3 const&			GetDefaultLocalSize ()	const	{ return _defaultLocalGroupSize; }
	};


}	// FG

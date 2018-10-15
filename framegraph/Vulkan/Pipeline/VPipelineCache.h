// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VDescriptorSetLayout.h"
#include "VPipelineLayout.h"
#include "VPipeline.h"
#include "VPipelineResources.h"

namespace FG
{

	//
	// Vulkan Pipeline Cache
	//

	class VPipelineCache final
	{
		friend class VPipelineCacheUnitTest;

	// types
	private:
		struct DescriptorSetItem
		{
			VDescriptorSetLayoutPtr		ptr;

			DescriptorSetItem () {}
			explicit DescriptorSetItem (const VDescriptorSetLayoutPtr &p) : ptr{p} {}

			ND_ bool  operator == (const DescriptorSetItem &rhs) const	{ return *ptr == *rhs.ptr; }
		};

		struct DescriptorSetHash {
			ND_ size_t operator () (const DescriptorSetItem &value) const  { return size_t(value.ptr->GetHash()); }
		};


		struct PipelineLayoutItem
		{
			VPipelineLayoutPtr		ptr;

			PipelineLayoutItem () {}
			explicit PipelineLayoutItem (const VPipelineLayoutPtr &p) : ptr{p} {}

			ND_ bool  operator == (const PipelineLayoutItem &rhs) const	{ return *ptr == *rhs.ptr; }
		};

		struct PipelineLayoutHash {
			ND_ size_t operator () (const PipelineLayoutItem &value) const  { return size_t(value.ptr->GetHash()); }
		};

		struct FragmentOutputHash {
			ND_ size_t operator () (const VGraphicsPipeline::FragmentOutputInstance &value) const  { return size_t(value.GetHash()); }
		};

		static constexpr uint	MaxStages = 8;

		using ShaderModule_t			= VGraphicsPipeline::ShaderModule;
		using DynamicStates_t			= FixedArray< VkDynamicState, 32 >;
		using Viewports_t				= FixedArray< VkViewport, 16 >;
		using Scissors_t				= FixedArray< VkRect2D, 16 >;
		using ShaderStages_t			= FixedArray< VkPipelineShaderStageCreateInfo, MaxStages >;
		using VkShaderPtr				= PipelineDescription::VkShaderPtr;

		using VertexInputAttributes_t	= FixedArray< VkVertexInputAttributeDescription, FG_MaxAttribs >;
		using VertexInputBindings_t		= FixedArray< VkVertexInputBindingDescription, FG_MaxVertexBuffers >;
		using ColorAttachments_t		= FixedArray< VkPipelineColorBlendAttachmentState, FG_MaxColorBuffers >;
		using Specializations_t			= FixedArray< VkSpecializationInfo, MaxStages >;
		using SpecializationEntries_t	= FixedArray< VkSpecializationMapEntry, FG_MaxSpecConstants * MaxStages >;
		using SpecializationData_t		= FixedArray< uint, FG_MaxSpecConstants * MaxStages >;

		using DescriptorSet_t			= PipelineDescription::DescriptorSet;
		using PipelineLayout_t			= PipelineDescription::PipelineLayout;
		using DescriptorSetMap_t		= HashSet< DescriptorSetItem, DescriptorSetHash >;
		using PipelineLayoutMap_t		= HashSet< PipelineLayoutItem, PipelineLayoutHash >;
		using ShaderModules_t			= Array< VkShaderPtr >;
		using PipelineCompilers_t		= HashSet< IPipelineCompilerPtr >;
		using FragmentOutputCache_t		= HashSet< VGraphicsPipeline::FragmentOutputInstance, FragmentOutputHash >;
		using FragmentOutputPtr			= const VGraphicsPipeline::FragmentOutputInstance *;


	// variables
	private:
		VDevice const&			_device;
		VkPipelineCache			_pipelinesCache;

		DescriptorSetMap_t		_descriptorSetCache;
		PipelineLayoutMap_t		_pipelineLayoutCache;
		//GraphicsPipelineMap_t	_graphicsPipelineCache;
		//ComputePipelineMap_t	_computePipelineCache;
		ShaderModules_t			_shaderCache;
		PipelineCompilers_t		_compilers;
		FragmentOutputCache_t	_fragmentOutputCache;
		
		// temporary arrays
		ShaderStages_t			_tempStages;
		Specializations_t		_tempSpecialization;
		DynamicStates_t			_tempDynamicStates;
		Viewports_t				_tempViewports;
		Scissors_t				_tempScissors;
		VertexInputAttributes_t	_tempVertexAttribs;
		VertexInputBindings_t	_tempVertexBinding;
		ColorAttachments_t		_tempAttachments;
		SpecializationEntries_t	_tempSpecEntries;
		SpecializationData_t	_tempSpecData;


	// methods
	public:
		explicit VPipelineCache (const VDevice &dev);
		~VPipelineCache ();
		
		bool Initialize ();
		void Deinitialize ();

		void AddCompiler (const IPipelineCompilerPtr &comp);

		bool InitPipelineResources (const VPipelineResourcesPtr &pr);
		bool UpdatePipelineResources (const VPipelineResourcesPtr &pr, const VPipelineResources::UpdateDescriptors &upd);
		
		ND_ VMeshPipelinePtr		CreatePipeline (const VFrameGraphWeak &fg, MeshPipelineDesc &&desc);
		ND_ VRayTracingPipelinePtr	CreatePipeline (const VFrameGraphWeak &fg, RayTracingPipelineDesc &&desc);
		ND_ VGraphicsPipelinePtr	CreatePipeline (const VFrameGraphWeak &fg, GraphicsPipelineDesc &&desc);
		ND_ VComputePipelinePtr		CreatePipeline (const VFrameGraphWeak &fg, ComputePipelineDesc &&desc);
		
		ND_ VkPipeline	CreatePipelineInstance (const VGraphicsPipelinePtr	&ppln,
												const VRenderPassPtr		&renderPass,
												uint						 subpassIndex,
												const RenderState			&renderState,
												const VertexInputState		&vertexInput,
												EPipelineDynamicState		 dynamicState,
												VkPipelineCreateFlags		 pipelineFlags,
												uint						 viewportCount);

		ND_ VkPipeline	CreatePipelineInstance (const VComputePipelinePtr	&ppln,
												const Optional<uint3>		&localGroupSize,
												VkPipelineCreateFlags		 pipelineFlags);
		
		ND_ VkPipeline	CreatePipelineInstance (const VMeshPipelinePtr		&ppln,
												VkPipelineCreateFlags		 pipelineFlags);

		ND_ VkPipeline	CreatePipelineInstance (const VRayTracingPipelinePtr	&ppln,
												VkPipelineCreateFlags			 pipelineFlags);


	private:
		ND_ VDescriptorSetLayoutPtr	_CreateDescriptorSetLayout (DescriptorSet_t &&);
		ND_ VPipelineLayoutPtr		_CreatePipelineLayout (PipelineLayout_t &&);
		
		ND_ VGraphicsPipelinePtr	_CreatePipeline (const VFrameGraphWeak &fg, GraphicsPipelineDesc &&desc);
		ND_ VComputePipelinePtr		_CreatePipeline (const VFrameGraphWeak &fg, ComputePipelineDesc &&desc);

		ND_ FragmentOutputPtr		_CreateFramentOutput (ArrayView<GraphicsPipelineDesc::FragmentOutput> values);

		ND_ FixedArray<EShaderLangFormat,16>	_GetBuiltinFormats () const;

		void _Destroy ();
		void _ClearTemp ();

		bool _CompileShader (const PipelineDescription::ShaderDataMap_t &shaders,
							 OUT VkShaderPtr &module);
		
		void _SetColorBlendState (OUT VkPipelineColorBlendStateCreateInfo &outState,
								  OUT ColorAttachments_t &attachments,
								  const RenderState::ColorBuffersState &inState,
								  const VRenderPassPtr &renderPass,
								  uint subpassIndex) const;

		void _SetShaderStages (OUT ShaderStages_t &stages,
							   INOUT Specializations_t &specialization,
							   INOUT SpecializationEntries_t &specEntries,
							   ArrayView<ShaderModule_t> shaders) const;

		void _SetDynamicState (OUT VkPipelineDynamicStateCreateInfo &outState,
							   OUT DynamicStates_t &states,
							   EPipelineDynamicState inState) const;

		void _SetMultisampleState (OUT VkPipelineMultisampleStateCreateInfo &outState,
								   const RenderState::MultisampleState &inState) const;

		void _SetTessellationState (OUT VkPipelineTessellationStateCreateInfo &outState,
									uint patchSize) const;

		void _SetDepthStencilState (OUT VkPipelineDepthStencilStateCreateInfo &outState,
									const RenderState::DepthBufferState &depth,
									const RenderState::StencilBufferState &stencil) const;

		void _SetRasterizationState (OUT VkPipelineRasterizationStateCreateInfo &outState,
									 const RenderState::RasterizationState &inState) const;

		void _SetupPipelineInputAssemblyState (OUT VkPipelineInputAssemblyStateCreateInfo &outState,
											   const RenderState::InputAssemblyState &inState) const;

		void _SetVertexInputState (OUT VkPipelineVertexInputStateCreateInfo &outState,
								   OUT VertexInputAttributes_t &vertexAttribs,
								   OUT VertexInputBindings_t &vertexBinding,
								   const VertexInputState &inState) const;

		void _SetViewportState (OUT VkPipelineViewportStateCreateInfo &outState,
								OUT Viewports_t &tmpViewports,
								OUT Scissors_t &tmpScissors,
								const uint2 &viewportSize,
								uint viewportCount,
								EPipelineDynamicState dynamicStates) const;

		void _AddLocalGroupSizeSpecialization (INOUT SpecializationEntries_t &outEntries,
											   INOUT SpecializationData_t &outEntryData,
											   const uint3 &localSizeSpec,
											   const uint3 &localGroupSize) const;

		void _ValidateRenderState (INOUT RenderState &renderState, INOUT EPipelineDynamicState &dynamicStates) const;
	};


}	// FG

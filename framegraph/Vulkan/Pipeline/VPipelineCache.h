// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraphTask.h"
#include "VDescriptorSetLayout.h"
#include "VPipelineLayout.h"
#include "VGraphicsPipeline.h"
#include "VComputePipeline.h"
#include "VMeshPipeline.h"
#include "VRayTracingPipeline.h"

namespace FG
{

	//
	// Pipeline Cache
	//

	class VPipelineCache final
	{
	// types
	private:
		struct FragmentOutputHash {
			ND_ size_t  operator () (const VGraphicsPipeline::FragmentOutputInstance &value) const {
				return size_t(value.GetHash());
			}
		};

		using PipelineCompilers_t		= HashSet< IPipelineCompilerPtr >;
		using VkShaderPtr				= PipelineDescription::VkShaderPtr;
		using ShaderModules_t			= Array< VkShaderPtr >;
		using FragmentOutputCache_t		= HashSet< VGraphicsPipeline::FragmentOutputInstance, FragmentOutputHash >;
		using FragmentOutputPtr			= const VGraphicsPipeline::FragmentOutputInstance *;
		using ShaderModule_t			= VGraphicsPipeline::ShaderModule;

		template <typename T>
		struct PipelineInstancePairHash {
			ND_ size_t  operator () (const Pair<T const*, typename T::PipelineInstance> &value) const {
				return size_t(HashOf(value.first) + value.second._hash);
			}
		};

		template <typename T>
		using PipelineInstanceCacheTempl = std::unordered_map< Pair<T const*, typename T::PipelineInstance>, VkPipeline,
															   PipelineInstancePairHash<T>, std::equal_to<Pair<T const*, typename T::PipelineInstance>>,
															   StdLinearAllocator<Pair< const Pair<T const*, typename T::PipelineInstance>, VkPipeline >> >;

		using CPipelineInstanceMap_t	= InPlace< PipelineInstanceCacheTempl< VComputePipeline > >;
		using GPipelineInstanceMap_t	= InPlace< PipelineInstanceCacheTempl< VGraphicsPipeline > >;
		using MPipelineInstanceMap_t	= InPlace< PipelineInstanceCacheTempl< VMeshPipeline > >;
		
		using DynamicStates_t			= FixedArray< VkDynamicState, 32 >;
		using Viewports_t				= FixedArray< VkViewport, 16 >;
		using Scissors_t				= FixedArray< VkRect2D, 16 >;
		using VertexInputAttributes_t	= FixedArray< VkVertexInputAttributeDescription, FG_MaxVertexAttribs >;
		using VertexInputBindings_t		= FixedArray< VkVertexInputBindingDescription, FG_MaxVertexBuffers >;
		using ColorAttachments_t		= FixedArray< VkPipelineColorBlendAttachmentState, FG_MaxColorBuffers >;
		using Specializations_t			= Array< VkSpecializationInfo >;
		using SpecializationEntries_t	= Array< VkSpecializationMapEntry >;
		using SpecializationData_t		= Array< uint >;
		using ShaderStages_t			= Array< VkPipelineShaderStageCreateInfo >;
		using RTShaderGroups_t			= Array< VkRayTracingShaderGroupCreateInfoNV >;
		
		using EGroupType				= UpdateRayTracingShaderTable::EGroupType;
		using RayGenShader				= UpdateRayTracingShaderTable::RayGenShader;
		using RTShaderGroup				= UpdateRayTracingShaderTable::ShaderGroup;
		
		struct RTShaderGroupHash {
			ND_ size_t  operator () (const RTShaderGroup *value) const noexcept {
				return size_t(HashOf( value->type ) + HashOf( value->mainShader ) + HashOf( value->anyHitShader ) + HashOf( value->intersectionShader ));
			}
		};
		struct RTShaderGroupEqual {
			ND_ bool  operator () (const RTShaderGroup *lhs, const RTShaderGroup *rhs) const noexcept {
				return	lhs->type == rhs->type and lhs->mainShader == rhs->mainShader and
						lhs->anyHitShader == rhs->anyHitShader and lhs->intersectionShader == rhs->intersectionShader;
			}
		};
		using RTShaderGroupMap_t		= std::unordered_map< RTShaderGroup const*, uint, RTShaderGroupHash, RTShaderGroupEqual >;

		struct RTShaderSpec
		{
			SpecializationID	id;
			uint				offset	= UMax;
		};
		using RTShaderSpecializations_t	= FixedArray< RTShaderSpec, 32 >;


	public:
		struct BufferCopyRegion
		{
			VLocalBuffer const*		srcBuffer	= null;
			VLocalBuffer const*		dstBuffer	= null;
			VkBufferCopy			region		{};
		};
		using BufferCopyRegions_t = FixedArray< BufferCopyRegion, 8 >;


	// variables
	private:
		VkPipelineCache				_pipelinesCache;
		FragmentOutputCache_t		_fragmentOutputCache;
		
		ShaderModules_t				_shaderCache;
		PipelineCompilers_t			_compilers;

		// temporary arrays
		ShaderStages_t				_tempStages;			// TODO: use custom allocator?
		Specializations_t			_tempSpecialization;
		DynamicStates_t				_tempDynamicStates;
		Viewports_t					_tempViewports;
		Scissors_t					_tempScissors;
		VertexInputAttributes_t		_tempVertexAttribs;
		VertexInputBindings_t		_tempVertexBinding;
		ColorAttachments_t			_tempAttachments;
		SpecializationEntries_t		_tempSpecEntries;
		SpecializationData_t		_tempSpecData;
		RTShaderGroups_t			_tempShaderGroups;
		RTShaderGroupMap_t			_tempShaderGraphMap;
		RTShaderSpecializations_t	_rtShaderSpecs;


	// methods
	public:
		VPipelineCache ();
		~VPipelineCache ();
		
		bool Initialize (const VDevice &dev);
		void Deinitialize (const VDevice &dev);

		bool MergeCache (VPipelineCache &);
		
		void AddCompiler (const IPipelineCompilerPtr &comp);

		bool CompileShaders (INOUT GraphicsPipelineDesc &desc, const VDevice &dev);
		bool CompileShaders (INOUT MeshPipelineDesc &desc, const VDevice &dev);
		bool CompileShader (INOUT ComputePipelineDesc &desc, const VDevice &dev);
		bool CompileShaders (INOUT RayTracingPipelineDesc &desc, const VDevice &dev);

		ND_ FragmentOutputPtr  CreateFramentOutput (ArrayView<GraphicsPipelineDesc::FragmentOutput> values);

		bool CreatePipelineInstance (VResourceManagerThread			&resMngr,
									 Ptr<VShaderDebugger>			 shaderDebugger,
									 const VLogicalRenderPass		&logicalRP,
									 const VBaseDrawVerticesTask	&drawTask,
									 OUT VkPipeline					&outPipeline,
									 OUT VPipelineLayout const*		&outLayout);
		
		bool CreatePipelineInstance (VResourceManagerThread			&resMngr,
									 Ptr<VShaderDebugger>			 shaderDebugger,
									 const VLogicalRenderPass		&logicalRP,
									 const VBaseDrawMeshes			&drawTask,
									 OUT VkPipeline					&outPipeline,
									 OUT VPipelineLayout const*		&outLayout);

		bool CreatePipelineInstance (VResourceManagerThread			&resMngr,
									 Ptr<VShaderDebugger>			 shaderDebugger,
									 const VComputePipeline			&ppln,
									 const Optional<uint3>			&localGroupSize,
									 VkPipelineCreateFlags			 pipelineFlags,
									 uint							 debugModeIndex,
									 OUT VkPipeline					&outPipeline,
									 OUT VPipelineLayout const*		&outLayout);

		bool InitShaderTable (VFrameGraphThread				&fg,
							  RawRTPipelineID				 pipelineId,
							  VLocalRTScene const*			rtScene,
							  const RayGenShader			&rayGenShader,
							  ArrayView< RTShaderGroup >	 shaderGroups,
							  uint							 maxRecursionDepth,
							  INOUT VRayTracingShaderTable	&shaderTable,
							  OUT BufferCopyRegions_t		&copyRegions);


	private:
		ND_ FixedArray<EShaderLangFormat,16>	_GetBuiltinFormats (const VDevice &dev) const;
		
		template <typename T>
		void _MergePipelines (INOUT InPlace<PipelineInstanceCacheTempl<T>> &, OUT AppendableVkResources_t) const;

		template <typename DescT>
		bool _CompileShaders (INOUT DescT &desc, const VDevice &dev);
		bool _CompileSPIRVShader (const VDevice &dev, const PipelineDescription::ShaderDataUnion_t &shaderData, OUT VkShaderPtr &module);

		bool _CreatePipelineCache (const VDevice &dev);

		template <typename Pipeline>
		bool _SetupShaderDebugging (VResourceManagerThread &resMngr, VShaderDebugger &shaderDebugger, const Pipeline &ppln, uint debugModeIndex,
									OUT EShaderDebugMode &debugMode, OUT EShaderStages &debuggableShaders, OUT RawPipelineLayoutID &layoutId);

		void _ClearTemp ();

		void _SetColorBlendState (OUT VkPipelineColorBlendStateCreateInfo &outState,
								  OUT ColorAttachments_t &attachments,
								  const RenderState::ColorBuffersState &inState,
								  const VRenderPass &renderPass,
								  uint subpassIndex) const;

		void _SetShaderStages (OUT ShaderStages_t &stages,
							   INOUT Specializations_t &specialization,
							   INOUT SpecializationEntries_t &specEntries,
							   ArrayView<ShaderModule_t> shaders,
							   EShaderDebugMode debugMode,
							   EShaderStages debuggableShaders) const;

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
		
		bool _InitShaderStage (const VRayTracingPipeline *ppln, const RTShaderID &id, EShaderDebugMode mode,
							   OUT VkPipelineShaderStageCreateInfo &stage);
		
		bool _GetShaderGroup (const VRayTracingPipeline *ppln, const RTShaderGroup &group, EShaderDebugMode mode,
								OUT VkRayTracingShaderGroupCreateInfoNV &group_ci);

		void _ValidateRenderState (const VDevice &dev, INOUT RenderState &renderState, INOUT EPipelineDynamicState &dynamicStates) const;
	};


}	// FG

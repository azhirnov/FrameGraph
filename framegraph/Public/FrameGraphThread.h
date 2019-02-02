// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	FrameGraphThread - framegraph interface to use in separate thread.
	There are two different modes:
		asynchronous --	between FrameGraphInstance::BeginFrame() and FrameGraphInstance::EndFrame() calls. Frame recording happens in asynchronous mode.
						Resource creation are available, but to make new resource data visible for other threads you must add memory barrier.
						Methods 'ReleaseResource' does not delete resources until frame execution is complete.

		synchronous --	after FrameGraphInstance::EndFrame() and before FrameGraphInstance::BeginFrame() calls.
						'Begin', 'Execute', all 'AddTask' and all 'Acquire' methods are not available.
						All 'Create*', all 'ReleaseResource' and 'RecreateSwapchain' methods must be externally synchronized with all other threads.
*/

#pragma once

#include "framegraph/Public/FrameGraphTask.h"
#include "framegraph/Public/FrameGraphDrawTask.h"
#include "framegraph/Public/FGEnums.h"
#include "framegraph/Public/VulkanTypes.h"
#include "framegraph/Public/Pipeline.h"
#include "framegraph/Public/BufferDesc.h"
#include "framegraph/Public/ImageDesc.h"
#include "framegraph/Public/SamplerDesc.h"
#include "framegraph/Public/MemoryDesc.h"
#include "framegraph/Public/RenderPassDesc.h"
#include "framegraph/Public/PipelineResources.h"

namespace FG
{

	//
	// Thread Description
	//
	
	struct ThreadDesc
	{
		EThreadUsage	usage;
		StringView		name;

		ThreadDesc () {}
		explicit ThreadDesc (EThreadUsage usage, StringView name = Default) : usage{usage}, name{name} {}
	};



	//
	// Frame Graph Thread interface
	//

	class FrameGraphThread : public std::enable_shared_from_this<FrameGraphThread>
	{
	// types
	public:
		using SwapchainCreateInfo	= Union< NullUnion, VulkanSwapchainCreateInfo, VulkanVREmulatorSwapchainCreateInfo >;
		using ExternalImageDesc		= Union< NullUnion, VulkanImageDesc >;
		using ExternalBufferDesc	= Union< NullUnion, VulkanBufferDesc >;
		using ExternalImage_t		= Union< NullUnion, ImageVk_t >;
		using ExternalBuffer_t		= Union< NullUnion, BufferVk_t >;

		using OnExternalImageReleased_t		= std::function< void (const ExternalImage_t &) >;
		using OnExternalBufferReleased_t	= std::function< void (const ExternalBuffer_t &) >;
		using ShaderDebugCallback_t			= std::function< void (StringView taskName, StringView shaderName, EShaderStages, ArrayView<String> output) >;


	// interface
	public:
			
	// resource manager //

			// Create resources: pipeline, image, buffer, etc.
			// See synchronization requirements on top of this file.
		ND_ virtual MPipelineID		CreatePipeline (INOUT MeshPipelineDesc &desc, StringView dbgName = Default) = 0;
		ND_ virtual RTPipelineID	CreatePipeline (INOUT RayTracingPipelineDesc &desc, StringView dbgName = Default) = 0;
		ND_ virtual GPipelineID		CreatePipeline (INOUT GraphicsPipelineDesc &desc, StringView dbgName = Default) = 0;
		ND_ virtual CPipelineID		CreatePipeline (INOUT ComputePipelineDesc &desc, StringView dbgName = Default) = 0;
		ND_ virtual ImageID			CreateImage (const ImageDesc &desc, const MemoryDesc &mem = Default, StringView dbgName = Default) = 0;
		ND_ virtual BufferID		CreateBuffer (const BufferDesc &desc, const MemoryDesc &mem = Default, StringView dbgName = Default) = 0;
		ND_ virtual ImageID			CreateImage (const ExternalImageDesc &desc, OnExternalImageReleased_t &&, StringView dbgName = Default) = 0;
		ND_ virtual BufferID		CreateBuffer (const ExternalBufferDesc &desc, OnExternalBufferReleased_t &&, StringView dbgName = Default) = 0;
		ND_ virtual SamplerID		CreateSampler (const SamplerDesc &desc, StringView dbgName = Default) = 0;
		ND_ virtual RTGeometryID	CreateRayTracingGeometry (const RayTracingGeometryDesc &desc, const MemoryDesc &mem = Default, StringView dbgName = Default) = 0;
		ND_ virtual RTSceneID		CreateRayTracingScene (const RayTracingSceneDesc &desc, const MemoryDesc &mem = Default, StringView dbgName = Default) = 0;
			virtual bool			InitPipelineResources (const RawGPipelineID &pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const = 0;
			virtual bool			InitPipelineResources (const RawCPipelineID &pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const = 0;
			virtual bool			InitPipelineResources (const RawMPipelineID &pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const = 0;
			virtual bool			InitPipelineResources (const RawRTPipelineID &pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const = 0;
		
			template <typename Pipeline>
			inline bool				InitPipelineResources (const Pipeline &pplnId, const DescriptorSetID &id, OUT PipelineResources &resources) const { return InitPipelineResources( pplnId.Get(), id, OUT resources ); }

			// Release reference to resource, if reference counter is 0 then resource will be destroyed after frame execution.
			// See synchronization requirements on top of this file.
			virtual void			ReleaseResource (INOUT GPipelineID &id) = 0;
			virtual void			ReleaseResource (INOUT CPipelineID &id) = 0;
			virtual void			ReleaseResource (INOUT MPipelineID &id) = 0;
			virtual void			ReleaseResource (INOUT RTPipelineID &id) = 0;
			virtual void			ReleaseResource (INOUT ImageID &id) = 0;
			virtual void			ReleaseResource (INOUT BufferID &id) = 0;
			virtual void			ReleaseResource (INOUT SamplerID &id) = 0;
			virtual void			ReleaseResource (INOUT RTGeometryID &id) = 0;
			virtual void			ReleaseResource (INOUT RTSceneID &id) = 0;

		ND_ virtual BufferDesc const&	GetDescription (const BufferID &id) const = 0;
		ND_ virtual ImageDesc const&	GetDescription (const ImageID &id) const = 0;
		//ND_ virtual SamplerDesc const&	GetDescription (const SamplerID &id) const = 0;

		ND_ virtual EThreadUsage	GetThreadUsage () const = 0;

			// Returns 'true' if both threads use same command queue.
			// Both threads must be externally synchronized.
		ND_ virtual bool			IsCompatibleWith (const FGThreadPtr &thread, EThreadUsage usage) const = 0;


	// initialization //

			// Initialize framegraph thread.
			// Current thread will use same GPU queues as 'relativeThreads' and
			// will try not to use same GPU queues as 'parallelThreads'.
			// Must be externally synchronized with all framegraph threads.
			virtual bool		Initialize (const SwapchainCreateInfo *swapchainCI = null,
											ArrayView<FGThreadPtr> relativeThreads = Default,
											ArrayView<FGThreadPtr> parallelThreads = Default) = 0;

			// Deinitialize framegraph thread and all dependent systems.
			// Must be externally synchronized with all framegraph threads.
			virtual void		Deinitialize () = 0;

			// Set framegraph compilation flags only for this thread.
			virtual bool		SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags = Default) = 0;

			// Recreate or create swapchain.
			// Must be externally synchronized with all framegraph threads.
			virtual bool		RecreateSwapchain (const SwapchainCreateInfo &) = 0;

			// Callback will be called at end of the frame if debugging enabled by
			// calling 'Task::EnableDebugTrace' and shader compiled with 'DebugTrace' mode.
			virtual bool		SetShaderDebugCallback (ShaderDebugCallback_t &&) = 0;


	// frame execution //

			// Starts framegraph subbatch recording.
			// Available only in asynchronous mode.
			// 'batchId' and 'indexInBatch' must be unique.
			virtual bool		Begin (const CommandBatchID &id, uint index, EThreadUsage usage) = 0;

			// Compile framegraph and add subbatch to the submission graph.
			// Current thread must be in recording state.
			// Available only in asynchronous mode.
		ND_ virtual bool		Execute () = 0;


	// resource acquiring //

			// Acquire resource or specific data range in mutable or immutable state.
			// Must be used between 'Begin' and 'Execute' calls.
			//virtual bool		Acquire (const ImageID &id, bool immutable = true) = 0;
			//virtual bool		Acquire (const ImageID &id, MipmapLevel baseLevel, uint levelCount, ImageLayer baseLayer, uint layerCount, bool immutable = true) = 0;
			//virtual bool		Acquire (const BufferID &id, bool immutable = true) = 0;
			//virtual bool		Acquire (const BufferID &id, BytesU offset, BytesU size, bool immutable = true) = 0;

		//ND_ virtual ImageID		GetSwapchainImage (ESwapchainImage type) = 0;

			virtual bool		UpdateHostBuffer (const BufferID &id, BytesU offset, BytesU size, const void *data) = 0;
			virtual bool		MapBufferRange (const BufferID &id, BytesU offset, INOUT BytesU &size, OUT void* &data) = 0;

	// tasks //

			// Add task to the frame graph subbatch.
			// Must be used between 'Begin' and 'Execute' calls.
		ND_ virtual Task		AddTask (const SubmitRenderPass &) = 0;
		ND_ virtual Task		AddTask (const DispatchCompute &) = 0;
		ND_ virtual Task		AddTask (const DispatchComputeIndirect &) = 0;
		ND_ virtual Task		AddTask (const CopyBuffer &) = 0;
		ND_ virtual Task		AddTask (const CopyImage &) = 0;
		ND_ virtual Task		AddTask (const CopyBufferToImage &) = 0;
		ND_ virtual Task		AddTask (const CopyImageToBuffer &) = 0;
		ND_ virtual Task		AddTask (const BlitImage &) = 0;
		ND_ virtual Task		AddTask (const ResolveImage &) = 0;
		ND_ virtual Task		AddTask (const FillBuffer &) = 0;
		ND_ virtual Task		AddTask (const ClearColorImage &) = 0;
		ND_ virtual Task		AddTask (const ClearDepthStencilImage &) = 0;
		ND_ virtual Task		AddTask (const UpdateBuffer &) = 0;
		ND_ virtual Task		AddTask (const UpdateImage &) = 0;
		ND_ virtual Task		AddTask (const ReadBuffer &) = 0;
		ND_ virtual Task		AddTask (const ReadImage &) = 0;
		ND_ virtual Task		AddTask (const Present &) = 0;
		//ND_ virtual Task		AddTask (const PresentVR &) = 0;
		ND_ virtual Task		AddTask (const UpdateRayTracingShaderTable &) = 0;
		ND_ virtual Task		AddTask (const BuildRayTracingGeometry &) = 0;
		ND_ virtual Task		AddTask (const BuildRayTracingScene &) = 0;
		ND_ virtual Task		AddTask (const TraceRays &) = 0;


	// draw tasks //

			// Creates new render pass.
			// Must be used between 'Begin' and 'Execute' calls.
		ND_ virtual LogicalPassID  CreateRenderPass (const RenderPassDesc &desc) = 0;

			// Add task to the render pass.
			// Must be used between 'Begin' and 'Execute' calls.
			virtual void		AddTask (LogicalPassID, const DrawVertices &) = 0;
			virtual void		AddTask (LogicalPassID, const DrawIndexed &) = 0;
			virtual void		AddTask (LogicalPassID, const DrawVerticesIndirect &) = 0;
			virtual void		AddTask (LogicalPassID, const DrawIndexedIndirect &) = 0;
			virtual void		AddTask (LogicalPassID, const DrawMeshes &) = 0;
			virtual void		AddTask (LogicalPassID, const DrawMeshesIndirect &) = 0;
		//	virtual void		AddTask (LogicalPassID, const ClearAttachments &) = 0;
	};


}	// FG

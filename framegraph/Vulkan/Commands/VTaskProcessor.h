// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VTaskGraph.h"
#include "VBarrierManager.h"

namespace FG
{

	//
	// Task Processor
	//

	class VTaskProcessor final
	{
	// types
	public:
		class DrawTaskBarriers;
		class DrawTaskCommands;
		class PipelineResourceBarriers;
		
		using VkDescriptorSets_t	= FixedArray< VkDescriptorSet, FG_MaxDescriptorSets >;
		using VkClearValues_t		= FixedArray< VkClearValue, FG_MaxColorBuffers+1 >;

		using BufferRange			= VBuffer::BufferRange;
		using ImageRange			= VImage::ImageRange;
		using BufferState			= VBuffer::BufferState;
		using ImageState			= VImage::ImageState;


	// variables
	private:
		VFrameGraph &				_frameGraph;
		VDevice const&				_dev;

		VkCommandBuffer				_cmdBufferId;
		VCommandQueue *				_currQueue;
		VBarrierManager				_barrierMngr;

		HashSet< VBufferPtr >		_pendingBufferBarriers;
		HashSet< VImagePtr >		_pendingImageBarriers;

		Task						_currTask	= null;
		bool						_isCompute	= false;

		bool						_isDebugMarkerSupported	= false;


	// methods
	public:
		explicit VTaskProcessor (VFrameGraph &fg);
		~VTaskProcessor ();

		void Run (Task node);

		void Visit (const VFgTask<SubmitRenderPass> &task);
		void Visit (const VFgTask<DispatchCompute> &task);
		void Visit (const VFgTask<DispatchIndirectCompute> &task);
		void Visit (const VFgTask<CopyBuffer> &task);
		void Visit (const VFgTask<CopyImage> &task);
		void Visit (const VFgTask<CopyBufferToImage> &task);
		void Visit (const VFgTask<CopyImageToBuffer> &task);
		void Visit (const VFgTask<BlitImage> &);
		void Visit (const VFgTask<ResolveImage> &);
		void Visit (const VFgTask<FillBuffer> &);
		void Visit (const VFgTask<ClearColorImage> &);
		void Visit (const VFgTask<ClearDepthStencilImage> &);
		void Visit (const VFgTask<UpdateBuffer> &);
		void Visit (const VFgTask<Present> &task);
		void Visit (const VFgTask<PresentVR> &task);
		void Visit (const VFgTask<TaskGroupSync> &)	{}

		static void Visit1_DrawTask (void *visitor, void *taskData);
		static void Visit2_DrawTask (void *visitor, void *taskData);
		static void Visit1_DrawIndexedTask (void *visitor, void *taskData);
		static void Visit2_DrawIndexedTask (void *visitor, void *taskData);

	private:
		void _CmdDebugMarker (StringView text) const;
		void _OnRunTask (VCommandQueue const* queue, const IFrameGraphTask *task) const;

		void _CommitBarriers ();
		
		void _BindPipelineResources (const VComputePipelinePtr &pipeline, const VPipelineResourceSet &resources) const;
		void _BindPipeline (const VComputePipelinePtr &pipeline, const Optional<uint3> &localSize) const;

		void _AddRenderTargetBarriers (const VLogicalRenderPass *logicalRP, const DrawTaskBarriers &info);
		void _ExtractClearValues (const VLogicalRenderPass *logicalRP, OUT VkClearValues_t &clearValues) const;
		void _BeginRenderPass (const VFgTask<SubmitRenderPass> &task);
		void _BeginSubpass (const VFgTask<SubmitRenderPass> &task);

		void _AddImage (const VImagePtr &img, EResourceState state, VkImageLayout layout, const ImageViewDesc &desc);
		void _AddImage (const VImagePtr &img, EResourceState state, VkImageLayout layout, const VkImageSubresourceLayers &subresLayers);
		void _AddImage (const VImagePtr &img, EResourceState state, VkImageLayout layout, const VkImageSubresourceRange &subres);
		void _AddImageState (const VImagePtr &img, const ImageState &state);

		void _AddBuffer (const VBufferPtr &buf, EResourceState state, VkDeviceSize offset, VkDeviceSize size);
		void _AddBuffer (const VBufferPtr &buf, EResourceState state, const VkBufferImageCopy &reg, const VImagePtr &img);
		void _AddBufferState (const VBufferPtr &buf, const BufferState &state);
	};
	

/*
=================================================
	Run
=================================================
*/
	forceinline void VTaskProcessor::Run (Task node)
	{
		// reset states
		_currTask	= node;
		_isCompute	= false;

		node->Process( this );
	}

}	// FG

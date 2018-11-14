// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VTaskGraph.h"
#include "VLocalBuffer.h"
#include "VLocalImage.h"
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

		using BufferRange				= VLocalBuffer::BufferRange;
		using ImageRange				= VLocalImage::ImageRange;
		using BufferState				= VLocalBuffer::BufferState;
		using ImageState				= VLocalImage::ImageState;
		
		using VkClearValues_t			= FixedArray< VkClearValue, FG_MaxColorBuffers+1 >;

		template <typename T>
		using PendingBarriersTempl		= std::unordered_set< T const*, std::hash<T const*>, std::equal_to<T const*>, StdLinearAllocator<T const*> >;	// TODO: use temp allocator

		using PendingBufferBarriers_t	= PendingBarriersTempl< VLocalBuffer >;
		using PendingImageBarriers_t	= PendingBarriersTempl< VLocalImage >;
		
		using BufferCopyRegions_t		= FixedArray< VkBufferCopy, FG_MaxCopyRegions >;
		using ImageCopyRegions_t		= FixedArray< VkImageCopy, FG_MaxCopyRegions >;
		using BufferImageCopyRegions_t	= FixedArray< VkBufferImageCopy, FG_MaxCopyRegions >;
		using BlitRegions_t				= FixedArray< VkImageBlit, FG_MaxBlitRegions >;
		using ResolveRegions_t			= FixedArray< VkImageResolve, FG_MaxResolveRegions >;
		using ImageClearRanges_t		= FixedArray< VkImageSubresourceRange, FG_MaxClearRanges >;


	// variables
	private:
		VFrameGraphThread &			_frameGraph;
		VDevice const &				_dev;

		VkCommandBuffer				_cmdBuffer;
		
		Task						_currTask;
		bool						_isCompute				= false;
		bool						_isDebugMarkerSupported	= false;

		PendingBufferBarriers_t		_pendingBufferBarriers;
		PendingImageBarriers_t		_pendingImageBarriers;
		VBarrierManager &			_barrierMngr;


	// methods
	public:
		VTaskProcessor (VFrameGraphThread &fg, VBarrierManager &barrierMngr, VkCommandBuffer cmd,
						const CommandBatchID &batchId, uint indexInBatch);
		~VTaskProcessor ();

		void Visit (const VFgTask<SubmitRenderPass> &);
		void Visit (const VFgTask<DispatchCompute> &);
		void Visit (const VFgTask<DispatchIndirectCompute> &) {}
		void Visit (const VFgTask<CopyBuffer> &);
		void Visit (const VFgTask<CopyImage> &);
		void Visit (const VFgTask<CopyBufferToImage> &);
		void Visit (const VFgTask<CopyImageToBuffer> &);
		void Visit (const VFgTask<BlitImage> &);
		void Visit (const VFgTask<ResolveImage> &);
		void Visit (const VFgTask<FillBuffer> &);
		void Visit (const VFgTask<ClearColorImage> &);
		void Visit (const VFgTask<ClearDepthStencilImage> &);
		void Visit (const VFgTask<UpdateBuffer> &);
		void Visit (const VFgTask<Present> &);
		void Visit (const VFgTask<PresentVR> &);
		void Visit (const VFgTask<TaskGroupSync> &) {}

		static void Visit1_DrawTask (void *, void *);
		static void Visit2_DrawTask (void *, void *);
		static void Visit1_DrawIndexedTask (void *, void *);
		static void Visit2_DrawIndexedTask (void *, void *);

		void Run (Task);


	private:
		void _CmdDebugMarker (StringView text) const;
		void _CmdPushDebugGroup (StringView text) const;
		void _CmdPopDebugGroup () const;

		void _OnRunTask (const IFrameGraphTask *task) const;
		
		template <typename ID>	ND_ auto const*  _GetState (ID id) const;
		template <typename ID>	ND_ auto const*  _GetResource (ID id) const;
		
		void _CommitBarriers ();
		
		void _AddRenderTargetBarriers (const VLogicalRenderPass *logicalRP, const DrawTaskBarriers &info);
		void _ExtractClearValues (const VLogicalRenderPass *logicalRP, const VRenderPass *rp, OUT VkClearValues_t &clearValues) const;
		void _BeginRenderPass (const VFgTask<SubmitRenderPass> &task);
		void _BeginSubpass (const VFgTask<SubmitRenderPass> &task);

		void _ExtractDescriptorSets (const VPipelineResourceSet &, OUT VkDescriptorSets_t &);
		void _BindPipelineResources (RawCPipelineID pipeline, const VPipelineResourceSet &resourceSet);
		void _BindPipeline (RawCPipelineID pipeline, const Optional<uint3> &localSize) const;

		void _AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, const ImageViewDesc &desc);
		void _AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, const VkImageSubresourceLayers &subresLayers);
		void _AddImage (const VLocalImage *img, EResourceState state, VkImageLayout layout, const VkImageSubresourceRange &subres);
		void _AddImageState (const VLocalImage *img, const ImageState &state);

		void _AddBuffer (const VLocalBuffer *buf, EResourceState state, VkDeviceSize offset, VkDeviceSize size);
		void _AddBuffer (const VLocalBuffer *buf, EResourceState state, const VkBufferImageCopy &reg, const VLocalImage *img);
		void _AddBufferState (const VLocalBuffer *buf, const BufferState &state);
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

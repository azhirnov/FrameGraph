// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Access to the command buffer must be externally synchronized.
*/

#pragma once

#include "framegraph/Public/FrameGraphTask.h"
#include "framegraph/Public/FrameGraphDrawTask.h"
#include "framegraph/Public/DrawCommandBuffer.h"
#include "framegraph/Public/RenderPassDesc.h"
#include "framegraph/Public/CommandBufferPtr.h"
#include "framegraph/Public/FGEnums.h"

namespace FG
{

	//
	// Command Buffer Description
	//
	
	struct CommandBufferDesc
	{
		EQueueType		queueType					= EQueueType::Graphics;
		BytesU			hostWritableBufferSize		= 256_Mb;
		BytesU			hostReadableBufferSize		= 256_Mb;
		EBufferUsage	hostWritebleBufferUsage		= EBufferUsage::TransferSrc;
		//bool			immutableResources			= true;		// all resources except render targets and storage buffer/image will be immutable
		//bool			submitImmediately			= true;		// set 'false' to merge commands into some betches
		EDebugFlags		debugFlags					= Default;
		StringView		name;
		
				 CommandBufferDesc () {}
		explicit CommandBufferDesc (EQueueType type) : queueType{type} {}

		CommandBufferDesc&  SetHostWritableBufferSize (BytesU value)		{ hostWritableBufferSize = value;  return *this; }
		CommandBufferDesc&  SetHostReadableBufferSize (BytesU value)		{ hostReadableBufferSize = value;  return *this; }
		CommandBufferDesc&  SetHostWritableBufferUsage (EBufferUsage value)	{ hostWritebleBufferUsage = value;  return *this; }
		CommandBufferDesc&  SetDebugFlags (EDebugFlags value)				{ debugFlags = value;  return *this; }
		CommandBufferDesc&  SetDebugName (StringView value)					{ name = value;  return *this; }
	};



	//
	// Command Buffer interface
	//

	class ICommandBuffer
	{
	// types
	public:
		using ExternalCmdBatch_t	= Union< NullUnion, VulkanCommandBatch >;


	// interface
	public:
		ND_ virtual FrameGraph	GetFrameGraph () = 0;

		// Acquire next swapchain image. This image will be presented after command buffer execution.
		// Do not use this image in any other command buffers.
		ND_ virtual RawImageID	GetSwapchainImage (RawSwapchainID swapchain, ESwapchainImage type = ESwapchainImage::Primary) = 0;

		// External command buffers will be executed in same batch but before internal command buffer.
		virtual bool		AddExternalCommands (const ExternalCmdBatch_t &) = 0;

		// Add input dependency.
		// Current command buffer will be executed on the GPU only after all input dependencies.
		virtual bool		AddDependency (const CommandBuffer &) = 0;

		// Allocate space in the staging buffer.
		template <typename T>
				bool		AllocBuffer (BytesU size, BytesU align, OUT RawBufferID &id, OUT BytesU &offset, OUT T* &mapped);
		virtual bool		AllocBuffer (BytesU size, BytesU align, OUT RawBufferID &id, OUT BytesU &offset, OUT void* &mapped) = 0;

		// Starts tracking image state in current command buffer.
		// Image may be in immutable or mutable state, immutable state disables layout transitions and barrier placement.
		// If 'invalidate' sets to 'true' then previous content of the image may be invalidated.
		virtual void		AcquireImage (RawImageID id, bool makeMutable, bool invalidate) = 0;

		// Starts tracking buffer state in current command buffer.
		// Buffer may be in immutable or mutable state, immutable state disables barrier placement that increases CPU performance.
		virtual void		AcquireBuffer (RawBufferID id, bool makeMutable) = 0;

	// tasks //
		virtual Task		AddTask (const SubmitRenderPass &) = 0;
		virtual Task		AddTask (const DispatchCompute &) = 0;
		virtual Task		AddTask (const DispatchComputeIndirect &) = 0;
		virtual Task		AddTask (const CopyBuffer &) = 0;
		virtual Task		AddTask (const CopyImage &) = 0;
		virtual Task		AddTask (const CopyBufferToImage &) = 0;
		virtual Task		AddTask (const CopyImageToBuffer &) = 0;
		virtual Task		AddTask (const BlitImage &) = 0;
		virtual Task		AddTask (const ResolveImage &) = 0;
		virtual Task		AddTask (const GenerateMipmaps &) = 0;
		virtual Task		AddTask (const FillBuffer &) = 0;
		virtual Task		AddTask (const ClearColorImage &) = 0;
		virtual Task		AddTask (const ClearDepthStencilImage &) = 0;
		virtual Task		AddTask (const UpdateBuffer &) = 0;
		virtual Task		AddTask (const UpdateImage &) = 0;
		virtual Task		AddTask (const ReadBuffer &) = 0;
		virtual Task		AddTask (const ReadImage &) = 0;
		virtual Task		AddTask (const Present &) = 0;
		virtual Task		AddTask (const UpdateRayTracingShaderTable &) = 0;
		virtual Task		AddTask (const BuildRayTracingGeometry &) = 0;
		virtual Task		AddTask (const BuildRayTracingScene &) = 0;
		virtual Task		AddTask (const TraceRays &) = 0;
		virtual Task		AddTask (const CustomTask &) = 0;
		
		// Begin secondary command buffer recording.
		//ND_ virtual CommandBuffer  BeginSecondary () = 0;		// TODO


	// draw tasks //

		// Create render pass.
		ND_ virtual LogicalPassID  CreateRenderPass (const RenderPassDesc &desc) = 0;

		// Create render pass and begin recording to the secondary buffer.
		//ND_ virtual CommandBuffer2 BeginSecondary (const RenderPassDesc &desc) = 0;	// TODO

		// Add task to the render pass.
		virtual void		AddTask (LogicalPassID, const DrawVertices &) = 0;
		virtual void		AddTask (LogicalPassID, const DrawIndexed &) = 0;
		virtual void		AddTask (LogicalPassID, const DrawVerticesIndirect &) = 0;
		virtual void		AddTask (LogicalPassID, const DrawIndexedIndirect &) = 0;
		virtual void		AddTask (LogicalPassID, const DrawMeshes &) = 0;
		virtual void		AddTask (LogicalPassID, const DrawMeshesIndirect &) = 0;
		virtual void		AddTask (LogicalPassID, const CustomDraw &) = 0;
	};


	
	template <typename T>
	inline bool  ICommandBuffer::AllocBuffer (BytesU size, BytesU align, OUT RawBufferID &id, OUT BytesU &offset, OUT T* &mapped)
	{
		void*	ptr = null;
		bool	res = AllocBuffer( size, align, id, OUT offset, OUT ptr );
		mapped = Cast<T>( ptr );
		return res;
	}

}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
		EQueueType				queueType					= EQueueType::Graphics;
		BytesU					hostWritableBufferSize		= 256_Mb;
		BytesU					hostReadableBufferSize		= 256_Mb;
		EBufferUsage			hostWritebleBufferUsage		= EBufferUsage::TransferSrc;
		//bool					immutableResources			= true;		// all resources except render targets and storage buffer/image will be immutable
		//bool					submitImmediately			= true;		// set 'false' to merge commands into some betches
		EDebugFlags				debugFlags					= Default;
		StringView				name;
		
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
		ND_ virtual RawImageID	GetSwapchainImage (RawSwapchainID swapchain, ESwapchainImage type = ESwapchainImage::Primary) = 0;
			virtual bool		AddExternalCommands (const ExternalCmdBatch_t &) = 0;
			virtual bool		AddDependency (const CommandBuffer &) = 0;
			virtual bool		AllocBuffer (BytesU size, OUT RawBufferID &id, OUT BytesU &offset, OUT void* &mapped) = 0;

			virtual void		AcquireImage (RawImageID id, bool makeMutable, bool invalidate) = 0;
			virtual void		AcquireBuffer (RawBufferID id, bool makeMutable) = 0;

		// tasks //
		ND_ virtual Task		AddTask (const SubmitRenderPass &) = 0;
		ND_ virtual Task		AddTask (const DispatchCompute &) = 0;
		ND_ virtual Task		AddTask (const DispatchComputeIndirect &) = 0;
		ND_ virtual Task		AddTask (const CopyBuffer &) = 0;
		ND_ virtual Task		AddTask (const CopyImage &) = 0;
		ND_ virtual Task		AddTask (const CopyBufferToImage &) = 0;
		ND_ virtual Task		AddTask (const CopyImageToBuffer &) = 0;
		ND_ virtual Task		AddTask (const BlitImage &) = 0;
		ND_ virtual Task		AddTask (const ResolveImage &) = 0;
		ND_ virtual Task		AddTask (const GenerateMipmaps &) = 0;
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
		
		//ND_ virtual CommandBuffer  BeginSecondary () = 0;


		// draw tasks //
		ND_ virtual LogicalPassID  CreateRenderPass (const RenderPassDesc &desc) = 0;

			// Create render pass and begin recording to the secondary buffer.
		//ND_ virtual CommandBuffer2 BeginSecondary (const RenderPassDesc &desc) = 0;

			// Add task to the render pass.
			virtual void		AddTask (LogicalPassID, const DrawVertices &) = 0;
			virtual void		AddTask (LogicalPassID, const DrawIndexed &) = 0;
			virtual void		AddTask (LogicalPassID, const DrawVerticesIndirect &) = 0;
			virtual void		AddTask (LogicalPassID, const DrawIndexedIndirect &) = 0;
			virtual void		AddTask (LogicalPassID, const DrawMeshes &) = 0;
			virtual void		AddTask (LogicalPassID, const DrawMeshesIndirect &) = 0;
			virtual void		AddTask (LogicalPassID, const CustomDraw &) = 0;
	};


}	// FG

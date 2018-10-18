// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraphTask.h"
#include "framegraph/Public/FrameGraphDrawTask.h"
#include "framegraph/Public/FGEnums.h"
#include "framegraph/Public/LowLevel/VulkanTypes.h"
#include "framegraph/Public/LowLevel/Pipeline.h"
#include "framegraph/Public/LowLevel/BufferDesc.h"
#include "framegraph/Public/LowLevel/ImageDesc.h"
#include "framegraph/Public/LowLevel/SamplerDesc.h"
#include "framegraph/Public/LowLevel/MemoryDesc.h"

namespace FG
{

	//
	// Frame Graph Thread interface
	//

	class FrameGraphThread : public std::enable_shared_from_this<FrameGraphThread>
	{
	// types
	public:
		using SwapchainInfo_t	= Union< std::monostate, VulkanSwapchainInfo, VulkanVREmulatorSwapchainInfo >;


	// interface
	public:
			virtual ~FrameGraphThread () {}
			
		// resource manager
		ND_ virtual GPipelineID		CreatePipeline (MeshPipelineDesc &&desc) = 0;
		ND_ virtual RTPipelineID	CreatePipeline (RayTracingPipelineDesc &&desc) = 0;
		ND_ virtual GPipelineID		CreatePipeline (GraphicsPipelineDesc &&desc) = 0;
		ND_ virtual CPipelineID		CreatePipeline (ComputePipelineDesc &&desc) = 0;

		ND_ virtual ImageID			CreateImage (const MemoryDesc &mem, const ImageDesc &desc) = 0;
		ND_ virtual BufferID		CreateBuffer (const MemoryDesc &mem, const BufferDesc &desc) = 0;
		ND_ virtual SamplerID		CreateSampler (const SamplerDesc &desc) = 0;
		
		//ND_ virtual AccelerationStructPtr	CreateAccelerationStructure (const AccelerationStructDesc &desc) = 0;

			virtual void			DestroyPipeline (GPipelineID id) = 0;
			virtual void			DestroyPipeline (CPipelineID id) = 0;
			virtual void			DestroyPipeline (RTPipelineID id) = 0;
			virtual void			DestroyImage (ImageID id) = 0;
			virtual void			DestroyBuffer (BufferID id) = 0;
			virtual void			DestroySampler (SamplerID id) = 0;

		ND_ virtual BufferDesc const&	GetDescription (BufferID id) const = 0;
		ND_ virtual ImageDesc const&	GetDescription (ImageID id) const = 0;
		//ND_ virtual SamplerDesc const&	GetDescription (SamplerID id) const = 0;
			virtual bool			GetDescriptorSet (GPipelineID pplnId, const DescriptorSetID &id, OUT DescriptorSetLayoutID &layout, OUT uint &binding) const = 0;
			virtual bool			GetDescriptorSet (CPipelineID pplnId, const DescriptorSetID &id, OUT DescriptorSetLayoutID &layout, OUT uint &binding) const = 0;
		ND_ virtual EThreadUsage	GetThreadUsage () const = 0;


		// initialization
			virtual bool		Initialize () = 0;
			virtual void		Deinitialize () = 0;
			virtual void		SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags = Default) = 0;
			virtual bool		CreateSwapchain (const SwapchainInfo_t &) = 0;

		// frame execution
			virtual bool		Begin () = 0;
		ND_ virtual bool		Compile () = 0;


		// resource acquiring
			virtual bool		Acquire (ImageID id) = 0;
			virtual bool		Acquire (ImageID id, MipmapLevel baseLevel, uint levelCount, ImageLayer baseLayer, uint layerCount) = 0;
			virtual bool		Acquire (BufferID id) = 0;
			virtual bool		Acquire (BufferID id, BytesU offset, BytesU size) = 0;

		// tasks
		ND_ virtual Task		AddTask (const SubmitRenderPass &) = 0;
		ND_ virtual Task		AddTask (const DispatchCompute &) = 0;
		ND_ virtual Task		AddTask (const DispatchIndirectCompute &) = 0;
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
		//ND_ virtual Task		AddTask (const TaskGroupSync &) = 0;

		// draw tasks
		ND_ virtual RenderPass	CreateRenderPass (const RenderPassDesc &desc) = 0;
			virtual void		AddDrawTask (RenderPass, const DrawTask &) = 0;
			virtual void		AddDrawTask (RenderPass, const DrawIndexedTask &) = 0;
		//	virtual void		AddDrawTask (RenderPass, const ClearAttachments &) = 0;
		//	virtual void		AddDrawTask (RenderPass, const DrawCommandBuffer &) = 0;
		//	virtual void		AddDrawTask (RenderPass, const DrawMeshTask &) = 0;
	};


}	// FG

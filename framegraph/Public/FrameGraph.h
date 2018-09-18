// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraphTask.h"
#include "framegraph/Public/FrameGraphDrawTask.h"
#include "framegraph/Public/IPipelineCompiler.h"
#include "framegraph/Public/FGEnums.h"
#include "framegraph/Public/LowLevel/VulkanTypes.h"
#include "framegraph/Public/LowLevel/Pipeline.h"

namespace FG
{

	//
	// Frame Graph interface
	//

	class FrameGraph : public std::enable_shared_from_this<FrameGraph>
	{
	// types
	public:
		struct Statistics
		{
		};
		
#		if 1
		ND_ static FrameGraphPtr  CreateVulkanFrameGraph (const VulkanDeviceInfo &);
#		endif

#		if 0
		ND_ static FrameGraphPtr  CreateOpenCLComputeGraph (const CLDeviceInfo &);
#		endif


	// interface
	public:
#	if FG_IS_VIRTUAL
			virtual ~FrameGraph () {}

		// resource manager
		ND_ virtual PipelinePtr			CreatePipeline (GraphicsPipelineDesc &&desc) = 0;
		ND_ virtual PipelinePtr			CreatePipeline (ComputePipelineDesc &&desc) = 0;

		ND_ virtual ImagePtr			CreateImage (EMemoryType	memType,
													 EImage			imageType,
													 const uint3	&dimension,
													 EPixelFormat	format,
													 EImageUsage	usage,
													 ImageLayer		arrayLayer	= Default,
													 MipmapLevel	maxLevel	= Default,
													 MultiSamples	samples		= Default) = 0;

		ND_ virtual ImagePtr			CreateLogicalImage (EMemoryType		memType,
															EImage			imageType,
															const uint3		&dimension,
															EPixelFormat	format,
															ImageLayer		arrayLayer	= Default,
															MipmapLevel		maxLevel	= Default,
															MultiSamples	samples		= Default) = 0;

		ND_ virtual BufferPtr			CreateBuffer (EMemoryType	memType,
													  BytesU		size,
													  EBufferUsage	usage) = 0;

		ND_ virtual BufferPtr			CreateLogicalBuffer (EMemoryType	memType,
															 BytesU			size) = 0;

		ND_ virtual SamplerPtr			CreateSampler (const SamplerDesc &desc) = 0;

		ND_ virtual RenderPass			CreateRenderPass (const RenderPassDesc &desc) = 0;


		// initialization
			virtual bool				Initialize (uint swapchainLength) = 0;
			virtual void				Deinitialize () = 0;
			virtual void				AddPipelineCompiler (const IPipelineCompilerPtr &comp) = 0;
			virtual void				SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags = Default) = 0;


		// frame execution
			virtual bool				Begin () = 0;
		ND_ virtual bool				Compile () = 0;
			virtual bool				Execute () = 0;

			virtual void				ResizeSurface (const uint2 &size) = 0;
			virtual bool				WaitIdle () = 0;

		// tasks
		ND_ virtual Task				AddTask (const SubmitRenderPass &) = 0;
		ND_ virtual Task				AddTask (const DispatchCompute &) = 0;
		ND_ virtual Task				AddTask (const DispatchIndirectCompute &) = 0;
		ND_ virtual Task				AddTask (const CopyBuffer &) = 0;
		ND_ virtual Task				AddTask (const CopyImage &) = 0;
		ND_ virtual Task				AddTask (const CopyBufferToImage &) = 0;
		ND_ virtual Task				AddTask (const CopyImageToBuffer &) = 0;
		ND_ virtual Task				AddTask (const BlitImage &) = 0;
		ND_ virtual Task				AddTask (const ResolveImage &) = 0;
		ND_ virtual Task				AddTask (const FillBuffer &) = 0;
		ND_ virtual Task				AddTask (const ClearColorImage &) = 0;
		ND_ virtual Task				AddTask (const ClearDepthStencilImage &) = 0;
		ND_ virtual Task				AddTask (const UpdateBuffer &) = 0;
		ND_ virtual Task				AddTask (const UpdateImage &) = 0;
		ND_ virtual Task				AddTask (const ReadBuffer &) = 0;
		ND_ virtual Task				AddTask (const ReadImage &) = 0;
		ND_ virtual Task				AddTask (const Present &) = 0;
		//ND_ virtual Task				AddTask (const PresentVR &) = 0;
		ND_ virtual Task				AddTask (const TaskGroupSync &) = 0;

		// draw tasks
			virtual void				AddDrawTask (RenderPass, const DrawTask &) = 0;
			virtual void				AddDrawTask (RenderPass, const DrawIndexedTask &) = 0;
		//	virtual void				AddDrawTask (RenderPass, const ClearAttachments &) = 0;
		//	virtual void				AddDrawTask (RenderPass, const DrawCommandBuffer &) = 0;


		// debugging
		ND_ virtual Statistics const&	GetStatistics () const = 0;
			virtual bool				DumpToString (OUT String &result) const = 0;
			virtual bool				DumpToGraphViz (EGraphVizFlags flags, OUT String &result) const = 0;
#	endif
	};


}	// FG

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraph.h"
#include "VDevice.h"
#include "VTaskGraph.h"
#include "VLogicalRenderPass.h"
#include "VTaskProcessor.h"
#include "VRenderPassCache.h"
#include "VPipelineCache.h"
#include "VSamplerCache.h"
#include "VMemoryManager.h"
#include "VCommandQueue.h"
#include "VStagingBufferManager.h"
#include "VFrameGraphDebugger.h"

namespace FG
{

	//
	// Vulkan Frame Graph
	//

	class VFrameGraph final : public FrameGraph
	{
	// types
	private:
		using LogicalBuffers_t	= HashSet< VLogicalBufferPtr >;
		using LogicalImages_t	= HashSet< VLogicalImagePtr >;
		using CommandQueues_t	= FixedArray< VCommandQueue, 8 >;


		struct ArrangedQueues
		{
			VCommandQueue*		present			= null;
			VCommandQueue*		main			= null;		// graphics and compute and transfer (optional: present)
			VCommandQueue*		asyncCompute	= null;		// may be null

			VCommandQueue*		transfer		= null;
			VCommandQueue*		streaming		= null;		// may be null
			VCommandQueue*		sparseBinding	= null;		// may be null
		};


		struct PerFrame
		{
			LogicalBuffers_t		logicalBuffers;
			LogicalImages_t			logicalImages;
		};
		
		using FGDebuggerPtr_t		= UniquePtr< VFrameGraphDebugger >;

		using PerFrameArray_t		= FixedArray< PerFrame, FG_MaxSwapchainLength >;
		using RenderPasses_t		= Array< UniquePtr< VLogicalRenderPass >>;
		using SortedGraph_t			= Array< Task >;
		using TaskGraph_t			= VTaskGraph< VTaskProcessor >;
		using BufferBarrier_t		= VBuffer::BufferBarrier;
		using ImageBarrier_t		= VImage::ImageBarrier;


	// variables
	private:
		VDevice					_device;
		//VulkanSwapchainPtr		_swapchain;
		VRenderPassCache		_renderPassCache;
		VPipelineCache			_pipelineCache;
		VSamplerCache			_samplerCache;
		VMemoryManager			_memoryMngr;
		VStagingBufferManager	_stagingMngr;
		
		RenderPasses_t			_renderPasses;
		VRenderPassGraph		_renderPassGraph;
		PoolAllocator			_drawTaskAllocator;

		TaskGraph_t				_taskGraph;
		SortedGraph_t			_sortedGraph;
		EmptyTask				_firstTask;
		uint					_visitorID		= 0;
		ExeOrderIndex			_exeOrderIndex	= ExeOrderIndex::Initial;
		bool					_isRecording;

		ArrangedQueues			_arrangedQueues;
		CommandQueues_t			_cmdQueues;

		PerFrameArray_t			_perFrame;
		uint					_currFrame		= 0;

		ECompilationFlags		_compilationFlags;

		FGDebuggerPtr_t			_debugger;
		Statistics				_statistics;


	// methods
	public:
		VFrameGraph (const VulkanDeviceInfo &);
		~VFrameGraph ();
		
		// resource manager
		ND_ PipelinePtr			CreatePipeline (MeshPipelineDesc &&desc)		FG_OVERRIDE;
		ND_ PipelinePtr			CreatePipeline (RayTracingPipelineDesc &&desc)	FG_OVERRIDE;
		ND_ PipelinePtr			CreatePipeline (GraphicsPipelineDesc &&desc)	FG_OVERRIDE;
		ND_ PipelinePtr			CreatePipeline (ComputePipelineDesc &&desc)		FG_OVERRIDE;

		ND_ ImagePtr			CreateImage (EMemoryType	memType,
											 EImage			imageType,
											 const uint3	&dimension,
											 EPixelFormat	format,
											 EImageUsage	usage,
											 ImageLayer		arrayLayer,
											 MipmapLevel	maxLevel,
											 MultiSamples	samples)			FG_OVERRIDE;

		ND_ ImagePtr			CreateLogicalImage (EMemoryType		memType,
													EImage			imageType,
													const uint3		&dimension,
													EPixelFormat	format,
													ImageLayer		arrayLayer,
													MipmapLevel		maxLevel,
													MultiSamples	samples)	FG_OVERRIDE;

		ND_ BufferPtr			CreateBuffer (EMemoryType	memType,
											  BytesU		size,
											  EBufferUsage	usage)				FG_OVERRIDE;

		ND_ BufferPtr			CreateLogicalBuffer (EMemoryType	memType,
													 BytesU			size)		FG_OVERRIDE;
		
		ND_ SamplerPtr			CreateSampler (const SamplerDesc &desc)			FG_OVERRIDE;

		ND_ RenderPass			CreateRenderPass (const RenderPassDesc &desc)	FG_OVERRIDE;
		
		
		// initialization
			bool				Initialize (uint swapchainLength)						FG_OVERRIDE;
			void				Deinitialize ()											FG_OVERRIDE;
			void				AddPipelineCompiler (const IPipelineCompilerPtr &comp)	FG_OVERRIDE;
			void				SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags) FG_OVERRIDE;
			

		// frame execution
			bool				Begin ()							FG_OVERRIDE;
			bool				Compile ()							FG_OVERRIDE;
			bool				Execute ()							FG_OVERRIDE;
			
			void				ResizeSurface (const uint2 &size)	FG_OVERRIDE;
			bool				WaitIdle ()							FG_OVERRIDE;
			

		// tasks
		ND_ Task				AddTask (const SubmitRenderPass &)			FG_OVERRIDE;
		ND_ Task				AddTask (const DispatchCompute &)			FG_OVERRIDE;
		ND_ Task				AddTask (const DispatchIndirectCompute &)	FG_OVERRIDE;
		ND_ Task				AddTask (const CopyBuffer &)				FG_OVERRIDE;
		ND_ Task				AddTask (const CopyImage &)					FG_OVERRIDE;
		ND_ Task				AddTask (const CopyBufferToImage &)			FG_OVERRIDE;
		ND_ Task				AddTask (const CopyImageToBuffer &)			FG_OVERRIDE;
		ND_ Task				AddTask (const BlitImage &)					FG_OVERRIDE;
		ND_ Task				AddTask (const ResolveImage &)				FG_OVERRIDE;
		ND_ Task				AddTask (const FillBuffer &)				FG_OVERRIDE;
		ND_ Task				AddTask (const ClearColorImage &)			FG_OVERRIDE;
		ND_ Task				AddTask (const ClearDepthStencilImage &)	FG_OVERRIDE;
		ND_ Task				AddTask (const UpdateBuffer &)				FG_OVERRIDE;
		ND_ Task				AddTask (const UpdateImage &)				FG_OVERRIDE;
		ND_ Task				AddTask (const ReadBuffer &)				FG_OVERRIDE;
		ND_ Task				AddTask (const ReadImage &)					FG_OVERRIDE;
		ND_ Task				AddTask (const Present &)					FG_OVERRIDE;
		//ND_ Task				AddTask (const PresentVR &)					FG_OVERRIDE;
		ND_ Task				AddTask (const TaskGroupSync &)				FG_OVERRIDE;

		
		// draw tasks
			void				AddDrawTask (RenderPass, const DrawTask &)			FG_OVERRIDE;
			void				AddDrawTask (RenderPass, const DrawIndexedTask &)	FG_OVERRIDE;
		
			
		// debugging
		ND_ Statistics const&	GetStatistics ()											const FG_OVERRIDE;
			bool				DumpToString (OUT String &result)							const FG_OVERRIDE;
			bool				DumpToGraphViz (EGraphVizFlags flags, OUT String &result)	const FG_OVERRIDE;


	public:
		ND_ uint					GetSwapchainLength ()		 const	{ return uint(_perFrame.size()); }

		ND_ VRenderPassCache&		GetRenderPassCache ()				{ return _renderPassCache; }
		ND_ VPipelineCache &		GetPipelineCache ()					{ return _pipelineCache; }
		ND_ VMemoryManager &		GetMemoryManager ()					{ return _memoryMngr; }

		ND_ VDevice const&			GetDevice ()				const	{ return _device; }

		ND_ VFrameGraphPtr			GetSelfShared ()					{ return Cast<VFrameGraph>(shared_from_this()); }

		ND_ ArrangedQueues const&	GetArrangedQueues ()		const	{ return _arrangedQueues; }

		ND_ VFrameGraphDebugger*	GetDebugger ()						{ return _debugger.get(); }
		//ND_ Task					GetFirstTask ()						{ return &_firstTask; }


	private:
		bool _SetupQueues (ArrayView<VulkanDeviceInfo::QueueInfo>);

		void _DestroyLogicalResources (PerFrame &frame) const;
		
		bool _PrepareNextFrame ();
		bool _SortTaskGraph ();
		bool _BuildCommandBuffers ();
	};


}	// FG

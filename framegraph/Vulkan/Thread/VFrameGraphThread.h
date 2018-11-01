// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraphThread.h"
#include "VFrameGraph.h"
#include "VResourceManagerThread.h"
#include "VSwapchain.h"
#include "VTaskGraph.h"
#include "VTaskProcessor.h"

namespace FG
{

	//
	// Frame Graph Thread
	//

	class VFrameGraphThread : public FrameGraphThread
	{
	// types
	private:
		enum class EState : uint
		{
			Initial,
			Idle,
			BeforeStart,
			Ready,
			BeforeRecording,
			Recording,		// in this state used delayed resource creation
			Compiling,
			Pending,		// if compilation succeded
			Execute,
			WaitForPresent,	// if present supported
			Presenting,
			WaitIdle,
			BeforeDestroy,
			Failed,			// if compilation failed
			Destroyed,
		};
		
		static constexpr uint	MaxQueues			= 4;
		static constexpr uint	MaxFrames			= 8;
		static constexpr uint	MaxSignalSemaphores	= 8;
		static constexpr uint	MaxWaitSemaphores	= 8;
		static constexpr uint	MaxCommandsPerFrame	= 8;

		using CommandBuffers_t	= FixedArray< VkCommandBuffer, MaxCommandsPerFrame >;
		using SignalSemaphores_t= FixedArray< VkSemaphore, MaxSignalSemaphores >;
		using WaitSemaphores_t	= FixedArray< VkSemaphore, MaxWaitSemaphores >;
		using WaitDstStages_t	= FixedArray< VkPipelineStageFlags, MaxWaitSemaphores >;
		using Allocator_t		= LinearAllocator<>;


		struct PerFrame
		{
			CommandBuffers_t	commands;
			VkSemaphore			semaphore	= VK_NULL_HANDLE;
		};
		using PerFrameArray_t	= FixedArray< PerFrame, MaxFrames >;


		struct PerQueue
		{
			PerFrameArray_t		frames;
			VkCommandPool		cmdPoolId		= VK_NULL_HANDLE;
			VkQueue				queueId			= VK_NULL_HANDLE;
			VkQueueFlags		queueFlags		= 0;
			uint				familyIndex		= ~0u;
			SignalSemaphores_t	signalSemaphores;
			WaitSemaphores_t	waitSemaphores;
			WaitDstStages_t		waitDstStageMasks;
			bool				fenceRequired	= true;
		};
		using PerQueueArray_t	= FixedArray< PerQueue, MaxQueues >;
		
		using TempTaskArray_t	= std::vector< Task, StdLinearAllocator<Task> >;
		using TaskGraph_t		= VTaskGraph< VTaskProcessor >;


	// variables
	private:
		Allocator_t					_mainAllocator;
		EThreadUsage				_usage;
		std::atomic<EState>			_state;
		ECompilationFlags			_compilationFlags;
		ThreadID					_ownThread;

		PerQueueArray_t				_queues;
		uint						_currFrame		= 0;
		
		TaskGraph_t					_taskGraph;
		uint						_visitorID		= 0;
		ExeOrderIndex				_exeOrderIndex	= ExeOrderIndex::Initial;

		VFrameGraph &						_instance;
		VResourceManagerThread				_resourceMngr;
		SharedPtr< VMemoryManager >			_memoryMngr;		// memory manager must live until all memory objects have been destroyed
		UniquePtr< VStagingBufferManager >	_stagingMngr;
		UniquePtr< VSwapchain >				_swapchain;


	// methods
	public:
		VFrameGraphThread (VFrameGraph &, EThreadUsage);
		~VFrameGraphThread () override;
			
		// resource manager
		MPipelineID		CreatePipeline (MeshPipelineDesc &&desc) override final;
		GPipelineID		CreatePipeline (GraphicsPipelineDesc &&desc) override final;
		CPipelineID		CreatePipeline (ComputePipelineDesc &&desc) override final;
		RTPipelineID	CreatePipeline (RayTracingPipelineDesc &&desc) override final;
		ImageID			CreateImage (const MemoryDesc &mem, const ImageDesc &desc) override final;
		BufferID		CreateBuffer (const MemoryDesc &mem, const BufferDesc &desc) override final;
		SamplerID		CreateSampler (const SamplerDesc &desc) override final;
		bool			InitPipelineResources (RawDescriptorSetLayoutID layout, OUT PipelineResources &resources) const override final;
		void			DestroyResource (INOUT GPipelineID &id) override final;
		void			DestroyResource (INOUT CPipelineID &id) override final;
		void			DestroyResource (INOUT RTPipelineID &id) override final;
		void			DestroyResource (INOUT ImageID &id) override final;
		void			DestroyResource (INOUT BufferID &id) override final;
		void			DestroyResource (INOUT SamplerID &id) override final;

		BufferDesc const&	GetDescription (const BufferID &id) const override final;
		ImageDesc  const&	GetDescription (const ImageID &id) const override final;
		//SamplerDesc const&	GetDescription (const SamplerID &id) const override final;
		
		bool			GetDescriptorSet (const GPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const override final;
		bool			GetDescriptorSet (const CPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const override final;
		bool			GetDescriptorSet (const MPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const override final;
		bool			GetDescriptorSet (const RTPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const override final;

		EThreadUsage	GetThreadUsage () const override final		{ return _usage; }

		// initialization
		bool		Initialize () override;
		void		Deinitialize () override;
		void		SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags) override;
		bool		CreateSwapchain (const SwapchainInfo_t &) override final;

		// frame execution
		bool		SyncOnBegin ();
		bool		Begin () override;
		bool		Compile () override;
		bool		SyncOnExecute (uint batchId, uint indexInBatch);
		bool		Present ();
		bool		OnWaitIdle ();
		
		// resource acquiring
		bool		Acquire (const ImageID &id, bool immutable) override final;
		bool		Acquire (const ImageID &id, MipmapLevel baseLevel, uint levelCount, ImageLayer baseLayer, uint layerCount, bool immutable) override final;
		bool		Acquire (const BufferID &id, bool immutable) override final;
		bool		Acquire (const BufferID &id, BytesU offset, BytesU size, bool immutable) override final;
		
		ImageID		GetSwapchainImage (ESwapchainImage type) override final;

		// tasks
		Task		AddTask (const SubmitRenderPass &) override final;
		Task		AddTask (const DispatchCompute &) override final;
		Task		AddTask (const DispatchIndirectCompute &) override final;
		Task		AddTask (const CopyBuffer &) override final;
		Task		AddTask (const CopyImage &) override final;
		Task		AddTask (const CopyBufferToImage &) override final;
		Task		AddTask (const CopyImageToBuffer &) override final;
		Task		AddTask (const BlitImage &) override final;
		Task		AddTask (const ResolveImage &) override final;
		Task		AddTask (const FillBuffer &) override final;
		Task		AddTask (const ClearColorImage &) override final;
		Task		AddTask (const ClearDepthStencilImage &) override final;
		Task		AddTask (const UpdateBuffer &) override;
		Task		AddTask (const UpdateImage &) override;
		Task		AddTask (const ReadBuffer &) override;
		Task		AddTask (const ReadImage &) override;
		Task		AddTask (const FG::Present &) override final;
		//Task		AddTask (const PresentVR &) override final;
		//Task		AddTask (const TaskGroupSync &) override final;

		// draw tasks
		RenderPass	CreateRenderPass (const RenderPassDesc &desc) override final;
		void		AddDrawTask (RenderPass, const DrawTask &) override final;
		void		AddDrawTask (RenderPass, const DrawIndexedTask &) override final;
		//void		AddDrawTask (RenderPass, const ClearAttachments &) override final;
		//void		AddDrawTask (RenderPass, const DrawCommandBuffer &) override final;
		//void		AddDrawTask (RenderPass, const DrawMeshTask &) override final;
		
		void SignalSemaphore (VkSemaphore sem);
		void WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage);
		
		bool AddPipelineCompiler (const IPipelineCompilerPtr &comp);

		ND_ VkCommandBuffer			CreateCommandBuffer ();

		ND_ bool					IsDestroyed ()				const	{ return _GetState() == EState::Destroyed; }
		ND_ ThreadID const&			GetThreadID ()				const	{ return _ownThread; }
		ND_ Allocator_t &			GetAllocator ()						{ return _mainAllocator; }
		ND_ VDevice const&			GetDevice ()				const	{ return _instance.GetDevice(); }

		ND_ VFrameGraph const*		GetInstance ()				const	{ return &_instance; }
		ND_ VResourceManagerThread*	GetResourceManager ()				{ return &_resourceMngr; }
		ND_ VMemoryManager*			GetMemoryManager ()					{ return _memoryMngr.operator->(); }
		ND_ VPipelineCache *		GetPipelineCache ()					{ return _resourceMngr.GetPipelineCache(); }


	private:
		template <typename Desc, typename ID>
		ND_ Desc const&  _GetDescription (const ID &id) const;
		
		template <typename PplnID>
		bool _GetDescriptorSet (const PplnID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const;

		template <typename ID>
		void _DestroyResource (INOUT ID &id);
		
		void _WaitSemaphore (VkQueue queue, VkSemaphore sem, VkPipelineStageFlags stage);

		ND_ EState	_GetState () const				{ return _state.load( std::memory_order_acquire ); }
			void	_SetState (EState newState);
		ND_ bool	_SetState (EState expected, EState newState);

		ND_ bool	_IsInSeparateThread ()	const;
		ND_ bool	_IsRecording ()			const	{ return _GetState() == EState::Recording; }
		ND_ bool	_IsInitialized ()		const;
		ND_ bool	_IsInitialOrIdleState ()const;

		bool _SetupQueues ();
		bool _AddGraphicsQueue ();
		bool _AddGraphicsPresentQueue ();
		bool _AddTransferQueue ();
		bool _AddAsyncComputeQueue ();
		bool _AddAsyncStreamingQueue ();

		bool _CreateCommandBuffers (INOUT PerQueue &) const;
		bool _BuildCommandBuffers ();
		
		ND_ Task  _AddUpdateBufferTask (const UpdateBuffer &task);
		ND_ Task  _AddUpdateImageTask (const UpdateImage &task);
		ND_ Task  _AddReadBufferTask (const ReadBuffer &task);
		ND_ Task  _AddReadImageTask (const ReadImage &task);
		ND_ Task  _AddPresentTask (const FG::Present &task);
	};


}	// FG

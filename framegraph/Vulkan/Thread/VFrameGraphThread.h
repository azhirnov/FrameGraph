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

	class VFrameGraphThread final : public FrameGraphThread
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
			WaitIdle,
			BeforeDestroy,
			Failed,			// if compilation failed
			Destroyed,
		};
		
		static constexpr uint	MaxQueues			= 4;
		static constexpr uint	MaxFrames			= 8;
		static constexpr uint	MaxCommandsPerFrame	= 8;

		using CommandBuffers_t	= FixedArray< VkCommandBuffer, MaxCommandsPerFrame >;

		struct PerFrame
		{
			CommandBuffers_t	commands;
		};
		using PerFrameArray_t	= FixedArray< PerFrame, MaxFrames >;


		struct PerQueue
		{
			PerFrameArray_t			frames;
			VkCommandPool			cmdPoolId	= VK_NULL_HANDLE;
			VDeviceQueueInfo const*	ptr			= null;
			EThreadUsage			usage		= Default;
		};
		using PerQueueArray_t	= FixedArray< PerQueue, MaxQueues >;
		
		using TempTaskArray_t	= std::vector< Task, StdLinearAllocator<Task> >;
		using TaskGraph_t		= VTaskGraph< VTaskProcessor >;
		using Allocator_t		= LinearAllocator<>;


	// variables
	private:
		Allocator_t					_mainAllocator;

		EThreadUsage				_threadUsage		= Default;
		EThreadUsage				_currUsage			= Default;

		std::atomic<EState>			_state;
		ECompilationFlags			_compilationFlags;

		PerQueueArray_t				_queues;
		uint						_frameId			= 0;
		
		TaskGraph_t					_taskGraph;
		uint						_visitorID			= 0;
		ExeOrderIndex				_exeOrderIndex		= ExeOrderIndex::Initial;
		
		VSubmissionGraph const*		_submissionGraph	= null;
		CommandBatchID				_cmdBatchId;
		uint						_indexInBatch		= ~0u;

		WeakPtr<VFrameGraphThread>	_relativeThread;
		const DebugName_t			_debugName;

		VFrameGraph &						_instance;
		VResourceManagerThread				_resourceMngr;
		VBarrierManager						_barrierMngr;
		SharedPtr< VMemoryManager >			_memoryMngr;		// memory manager must live until all memory objects have been destroyed
		UniquePtr< VStagingBufferManager >	_stagingMngr;
		UniquePtr< VSwapchain >				_swapchain;
		UniquePtr< VFrameGraphDebugger >	_debugger;

		RaceConditionCheck					_rcCheck;


	// methods
	public:
		VFrameGraphThread (VFrameGraph &, EThreadUsage, const FGThreadPtr &, StringView);
		~VFrameGraphThread () override;
			
		// resource manager
		MPipelineID		CreatePipeline (MeshPipelineDesc &&desc, StringView dbgName) override;
		GPipelineID		CreatePipeline (GraphicsPipelineDesc &&desc, StringView dbgName) override;
		CPipelineID		CreatePipeline (ComputePipelineDesc &&desc, StringView dbgName) override;
		RTPipelineID	CreatePipeline (RayTracingPipelineDesc &&desc, StringView dbgName) override;
		ImageID			CreateImage (const MemoryDesc &mem, const ImageDesc &desc, StringView dbgName) override;
		BufferID		CreateBuffer (const MemoryDesc &mem, const BufferDesc &desc, StringView dbgName) override;
		SamplerID		CreateSampler (const SamplerDesc &desc, StringView dbgName) override;
		bool			InitPipelineResources (RawDescriptorSetLayoutID layout, OUT PipelineResources &resources) const override;
		void			DestroyResource (INOUT GPipelineID &id) override;
		void			DestroyResource (INOUT CPipelineID &id) override;
		void			DestroyResource (INOUT RTPipelineID &id) override;
		void			DestroyResource (INOUT ImageID &id) override;
		void			DestroyResource (INOUT BufferID &id) override;
		void			DestroyResource (INOUT SamplerID &id) override;

		BufferDesc const&	GetDescription (const BufferID &id) const override;
		ImageDesc  const&	GetDescription (const ImageID &id) const override;
		//SamplerDesc const&	GetDescription (const SamplerID &id) const override;
		
		bool			GetDescriptorSet (const GPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const override;
		bool			GetDescriptorSet (const CPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const override;
		bool			GetDescriptorSet (const MPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const override;
		bool			GetDescriptorSet (const RTPipelineID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const override;

		EThreadUsage	GetThreadUsage () const override		{ SCOPELOCK( _rcCheck );  return _threadUsage; }
		bool			IsCompatibleWith (const FGThreadPtr &thread, EThreadUsage usage) const override;

		// initialization
		bool		Initialize () override;
		void		Deinitialize () override;
		void		SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags) override;
		bool		CreateSwapchain (const SwapchainInfo_t &) override;

		// frame execution
		bool		SyncOnBegin (const VSubmissionGraph *);
		bool		Begin (const CommandBatchID &id, uint index, EThreadUsage usage) override;
		bool		Compile () override;
		bool		SyncOnExecute ();
		bool		OnWaitIdle ();
		
		// resource acquiring
		bool		Acquire (const ImageID &id, bool immutable) override;
		bool		Acquire (const ImageID &id, MipmapLevel baseLevel, uint levelCount, ImageLayer baseLayer, uint layerCount, bool immutable) override;
		bool		Acquire (const BufferID &id, bool immutable) override;
		bool		Acquire (const BufferID &id, BytesU offset, BytesU size, bool immutable) override;
		
		ImageID		GetSwapchainImage (ESwapchainImage type) override;

		// tasks
		Task		AddTask (const SubmitRenderPass &) override;
		Task		AddTask (const DispatchCompute &) override;
		Task		AddTask (const DispatchIndirectCompute &) override;
		Task		AddTask (const CopyBuffer &) override;
		Task		AddTask (const CopyImage &) override;
		Task		AddTask (const CopyBufferToImage &) override;
		Task		AddTask (const CopyImageToBuffer &) override;
		Task		AddTask (const BlitImage &) override;
		Task		AddTask (const ResolveImage &) override;
		Task		AddTask (const FillBuffer &) override;
		Task		AddTask (const ClearColorImage &) override;
		Task		AddTask (const ClearDepthStencilImage &) override;
		Task		AddTask (const UpdateBuffer &) override;
		Task		AddTask (const UpdateImage &) override;
		Task		AddTask (const ReadBuffer &) override;
		Task		AddTask (const ReadImage &) override;
		Task		AddTask (const FG::Present &) override;
		//Task		AddTask (const PresentVR &) override;
		//Task		AddTask (const TaskGroupSync &) override;

		// draw tasks
		RenderPass	CreateRenderPass (const RenderPassDesc &desc) override;
		void		AddDrawTask (RenderPass, const DrawTask &) override;
		void		AddDrawTask (RenderPass, const DrawIndexedTask &) override;
		//void		AddDrawTask (RenderPass, const ClearAttachments &) override;
		//void		AddDrawTask (RenderPass, const DrawCommandBuffer &) override;
		//void		AddDrawTask (RenderPass, const DrawMeshTask &) override;
		
		void SignalSemaphore (VkSemaphore sem);
		void WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage);
		
		bool AddPipelineCompiler (const IPipelineCompilerPtr &comp);

		ND_ bool					IsDestroyed ()				const	{ SCOPELOCK( _rcCheck );  return _GetState() == EState::Destroyed; }
		ND_ Allocator_t &			GetAllocator ()						{ SCOPELOCK( _rcCheck );  return _mainAllocator; }
		ND_ VDevice const&			GetDevice ()				const	{ SCOPELOCK( _rcCheck );  return _instance.GetDevice(); }

		ND_ VFrameGraph const*		GetInstance ()				const	{ SCOPELOCK( _rcCheck );  return &_instance; }
		ND_ VResourceManagerThread*	GetResourceManager ()				{ SCOPELOCK( _rcCheck );  return &_resourceMngr; }
		ND_ VMemoryManager*			GetMemoryManager ()					{ SCOPELOCK( _rcCheck );  return _memoryMngr.operator->(); }
		ND_ VPipelineCache *		GetPipelineCache ()					{ SCOPELOCK( _rcCheck );  return _resourceMngr.GetPipelineCache(); }
		ND_ VFrameGraphDebugger *	GetDebugger ()						{ SCOPELOCK( _rcCheck );  ASSERT( _debugger );  return _debugger.get(); }


	private:
		template <typename Desc, typename ID>
		ND_ Desc const&  _GetDescription (const ID &id) const;
		
		template <typename PplnID>
		bool _GetDescriptorSet (const PplnID &pplnId, const DescriptorSetID &id, OUT RawDescriptorSetLayoutID &layout, OUT uint &binding) const;

		template <typename ID>
		void _DestroyResource (INOUT ID &id);

		ND_ EState	_GetState () const				{ return _state.load( memory_order_acquire ); }
			void	_SetState (EState newState);
		ND_ bool	_SetState (EState expected, EState newState);

		ND_ bool	_IsInSeparateThread ()	const;
		ND_ bool	_IsRecording ()			const	{ return _GetState() == EState::Recording; }
		ND_ bool	_IsInitialized ()		const;
		ND_ bool	_IsInitialOrIdleState ()const;
		
		ND_ VkCommandBuffer			_CreateCommandBuffer (EThreadUsage usage);
		ND_ PerQueue *				_GetQueue (EThreadUsage usage);
		ND_ VDeviceQueueInfo const*	_GetAnyGraphicsQueue () const;

		bool _SetupQueues ();
		bool _AddGpuQueue (EThreadUsage usage);
		bool _AddGraphicsQueue ();
		bool _AddGraphicsAndPresentQueue ();
		bool _AddTransferQueue ();
		bool _AddAsyncComputeQueue ();
		bool _AddAsyncComputeAndPresentQueue ();
		bool _AddAsyncStreamingQueue ();

		bool _CreateCommandBuffers (INOUT PerQueue &) const;

		bool _BuildCommandBuffers ();
		bool _ProcessTasks (VkCommandBuffer cmd);
		
		ND_ Task  _AddUpdateBufferTask (const UpdateBuffer &task);
		ND_ Task  _AddUpdateImageTask (const UpdateImage &task);
		ND_ Task  _AddReadBufferTask (const ReadBuffer &task);
		ND_ Task  _AddReadImageTask (const ReadImage &task);
		ND_ Task  _AddPresentTask (const FG::Present &task);
	};


}	// FG

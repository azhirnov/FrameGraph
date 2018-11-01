// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraph.h"
#include "framegraph/Shared/ThreadID.h"
#include "VResourceManager.h"
#include "VDevice.h"

namespace FG
{

	//
	// Frame Graph
	//

	class VFrameGraph final : public FrameGraph
	{
	// types
	private:
		enum class EState : uint
		{
			Initial,
			Idle,
			Begin,
			RunThreads,
			Execute,
			Destroyed,
		};

		using VThreadWeak_t		= std::weak_ptr< VFrameGraphThread >;
		using Dependencies_t	= ThreadDesc::Dependencies_t;


		struct ThreadInfo
		{
		// variables
			VThreadWeak_t		ptr;
			Dependencies_t		dependsOn;
			uint				visitorID		= 0;
			uint16_t			batchIndex		= 0;
			uint16_t			indexInBatch	= 0;

		// methods
			ThreadInfo () {}
			ThreadInfo (const VThreadWeak_t &ptr, const Dependencies_t &deps) : ptr{ptr}, dependsOn{deps} {}
		};

		using Threads_t			= HashMap< CommandBatchID, ThreadInfo >;
		using Compilers_t		= HashSet< IPipelineCompilerPtr >;
		
		static constexpr uint	MaxCmdBatches		= 4;
		static constexpr uint	MaxQueues			= 4;
		static constexpr uint	MaxFrames			= 8;
		static constexpr uint	MaxFences			= 32;


		struct CmdBatch
		{
			Array<Pair<uint, size_t>>		commandOffsets;		// TODO: use custom allocator
			Array<VkCommandBuffer>			commandBuffers;
			Array<VkSemaphore>				signalSemaphores;
			Array<VkSemaphore>				waitSemaphores;
			Array<VkPipelineStageFlags>		waitDstStages;
		};
		using CmdBatches_t		= FixedArray< CmdBatch, MaxCmdBatches >;


		struct PerQueue
		{
			VkFence				waitFence	= VK_NULL_HANDLE;
			CmdBatches_t		batches;
		};
		using Queues_t			= FixedMap< VkQueue, PerQueue, MaxFrames >;


		struct PerFrame
		{
			Queues_t			queues;
		};
		using Frames_t			= FixedArray< PerFrame, MaxQueues >;
		using Fences_t			= FixedArray< VkFence, MaxFences >;


	// variables
	private:
		VDevice					_device;
		Threads_t				_threads;
		std::mutex				_threadLock;

		Frames_t				_frames;
		Fences_t				_freeFences;
		uint					_currFrame		= 0;
		uint					_visitorID		= 0;

		VResourceManager		_resourceMngr;
		ThreadID				_ownThread;
		std::atomic<EState>		_state;

		Compilers_t				_pplnCompilers;
		ECompilationFlags		_defaultCompilationFlags;
		ECompilationDebugFlags	_defaultDebugFlags;

		Statistics				_statistics;


	// methods
	public:
		explicit VFrameGraph (const VulkanDeviceInfo &);
		~VFrameGraph () override;

		FGThreadPtr  CreateThread (const ThreadDesc &) override;

		// initialization
		bool  Initialize (uint ringBufferSize) override;
		void  Deinitialize () override;
		bool  AddPipelineCompiler (const IPipelineCompilerPtr &comp) override;
		void  SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags) override;
		
		// frame execution
		bool  Begin () override;
		bool  Execute () override;
		bool  WaitIdle () override;

		// debugging
		Statistics const&  GetStatistics () const override;
		bool  DumpToString (OUT String &result) const override;
		bool  DumpToGraphViz (EGraphVizFlags flags, OUT String &result) const override;


		bool Submit (VkQueue queue, uint batchId, uint indexInBatch,
					 ArrayView<VkCommandBuffer> commands, ArrayView<VkSemaphore> signalSemaphores,
					 ArrayView<VkSemaphore> waitSemaphores, ArrayView<VkPipelineStageFlags> waitDstStages, bool fenceRequired);

		ND_ ThreadID const&		GetThreadID ()			const	{ return _ownThread; }
		ND_ VResourceManager&	GetResourceMngr ()				{ return _resourceMngr; }
		ND_ VDevice const&		GetDevice ()			const	{ return _device; }

		ND_ uint				GetRingBufferSize ()	const	{ return uint(_frames.size()); }


	private:
		ND_ bool  _IsInitialized () const;

		ND_ VkFence  _CreateFence ();
			void     _WaitFences (uint frameId);
			void     _DestroyFences ();

		ND_ EState	_GetState () const;
			void	_SetState (EState newState);
		ND_ bool	_SetState (EState expected, EState newState);
	};


}	// FG

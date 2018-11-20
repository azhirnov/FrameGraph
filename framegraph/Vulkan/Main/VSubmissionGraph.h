// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"
#include "framegraph/Public/SubmissionGraph.h"

# ifdef COMPILER_MSVC
#	pragma warning (push)
#	pragma warning (disable: 4324)	// structure was padded due to alignment specifier
# endif


namespace FG
{

	//
	// Vulkan Submission Graph
	//

	class VSubmissionGraph final
	{
	// types
	private:
		using QueuePtr	= struct VDeviceQueueInfo const *;

		struct alignas(FG_CACHE_LINE) Atomics
		{
			std::atomic<QueuePtr>	queue					{null};
			std::atomic<uint>		existsSubBatchBits		{0};
			std::atomic<uint>		signalSemaphoreCount	{0};
			std::atomic<uint>		sharedSemaphoreCount	{0};
			std::atomic<uint>		waitSemaphoreCount		{0};

			Atomics () {}
			Atomics (Atomics &&) {}
		};

		struct Batch
		{
		// types
			static constexpr uint	MaxSemaphores		= FG_MaxCommandBatchDependencies + 8;
			//static constexpr uint	MaxSharedSemaphores	= 4;
			static constexpr uint	MaxSubBatches		= sizeof(uint) * 8 - 2;	// limited to 'existsSubBatchBits' bit count
			static constexpr uint	MaxCommands			= 8;

			using Dependencies_t		= FixedArray< Batch *, FG_MaxCommandBatchDependencies >;
			using Semaphores_t			= StaticArray< VkSemaphore, MaxSemaphores >;
			//using SharedSemaphores_t	= StaticArray< uint, MaxSharedSemaphores >;
			using WaitDstStages_t		= StaticArray< VkPipelineStageFlags, MaxSemaphores >;
			using Commands_t			= StaticArray< FixedArray<VkCommandBuffer, MaxCommands>, MaxSubBatches >;

		// variables
			mutable Atomics				atomics;
			mutable Commands_t			subBatches;
			mutable Semaphores_t		signalSemaphores;
			//mutable SharedSemaphores_t	sharedSemaphores;
			mutable Semaphores_t		waitSemaphores;
			mutable WaitDstStages_t		waitDstStages;
			VkFence						waitFence		= VK_NULL_HANDLE;
			uint						threadCount		= 0;
			Dependencies_t				input;
			Dependencies_t				output;
		};
		using Batches_t	= FixedMap< CommandBatchID, Batch, FG_MaxCommandBatchCount >;


		/*struct SharedSemaphores
		{
			static constexpr uint	MaxSharedSemaphores		= 16;
			using SharedSemaphores_t = FixedMap< VkSemaphore, uint, MaxSharedSemaphores >;

			mutable std::mutex			lock;
			mutable SharedSemaphores_t	map;
		};*/


		static constexpr uint	MaxFences				= 32;
		static constexpr uint	MaxSemaphores			= 64;
		static constexpr uint	MaxFencesPerFrame		= 4;
		static constexpr uint	MaSemaphoresPerFrame	= Batch::MaxSemaphores * FG_MaxCommandBatchCount;

		using TempFences_t		= FixedArray< VkFence, MaxFencesPerFrame >;
		using TempSemaphores_t	= FixedArray< VkSemaphore, MaSemaphoresPerFrame >;


		struct PerFrame
		{
			TempFences_t		waitFences;
			TempSemaphores_t	semaphores;
		};
		using Frames_t			= FixedArray< PerFrame, FG_MaxRingBufferSize >;
		using FenceCache_t		= FixedArray< VkFence, MaxFences >;
		using SemaphoreCache_t	= FixedArray< VkSemaphore, MaxSemaphores >;


	// variables
	private:
		Batches_t			_batches;
		VDevice const&		_device;

		Frames_t			_frames;
		FenceCache_t		_freeFences;
		SemaphoreCache_t	_freeSemaphores;

		//SharedSemaphores	_sharedSemaphores;


	// methods
	public:
		explicit VSubmissionGraph (const VDevice &);
		~VSubmissionGraph ();

		bool Initialize (uint ringBufferSize);
		void Deinitialize ();

		bool Recreate (uint frameId, const SubmissionGraph &);

		bool WaitFences (uint frameId);

		bool IsAllBatchesSubmitted () const;
		
		bool SignalSemaphore (const CommandBatchID &batchId, VkSemaphore sem) const;
		bool WaitSemaphore (const CommandBatchID &batchId, VkSemaphore sem, VkPipelineStageFlags dstStageMask) const;
		
		//bool SignalSharedSemaphore (const CommandBatchID &batchId, VkSemaphore sem) const;
		//bool WaitSharedSemaphore (const CommandBatchID &batchId, VkSemaphore sem, VkPipelineStageFlags dstStageMask) const;

		bool Submit (VDeviceQueueInfoPtr queue, const CommandBatchID &batchId, uint indexInBatch,
					 ArrayView<VkCommandBuffer> commands) const;

		bool SkipSubBatch (const CommandBatchID &batchId, uint indexInBatch) const;


	private:
		ND_ VkFence		 _CreateFence ();
		ND_ VkSemaphore  _CreateSemaphores ();

		bool _SignalSemaphore (const Batch &batch, VkSemaphore sem) const;
		bool _WaitSemaphore (const Batch &batch, VkSemaphore sem, VkPipelineStageFlags dstStageMask) const;

		bool _AddSharedSemaphore (VkSemaphore sem, OUT uint &index) const;
		bool _RemoveSharedSemaphore (uint index, OUT VkSemaphore &sem) const;

		bool _OnSubBatchSubmittion (const Batch &batch, uint indexInBatch) const;
		bool _SubmitBatch (const Batch &batch) const;
	};

}	// FG


# ifdef COMPILER_MSVC
#	pragma warning (pop)
# endif

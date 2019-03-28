// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCmdBatch.h"

namespace FG
{

	//
	// Vulkan Submitted Batches
	//

	class VSubmitted final
	{
		friend class VFrameGraph;

	// types
	public:
		static constexpr uint	MaxBatches		= 32;
		static constexpr uint	MaxSemaphores	= 8;

		using Batches_t			= FixedArray< VCmdBatchPtr, MaxBatches >;
		using Semaphores_t		= FixedArray< VkSemaphore, MaxSemaphores >;


	// variables
	private:
		const uint			_indexInPool;
		Batches_t			_batches;
		Semaphores_t		_semaphores;
		VkFence				_fence;
		ExeOrderIndex		_submissionOrder;
		EQueueType			_queueType;


	// methods
	public:
		explicit VSubmitted (uint indexInPool);
		~VSubmitted ();

		void  Initialize (EQueueType queue, ArrayView<VCmdBatchPtr>, ArrayView<VkSemaphore>, VkFence, ExeOrderIndex);

		ND_ VkFence		GetFence ()			const	{ return _fence; }
		ND_ EQueueType	GetQueueType ()		const	{ return _queueType; }
		ND_ uint		GetIndexInPool ()	const	{ return _indexInPool; }


	private:
		bool _Release (VDevice const&, VDebugger &, const IFrameGraph::ShaderDebugCallback_t &, OUT Array<VkSemaphore> &, OUT Array<VkFence> &);
	};


}	// FG

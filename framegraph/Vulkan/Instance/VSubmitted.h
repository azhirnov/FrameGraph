// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCmdBatch.h"

namespace FG
{

	//
	// Vulkan Submitted Batches
	//

	class VSubmitted final : public std::enable_shared_from_this<VSubmitted>
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
		Batches_t				_batches;
		Semaphores_t			_semaphores;
		VkFence					_fence;
		const ExeOrderIndex		_submissionOrder;
		const EQueueUsage		_usage;


	// methods
	public:
		VSubmitted (EQueueUsage queue, ArrayView<VCmdBatchPtr>, ArrayView<VkSemaphore>, VkFence, ExeOrderIndex);
		~VSubmitted ();

		//ND_ bool		IsComplete ()		const;
		ND_ VkFence		GetFence ()			const	{ return _fence; }
		ND_ EQueueUsage	GetQueueUsage ()	const	{ return _usage; }


	private:
		bool _Release (VResourceManager &, OUT Array<VkSemaphore> &, OUT Array<VkFence> &);
	};


}	// FG

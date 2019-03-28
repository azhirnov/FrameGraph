// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VSubmitted.h"
#include "VDevice.h"
#include "VResourceManager.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VSubmitted::VSubmitted (uint indexInPool) :
		_indexInPool{ indexInPool }
	{}
	
/*
=================================================
	destructor
=================================================
*/
	VSubmitted::~VSubmitted ()
	{
		ASSERT( not _fence );
		ASSERT( _semaphores.empty() );
		ASSERT( _batches.empty() );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	void  VSubmitted::Initialize (EQueueType queue, ArrayView<VCmdBatchPtr> batches, ArrayView<VkSemaphore> semaphores, VkFence fence, ExeOrderIndex order)
	{
		_batches			= batches;
		_semaphores			= semaphores;
		_fence				= fence;
		_submissionOrder	= order;
		_queueType			= queue;
	}

/*
=================================================
	_Release
=================================================
*/
	bool VSubmitted::_Release (VDevice const &dev, VDebugger &debugger, const IFrameGraph::ShaderDebugCallback_t &shaderDbgCallback,
							   OUT Array<VkSemaphore> &, OUT Array<VkFence> &outFences)
	{
		// because of bug on Nvidia
		for (auto& sem : _semaphores) {
			dev.vkDestroySemaphore( dev.GetVkDevice(), sem, null );
		}

		//outSemaphores.insert( outSemaphores.end(), _semaphores.begin(), _semaphores.end() );
		outFences.push_back( _fence );

		_semaphores.clear();
		_fence = VK_NULL_HANDLE;

		for (auto& batch : _batches) {
			batch->OnComplete( debugger, shaderDbgCallback );
		}

		_batches.clear();
		return true;
	}


}	// FG

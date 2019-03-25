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
	VSubmitted::VSubmitted (EQueueType queue, ArrayView<VCmdBatchPtr> batches, ArrayView<VkSemaphore> semaphores, VkFence fence, ExeOrderIndex order) :
		_batches{ batches },		_semaphores{ semaphores },
		_fence{ fence },			_submissionOrder{ order },
		_queueType{ queue }
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
	_Release
=================================================
*/
	bool VSubmitted::_Release (VResourceManager &resMngr, VDebugger &debugger, OUT Array<VkSemaphore> &, OUT Array<VkFence> &outFences)
	{
		// because of bug on Nvidia
		auto&	dev = resMngr.GetDevice();
		for (auto& sem : _semaphores) {
			dev.vkDestroySemaphore( dev.GetVkDevice(), sem, null );
		}

		//outSemaphores.insert( outSemaphores.end(), _semaphores.begin(), _semaphores.end() );
		outFences.push_back( _fence );

		_semaphores.clear();
		_fence = VK_NULL_HANDLE;

		for (auto& batch : _batches) {
			batch->OnComplete( resMngr, debugger );
		}

		_batches.clear();
		return true;
	}


}	// FG

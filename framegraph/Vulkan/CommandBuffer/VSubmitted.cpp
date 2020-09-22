// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
		_indexInPool{ indexInPool },
		_fence{ VK_NULL_HANDLE },
		_queueType{ Default }
	{
	}
	
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
	void  VSubmitted::Initialize (const VDevice &dev, EQueueType queue, ArrayView<VCmdBatchPtr> batches, ArrayView<VkSemaphore> semaphores)
	{
		EXLOCK( _drCheck );

		if ( not _fence )
		{
			VkFenceCreateInfo	info = {};
			info.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			info.flags	= 0;
			VK_CALL( dev.vkCreateFence( dev.GetVkDevice(), &info, null, OUT &_fence ));
		}
		else
			VK_CALL( dev.vkResetFences( dev.GetVkDevice(), 1, &_fence ));

		_batches	= batches;
		_semaphores	= semaphores;
		_queueType	= queue;
	}

/*
=================================================
	Release
=================================================
*/
	void  VSubmitted::Release (const VDevice &dev, VDebugger &debugger, const IFrameGraph::ShaderDebugCallback_t &shaderDbgCallback, INOUT Statistic_t &outStatistic)
	{
		EXLOCK( _drCheck );

		for (auto& sem : _semaphores) {
			dev.vkDestroySemaphore( dev.GetVkDevice(), sem, null );
		}

		_semaphores.clear();

		for (auto& batch : _batches) {
			batch->OnComplete( debugger, shaderDbgCallback, INOUT outStatistic );
		}

		_batches.clear();
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void  VSubmitted::Destroy (const VDevice &dev)
	{
		EXLOCK( _drCheck );

		dev.vkDestroyFence( dev.GetVkDevice(), _fence, null );
		_fence = VK_NULL_HANDLE;
	}


}	// FG

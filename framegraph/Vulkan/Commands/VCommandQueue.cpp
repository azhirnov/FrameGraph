// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VCommandQueue.h"
#include "VDevice.h"
#include "stl/Algorithms/StringUtils.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VCommandQueue::VCommandQueue (const VDevice &dev, VkQueue id, VkQueueFlags queueFlags, uint familyIndex,
								  float priority, bool supportsPresent, StringView debugName) :
		_currFrame{ 0 },						_device{ dev },
		_queueId{ id },							_queueFlags{ queueFlags },
		_familyIndex{ familyIndex },			_priority{ priority },
		_supportsPresent{ supportsPresent },	_cmdPoolId{ VK_NULL_HANDLE },
		_useFence{ true },						_debugName{ debugName }
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	VCommandQueue::~VCommandQueue ()
	{
		CHECK( _perFrame.empty() );
		CHECK( not _cmdPoolId );
	}
	
/*
=================================================
	Initialize
=================================================
*/
	bool VCommandQueue::Initialize (uint swapchainLength, bool useFence)
	{
		CHECK_ERR( _perFrame.empty() );
		CHECK_ERR( _queueId );

		_perFrame.resize( swapchainLength );
		_useFence = useFence;
		
		const VkDevice	dev		= _device.GetVkDevice();
		uint			counter	= 0;

		for (auto& frame : _perFrame)
		{
			// create fence
			if ( _useFence )
			{
				VkFenceCreateInfo	fence_info	= {};
				fence_info.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fence_info.flags	= VK_FENCE_CREATE_SIGNALED_BIT;
				VK_CHECK( _device.vkCreateFence( dev, &fence_info, null, OUT &frame.fence ));

				_device.SetObjectName( BitCast<uint64_t>(frame.fence), "VCommandQueue_Fence_"s << ToString( counter ), VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT );
			}
			
			// create semaphore
			{
				VkSemaphoreCreateInfo	sem_info = {};
				sem_info.sType	= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				sem_info.flags	= 0;
				VK_CALL( _device.vkCreateSemaphore( dev, &sem_info, null, OUT &frame.semaphore ) );
				
				_device.SetObjectName( BitCast<uint64_t>(frame.semaphore), "VCommandQueue_Semaphore_"s << ToString( counter ), VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT );
			}
			++counter;
		}

		CHECK_ERR( _CreatePool() );
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void VCommandQueue::Deinitialize ()
	{
		const VkDevice	dev = _device.GetVkDevice();

		for (auto& frame : _perFrame)
		{
			if ( frame.fence ) {
				VK_CALL( _device.vkWaitForFences( dev, 1, &frame.fence, true, ~0ull ) );
				_device.vkDestroyFence( dev, frame.fence, null );
			}

			if ( frame.semaphore ) {
				_device.vkDestroySemaphore( dev, frame.semaphore, null );
			}
			
			if ( not frame.commands.empty() ) {
				_device.vkFreeCommandBuffers( dev, _cmdPoolId, uint(frame.commands.size()), frame.commands.data() );
			}
		}
		_perFrame.clear();

		_waitSemaphores.clear();
		_waitDstStageMasks.clear();

		_DestroyPool();
	}

/*
=================================================
	OnBeginFrame
=================================================
*/
	bool VCommandQueue::OnBeginFrame (uint frameIdx)
	{
		_currFrame = frameIdx;

		auto&			frame	= _perFrame[_currFrame];
		const VkDevice	dev		= _device.GetVkDevice();
		
		// wait fence
		if ( frame.fence ) {
			VK_CHECK( _device.vkWaitForFences( dev, 1, &frame.fence, true, ~0ull ));
		}

		// reset commands buffers
		if ( not frame.commands.empty() ) {
			_device.vkFreeCommandBuffers( dev, _cmdPoolId, uint(frame.commands.size()), frame.commands.data() );
		}
		frame.commands.clear();

		return true;
	}
	
/*
=================================================
	SubmitFrame
=================================================
*/
	bool VCommandQueue::SubmitFrame ()
	{
		ASSERT( _waitSemaphores.size() == _waitDstStageMasks.size() );

		auto&	frame = _perFrame[_currFrame];

		if ( frame.commands.empty() )
			return true;


		// reset fence
		if ( not _useFence ) {
			ASSERT( not frame.fence );
		}else{
			ASSERT( frame.fence );
			VK_CHECK( _device.vkResetFences( _device.GetVkDevice(), 1, &frame.fence ) );
		}


		// submit
		FixedArray<VkSemaphore, 8>	signal_semaphores = { frame.semaphore };

		VkSubmitInfo				submit_info = {};
		submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount		= uint(frame.commands.size());
		submit_info.pCommandBuffers			= frame.commands.data();
		submit_info.waitSemaphoreCount		= uint(_waitSemaphores.size());
		submit_info.pWaitSemaphores			= _waitSemaphores.data();
		submit_info.pWaitDstStageMask		= _waitDstStageMasks.data();
		submit_info.signalSemaphoreCount	= uint(signal_semaphores.size());
		submit_info.pSignalSemaphores		= signal_semaphores.data();

		VK_CHECK( _device.vkQueueSubmit( _queueId, 1, &submit_info, frame.fence ) );
		

		// reset
		_waitSemaphores.clear();
		_waitDstStageMasks.clear();

		WaitSemaphore( frame.semaphore, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
		return true;
	}
	
/*
=================================================
	WaitSemaphore
=================================================
*/
	void VCommandQueue::WaitSemaphore (VkSemaphore sem, VkPipelineStageFlags stage)
	{
		_waitSemaphores.push_back( sem );
		_waitDstStageMasks.push_back( stage );
	}

/*
=================================================
	ResetCommands
=================================================
*
	void VCommandQueue::ResetCommands ()
	{
		VK_CALL( vkResetCommandPool( _device.GetVkDevice(), _cmdPoolId, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT ) );
	}

/*
=================================================
	CreateCommandBuffer
=================================================
*/
	VkCommandBuffer VCommandQueue::CreateCommandBuffer ()
	{
		CHECK_ERR( _cmdPoolId );

		VkCommandBufferAllocateInfo	info = {};
		info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.pNext				= null;
		info.commandPool		= _cmdPoolId;
		info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandBufferCount	= 1;

		VkCommandBuffer		cmd;
		VK_CHECK( _device.vkAllocateCommandBuffers( _device.GetVkDevice(), &info, OUT &cmd ));

		_perFrame[_currFrame].commands.push_back( cmd );
		return cmd;
	}

/*
=================================================
	_CreatePool
=================================================
*/
	bool VCommandQueue::_CreatePool ()
	{
		CHECK_ERR( not _cmdPoolId );

		VkCommandPoolCreateInfo		pool_info = {};
		pool_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex	= _familyIndex;
		pool_info.flags				= 0;

		if ( EnumEq( _queueFlags, VK_QUEUE_PROTECTED_BIT ) )
			pool_info.flags |= VK_COMMAND_POOL_CREATE_PROTECTED_BIT;

		VK_CHECK( _device.vkCreateCommandPool( _device.GetVkDevice(), &pool_info, null, OUT &_cmdPoolId ));
		return true;
	}
	
/*
=================================================
	_DestroyPool
=================================================
*/
	void VCommandQueue::_DestroyPool ()
	{
		if ( _cmdPoolId )
		{
			_device.vkDestroyCommandPool( _device.GetVkDevice(), _cmdPoolId, null );
			_cmdPoolId = VK_NULL_HANDLE;
		}
	}


}	// FG

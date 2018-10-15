// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraph.h"
#include "VDrawTask.h"
#include "VLogicalRenderPass.h"
#include "VLogicalBuffer.h"
#include "VLogicalImage.h"

namespace FG
{

/*
=================================================
	operator ++ (ExeOrderIndex)
=================================================
*/
	ND_ ExeOrderIndex&  operator ++ (ExeOrderIndex &value)
	{
		value = BitCast<ExeOrderIndex>( BitCast<uint>( value ) + 1 );
		return value;
	}
	
/*
=================================================
	FindPresentQueue
=================================================
*/
	ND_ static VCommandQueue*  FindPresentQueue (ArrayView<VCommandQueue> queues)
	{
		VCommandQueue*	result = null;

		for (auto& q : queues)
		{
			if ( q.IsSupportsPresent() )
			{
				if ( result and result->IsSupportsPresent() )
				{
					if ( q.GetPriority() > result->GetPriority() )
						result = const_cast<VCommandQueue*>( &q );
				}
				else
					result = const_cast<VCommandQueue*>( &q );
			}
		}

		return result;
	}
	
/*
=================================================
	FindGraphicsQueue
=================================================
*/
	ND_ static VCommandQueue*  FindGraphicsQueue (ArrayView<VCommandQueue> queues, VCommandQueue* presentQueue)
	{
		if ( presentQueue )
			return presentQueue;
		
		VCommandQueue*	result = null;

		for (auto& q : queues)
		{
			// must support graphics (and compute and transfer)
			if ( not EnumEq( q.GetQueueFlags(), VK_QUEUE_GRAPHICS_BIT ) )
				continue;

			if ( not result or q.GetPriority() > result->GetPriority() )
				result = const_cast<VCommandQueue*>( &q );
		}

		return result;
	}

/*
=================================================
	FindAsyncComputeQueue
=================================================
*/
	ND_ static VCommandQueue*  FindAsyncComputeQueue (ArrayView<VCommandQueue> queues, VCommandQueue* presentQueue, VCommandQueue* graphicsQueue)
	{
		VCommandQueue*	result = null;

		for (auto& q : queues)
		{
			// must support compute
			if ( not EnumEq( q.GetQueueFlags(), VK_QUEUE_COMPUTE_BIT ) )
				continue;
			
			if ( &q == presentQueue or &q == graphicsQueue )
				continue;

			if ( not result or q.GetPriority() > result->GetPriority() )
				result = const_cast<VCommandQueue*>( &q );
		}

		return result;
	}
	
/*
=================================================
	FindStreamingQueue
=================================================
*/
	ND_ static VCommandQueue*  FindStreamingQueue (ArrayView<VCommandQueue> queues)
	{
		VCommandQueue*	result = null;

		for (auto& q : queues)
		{
			if ( q.GetQueueFlags() != VK_QUEUE_TRANSFER_BIT )
				continue;
			
			if ( not result or q.GetPriority() > result->GetPriority() )
				result = const_cast<VCommandQueue*>( &q );
		}

		return result;
	}

/*
=================================================
	_SetupQueues
=================================================
*/
	bool VFrameGraph::_SetupQueues (ArrayView<VulkanDeviceInfo::QueueInfo> queues)
	{
		CHECK_ERR( _cmdQueues.capacity() >= queues.size() );

		_arrangedQueues = Default;
		
		for (auto& q : queues)
		{
			VkBool32	supports_present = VK_FALSE;

			if ( _device.GetVkSurface() ) {
				VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _device.GetVkPhysicalDevice(), q.familyIndex, _device.GetVkSurface(), OUT &supports_present ) );
			}

			_cmdQueues.push_back(VCommandQueue{ _device, BitCast<VkQueue>(q.id), BitCast<VkQueueFlags>(q.familyFlags),
												q.familyIndex, q.priority, supports_present == VK_TRUE, q.debugName });
		}

		_arrangedQueues.present			= FindPresentQueue( _cmdQueues );
		_arrangedQueues.main			= FindGraphicsQueue( _cmdQueues, _arrangedQueues.present );
		_arrangedQueues.asyncCompute	= FindAsyncComputeQueue( _cmdQueues, _arrangedQueues.present, _arrangedQueues.main );
		_arrangedQueues.streaming		= FindStreamingQueue( _cmdQueues );
		_arrangedQueues.transfer		= null;	// TODO
		_arrangedQueues.sparseBinding	= null;	// TODO

		return true;
	}
	
/*
=================================================
	Begin
=================================================
*/
	bool VFrameGraph::Begin ()
	{
		CHECK_ERR( not _isRecording );
		_isRecording = true;
	
		CHECK_ERR( _PrepareNextFrame() );
		return true;
	}
	
/*
=================================================
	_PrepareNextFrame
=================================================
*/
	bool VFrameGraph::_PrepareNextFrame ()
	{
		_currFrame = (_currFrame + 1) % _perFrame.size();

		auto&	frame = _perFrame[_currFrame];
		
		for (auto& q : _cmdQueues) {
			q.OnBeginFrame( _currFrame );
		}
			
		_memoryMngr.OnBeginFrame( _currFrame );
		_device._OnBeginFrame( _currFrame );
		_stagingMngr.OnBeginFrame( _currFrame );

		if ( _debugger )
			_debugger->Clear();

		_DestroyLogicalResources( frame );
		return true;
	}
	
/*
=================================================
	_DestroyLogicalResources
=================================================
*/
	void VFrameGraph::_DestroyLogicalResources (PerFrame &frame) const
	{
		for (auto& buf : frame.logicalBuffers) {
			buf->_DestroyLogical();
		}
		for (auto& img : frame.logicalImages) {
			img->_DestroyLogical();
		}

		frame.logicalBuffers.clear();
		frame.logicalImages.clear();
	}

/*
=================================================
	Compile
=================================================
*/
	bool  VFrameGraph::Compile ()
	{
		CHECK_ERR( _isRecording );
		_isRecording = false;

		_memoryMngr.OnEndFrame();
		_stagingMngr.OnEndFrame( &_firstTask );


		if ( not _taskGraph.Empty() )
		{
			// sort graph
			CHECK_ERR( _SortTaskGraph() );
		
			CHECK_ERR( _BuildCommandBuffers() );
		}

		return true;
	}
	
/*
=================================================
	Execute
=================================================
*/
	bool  VFrameGraph::Execute ()
	{
		CHECK_ERR( not _isRecording );

		if ( not _taskGraph.Empty() )
		{
			// execute
			for (auto& q : _cmdQueues) {
				CHECK_ERR( q.SubmitFrame() );
			}
		}
		
		if ( _debugger )
			_debugger->Clear();

		_renderPassGraph.Clear();
		_sortedGraph.clear();
		_taskGraph.Clear();
		_visitorID	= 0;

		_renderPasses.clear();
		_drawTaskAllocator.Clear();

		return true;
	}

/*
=================================================
	ResizeSurface
=================================================
*/
	void VFrameGraph::ResizeSurface (const uint2 &size)
	{
		WaitIdle();

		ASSERT(false);
		//_swapchain.Recreate();

		// TODO:
		//	- delete swapchain images
		//	- delete framebuffers (with swapchain images)
		//	- delete command buffers
	}
	
/*
=================================================
	WaitIdle
=================================================
*/
	bool VFrameGraph::WaitIdle ()
	{
		CHECK_ERR( not _isRecording );

		for (size_t i = 0; i < _perFrame.size(); ++i)
		{
			CHECK_ERR( Begin() and Compile() and Execute() );
		}

		VK_CHECK( _device.vkDeviceWaitIdle( _device.GetVkDevice() ));
		return true;
	}

/*
=================================================
	_SortTaskGraph
=================================================
*/
	bool VFrameGraph::_SortTaskGraph ()
	{
		++_visitorID;

		_sortedGraph.clear();
		_sortedGraph.reserve( _taskGraph.Count() );

		_firstTask.SetVisitorID( _visitorID );
		_firstTask.SetExecutionOrder( ++_exeOrderIndex );
		//_sortedGraph.PushBack( _firstTask.ptr() );

		Array<Task>		pending	= _taskGraph.Entries();

		for (uint k = 0; k < 10 and not pending.empty(); ++k)
		{
			for (size_t i = 0; i < pending.size(); ++i)
			{
				auto&	node = pending[i];
				
				if ( /*node->IsHidden() or*/ node->VisitorID() == _visitorID )
					continue;

				// wait for input
				bool	input_processed = true;

				for (auto in_node : node->Inputs())
				{
					if ( in_node->VisitorID() != _visitorID ) {
						input_processed = false;
						break;
					}
				}

				if ( not input_processed )
					continue;
			
				node->SetVisitorID( _visitorID );
				node->SetExecutionOrder( ++_exeOrderIndex );
				_sortedGraph.push_back( node );

				for (auto out_node : node->Outputs())
				{
					pending.push_back( out_node );
				}

				pending.erase( pending.begin() + i );
				--i;
			}
		}

		//_sortedGraph.PushBack( _lastTask.ptr() );

		ASSERT( _exeOrderIndex < ExeOrderIndex::Max );	// TODO: check overflow
		return true;
	}
	
/*
=================================================
	_BuildCommandBuffers
=================================================
*/
	bool VFrameGraph::_BuildCommandBuffers ()
	{
		VTaskProcessor	processor{ *this };

		++_visitorID;

		for (auto& node : _sortedGraph)
		{
			DEBUG_ONLY(
				for (auto in_node : node->Inputs() )
				{
					CHECK_ERR( in_node->VisitorID() == _visitorID );
				}
				node->SetVisitorID( _visitorID );
			);

			processor.Run( node );
		}
		return true;
	}


}	// FG

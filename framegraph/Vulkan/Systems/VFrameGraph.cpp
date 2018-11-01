// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraph.h"
#include "VFrameGraphThread.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VFrameGraph::VFrameGraph (const VulkanDeviceInfo &vdi) :
		_device{ vdi },
		_resourceMngr{ _device },
		_defaultCompilationFlags{ Default },
		_defaultDebugFlags{ Default }
	{
		_threads.reserve( 32 );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VFrameGraph::~VFrameGraph ()
	{
		ASSERT( not _IsInitialized() );
		CHECK( _GetState() == EState::Destroyed );
		CHECK( _threads.empty() );
	}
	
/*
=================================================
	CreateThread
=================================================
*/
	FGThreadPtr  VFrameGraph::CreateThread (const ThreadDesc &desc)
	{
		CHECK_ERR( _IsInitialized() );

		EThreadUsage	usage = desc.usage;

		// validate usage
		if ( EnumEq( usage, EThreadUsage::AsyncStreaming ) )
			usage |= (EThreadUsage::Transfer | EThreadUsage::MemAllocation);

		if ( EnumEq( usage, EThreadUsage::Transfer ) )
			usage |= EThreadUsage::MemAllocation;

		// create thread
		SharedPtr<VFrameGraphThread>	thread{ new VFrameGraphThread{ *this, usage }};

		for (auto& comp : _pplnCompilers) {
			thread->AddPipelineCompiler( comp );
		}

		thread->SetCompilationFlags( _defaultCompilationFlags, _defaultDebugFlags );

		SCOPELOCK( _threadLock );
		CHECK_ERR( _threads.insert_or_assign( desc.batchId, ThreadInfo{ thread, desc.dependsOn }).second );	// batchId must be unique

		// sort threads
		{
			int						batch_index		= -1;
			int						index_in_batch	= -1;
			Array< ThreadInfo *>	pending;

			++_visitorID;
			
			for (auto& item : _threads)
			{
				if ( item.second.visitorID == _visitorID )
					continue;
				
				// wait for input
				bool	input_processed = true;
				bool	use_semaphore	= false;

				for (auto dep : item.second.dependsOn)
				{
					auto	in = _threads.find( dep.first );
					ASSERT( in != _threads.end() );

					use_semaphore |= (dep.second == EThreadSync::Semaphore);

					if ( in != _threads.end() and in->second.visitorID != _visitorID ) {
						input_processed = false;
						break;
					}
				}

				if ( not input_processed )
					continue;

				if ( use_semaphore ) {
					batch_index		= batch_index < 0 ? 0 : batch_index+1;
					index_in_batch	= 0;
				}else{
					batch_index		= batch_index < 0 ? 0 : batch_index;
					index_in_batch	= index_in_batch < 0 ? 0 : index_in_batch+1;
				}

				item.second.visitorID		= _visitorID;
				item.second.batchIndex		= uint16_t(batch_index);
				item.second.indexInBatch	= uint16_t(index_in_batch);
			}
		}

		return thread;
	}
	
/*
=================================================
	_IsInitialized
=================================================
*/
	inline bool  VFrameGraph::_IsInitialized () const
	{
		return not _frames.empty();
	}

/*
=================================================
	Initialize
=================================================
*/
	bool  VFrameGraph::Initialize (uint ringBufferSize)
	{
		CHECK_ERR( not _IsInitialized() );

		_frames.resize( ringBufferSize );
		_currFrame	= 0;
		
		CHECK_ERR( _SetState( EState::Initial, EState::Idle ));
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void  VFrameGraph::Deinitialize ()
	{
		CHECK( _IsInitialized() );
		
		// checks if all threads are destroyed
		{
			SCOPELOCK( _threadLock );
			for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
			{
				auto	thread = iter->second.ptr.lock();
				CHECK( not thread );
			}
		}

		CHECK( WaitIdle() );
		CHECK( _SetState( EState::Idle, EState::Destroyed ));

		_threads.clear();

		_frames.clear();
		_DestroyFences();
		_resourceMngr.OnDestroy();
	}
	
/*
=================================================
	_DestroyFences
=================================================
*/
	void  VFrameGraph::_DestroyFences ()
	{
		ASSERT( _GetState() == EState::Destroyed );

		for (auto fence : _freeFences) {
			_device.vkDestroyFence( _device.GetVkDevice(), fence, null );
		}
		_freeFences.clear();
	}

/*
=================================================
	AddPipelineCompiler
=================================================
*/
	bool  VFrameGraph::AddPipelineCompiler (const IPipelineCompilerPtr &comp)
	{
		CHECK_ERR( _GetState() == EState::Idle );

		if ( not _pplnCompilers.insert( comp ).second )
			return true;
		
		SCOPELOCK( _threadLock );
		for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
		{
			auto	thread = iter->second.ptr.lock();
			if ( thread ) {
				thread->AddPipelineCompiler( comp );
			}
		}

		return true;
	}
	
/*
=================================================
	SetCompilationFlags
=================================================
*/
	void  VFrameGraph::SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags)
	{
		_defaultCompilationFlags	= flags;
		_defaultDebugFlags			= debugFlags;
	}
	
/*
=================================================
	Begin
=================================================
*/
	bool  VFrameGraph::Begin ()
	{
		CHECK_ERR( _SetState( EState::Idle, EState::Begin ));

		_currFrame = (_currFrame + 1) % _frames.size();

		// begin thread execution
		{
			SCOPELOCK( _threadLock );
			for (auto iter = _threads.begin(); iter != _threads.end();)
			{
				auto	thread = iter->second.ptr.lock();

				if ( not thread or thread->IsDestroyed() ) {
					iter = _threads.erase( iter );
					continue;
				}

				CHECK( thread->SyncOnBegin() );
				++iter;
			}
		}

		_WaitFences( _currFrame );

		_resourceMngr.OnBeginFrame();

		CHECK_ERR( _SetState( EState::Begin, EState::RunThreads ));
		return true;
	}
	
/*
=================================================
	_WaitFences
=================================================
*/
	void  VFrameGraph::_WaitFences (uint frameId)
	{
		FixedArray< VkFence, MaxQueues >	fences;
		auto &								frame = _frames[frameId];
			
		for (auto& queue : frame.queues)
		{
			if ( queue.second.waitFence )
			{
				fences.push_back( queue.second.waitFence );
				queue.second.waitFence = VK_NULL_HANDLE;
			}
		}

		if ( not fences.empty() )
		{
			VK_CALL( _device.vkWaitForFences( _device.GetVkDevice(), uint(fences.size()), fences.data(), VK_TRUE, ~0ull ));		// TODO: set timeout ?
			VK_CALL( _device.vkResetFences( _device.GetVkDevice(), uint(fences.size()), fences.data() ));

			for (auto fence : fences) {
				_freeFences.push_back( fence );
			}
		}
	}

/*
=================================================
	Execute
=================================================
*/
	bool  VFrameGraph::Execute ()
	{
		CHECK_ERR( _SetState( EState::RunThreads, EState::Execute ));

		Array< VFrameGraphThread *>		present_threads;

		// complete thread execution
		{
			SCOPELOCK( _threadLock );
			for (auto iter = _threads.begin(); iter != _threads.end();)
			{
				auto	thread = iter->second.ptr.lock();

				if ( not thread or thread->IsDestroyed() ) {
					iter = _threads.erase( iter );
					continue;
				}

				if ( EnumEq( thread->GetThreadUsage(), EThreadUsage::Present ) )
					present_threads.push_back( thread.get() );

				CHECK( thread->SyncOnExecute( iter->second.batchIndex, iter->second.indexInBatch ));
				++iter;
			}
		}

		// submit commands from all threads
		{
			FixedArray< VkSubmitInfo, MaxCmdBatches >	submits;
			auto &										frame = _frames[_currFrame];

			for (auto& queue : frame.queues)
			{
				submits.clear();

				for (auto& batch : queue.second.batches)
				{
					submits.push_back( {} );
					auto&	submit = submits.back();

					submit.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
					submit.pNext				= null;
					submit.pCommandBuffers		= batch.commandBuffers.data();
					submit.commandBufferCount	= uint(batch.commandBuffers.size());
					submit.pSignalSemaphores	= batch.signalSemaphores.data();
					submit.signalSemaphoreCount	= uint(batch.signalSemaphores.size());
					submit.pWaitSemaphores		= batch.waitSemaphores.data();
					submit.pWaitDstStageMask	= batch.waitDstStages.data();
					submit.waitSemaphoreCount	= uint(batch.waitSemaphores.size());
				}

				ASSERT( not submits.empty() );
				VK_CALL( _device.vkQueueSubmit( queue.first, uint(submits.size()), submits.data(), OUT queue.second.waitFence ));

				queue.second.batches.clear();
			}
		}

		// present swapchains
		{
			for (auto& thread : present_threads)
			{
				CHECK( thread->Present() );
			}
		}

		_resourceMngr.OnEndFrame();
		
		CHECK_ERR( _SetState( EState::Execute, EState::Idle ));
		return true;
	}
	
/*
=================================================
	Submit
=================================================
*/
	bool  VFrameGraph::Submit (VkQueue queueId, const uint batchId, const uint indexInBatch,
							   ArrayView<VkCommandBuffer> commands, ArrayView<VkSemaphore> signalSemaphores,
							   ArrayView<VkSemaphore> waitSemaphores, ArrayView<VkPipelineStageFlags> waitDstStages, bool fenceRequired)
	{
		ASSERT( _GetState() == EState::Execute );
		CHECK_ERR( not commands.empty() );
		CHECK_ERR( waitSemaphores.size() == waitDstStages.size() );

		auto&	frame	= _frames[_currFrame];
		auto&	queue	= frame.queues.insert({ queueId, {} }).first->second;

		queue.batches.resize( Max(batchId+1, queue.batches.size()) );

		auto&	batch		= queue.batches[batchId];
		size_t	cmd_offset	= 0;
		
		for (size_t	i = batch.commandOffsets.size()-1;; --i)
		{
			if ( i < batch.commandOffsets.size() )
			{
				auto&	item = batch.commandOffsets[i];

				if ( item.first > indexInBatch ) {
					cmd_offset   = Min( item.second, cmd_offset );
					item.second += commands.size();
					continue;
				}
				if ( item.first == indexInBatch ) {
					item.second = cmd_offset;
					break;
				}
			}

			batch.commandOffsets.insert( batch.commandOffsets.begin(), { indexInBatch, cmd_offset });
			break;
		}
		
		batch.commandBuffers.insert( batch.commandBuffers.begin() + cmd_offset, commands.begin(), commands.end() );
		batch.signalSemaphores.insert( batch.signalSemaphores.end(), signalSemaphores.begin(), signalSemaphores.end() );
		batch.waitSemaphores.insert( batch.waitSemaphores.end(), waitSemaphores.begin(), waitSemaphores.end() );
		batch.waitDstStages.insert( batch.waitDstStages.end(), waitDstStages.begin(), waitDstStages.end() );

		if ( fenceRequired and not queue.waitFence )
			queue.waitFence = _CreateFence();

		return true;
	}
	
/*
=================================================
	_CreateFence
=================================================
*/
	VkFence  VFrameGraph::_CreateFence ()
	{
		if ( not _freeFences.empty() )
		{
			auto	fence = _freeFences.back();
			_freeFences.pop_back();
			return fence;
		}

		VkFence				fence;
		VkFenceCreateInfo	fence_info	= {};
		fence_info.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags	= 0;

		VK_CHECK( _device.vkCreateFence( _device.GetVkDevice(), &fence_info, null, OUT &fence ));
		return fence;
	}

/*
=================================================
	WaitIdle
=================================================
*/
	bool  VFrameGraph::WaitIdle ()
	{
		SCOPELOCK( _threadLock );

		for (size_t i = 0; i < _frames.size(); ++i)
		{
			const uint	frame_id = uint(_currFrame + i) % _frames.size();
			
			_WaitFences( frame_id );

			for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
			{
				auto	thread = iter->second.ptr.lock();
				if ( thread )
					CHECK( thread->OnWaitIdle() );
			}
		}
		return true;
	}
	
/*
=================================================
	GetStatistics
=================================================
*/
	VFrameGraph::Statistics const&  VFrameGraph::GetStatistics () const
	{
		// TODO
		return _statistics;
	}
	
/*
=================================================
	DumpToString
=================================================
*/
	bool  VFrameGraph::DumpToString (OUT String &result) const
	{
		return false;
	}
	
/*
=================================================
	DumpToGraphViz
=================================================
*/
	bool  VFrameGraph::DumpToGraphViz (EGraphVizFlags flags, OUT String &result) const
	{
		return false;
	}
	
/*
=================================================
	_GetState / _SetState
=================================================
*/
	inline VFrameGraph::EState  VFrameGraph::_GetState () const
	{
		return _state.load( std::memory_order_acquire );
	}

	inline void  VFrameGraph::_SetState (EState newState)
	{
		_state.store( newState, std::memory_order_release );
	}

	inline bool  VFrameGraph::_SetState (EState expected, EState newState)
	{
		return _state.compare_exchange_strong( INOUT expected, newState, std::memory_order_release, std::memory_order_relaxed );
	}


}	// FG

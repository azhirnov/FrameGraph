// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraphInstance.h"
#include "VFrameGraphThread.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VFrameGraphInstance::VFrameGraphInstance (const VulkanDeviceInfo &vdi) :
		_device{ vdi },
		_submissionGraph{ _device },
		_resourceMngr{ _device },
		_defaultCompilationFlags{ Default },
		_defaultDebugFlags{ Default }
	{
		EXLOCK( _rcCheck );
		_threads.reserve( 32 );
	}
	
/*
=================================================
	destructor
=================================================
*/
	VFrameGraphInstance::~VFrameGraphInstance ()
	{
		EXLOCK( _rcCheck );
		ASSERT( not _IsInitialized() );
		CHECK( _GetState() == EState::Destroyed );
		CHECK( _threads.empty() );
	}
	
/*
=================================================
	CreateThread
=================================================
*/
	FGThreadPtr  VFrameGraphInstance::CreateThread (const ThreadDesc &desc)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _IsInitialized() );

		EThreadUsage	usage = desc.usage;

		if ( EnumEq( usage, EThreadUsage::Transfer ) )
			usage |= EThreadUsage::MemAllocation;

		// create thread
		SharedPtr<VFrameGraphThread>	thread{ new VFrameGraphThread{ *this, usage, desc.name }};

		for (auto& comp : _pplnCompilers) {
			thread->AddPipelineCompiler( comp );
		}

		thread->SetCompilationFlags( _defaultCompilationFlags, _defaultDebugFlags );

		EXLOCK( _threadLock );
		_threads.push_back( thread );

		return thread;
	}
	
/*
=================================================
	_IsInitialized
=================================================
*/
	inline bool  VFrameGraphInstance::_IsInitialized () const
	{
		return _ringBufferSize > 0;
	}

/*
=================================================
	Initialize
=================================================
*/
	bool  VFrameGraphInstance::Initialize (uint ringBufferSize)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( not _IsInitialized() );

		_ringBufferSize	= ringBufferSize;
		_frameId		= 0;
		
		// setup queues
		{
			_AddGraphicsQueue();
			_AddAsyncComputeQueue();
			_AddAsyncTransferQueue();
			CHECK_ERR( not _queueMap.empty() );
		}

		CHECK_ERR( _submissionGraph.Initialize( ringBufferSize ));
		CHECK_ERR( _resourceMngr.Initialize( ringBufferSize ));
		
		CHECK_ERR( _SetState( EState::Initial, EState::Idle ));
		return true;
	}
	
/*
=================================================
	Deinitialize
=================================================
*/
	void  VFrameGraphInstance::Deinitialize ()
	{
		EXLOCK( _rcCheck );
		CHECK( _IsInitialized() );
		
		// checks if all threads was destroyed
		{
			EXLOCK( _threadLock );
			for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
			{
				auto	thread = iter->lock();
				CHECK( not thread );
			}
		}

		CHECK( _WaitIdle() );
		CHECK( _SetState( EState::Idle, EState::Destroyed ));

		_threads.clear();
		_ringBufferSize = 0;

		_submissionGraph.Deinitialize();
		_resourceMngr.Deinitialize();
	}

/*
=================================================
	AddPipelineCompiler
=================================================
*/
	bool  VFrameGraphInstance::AddPipelineCompiler (const IPipelineCompilerPtr &comp)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _GetState() == EState::Idle );

		if ( not _pplnCompilers.insert( comp ).second )
			return true;
		
		EXLOCK( _threadLock );
		for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
		{
			auto	thread = iter->lock();
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
	void  VFrameGraphInstance::SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags)
	{
		EXLOCK( _rcCheck );

		_defaultCompilationFlags	= flags;
		_defaultDebugFlags			= debugFlags;
	}
	
/*
=================================================
	BeginFrame
=================================================
*/
	bool  VFrameGraphInstance::BeginFrame (const SubmissionGraph &graph)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Idle, EState::Begin ));

		_frameId	= (_frameId + 1) % _ringBufferSize;
		_statistics	= Default;

		CHECK_ERR( _submissionGraph.WaitFences( _frameId ));
		CHECK_ERR( _submissionGraph.Recreate( _frameId, graph, *this ));

		// begin thread execution
		{
			EXLOCK( _threadLock );
			for (auto iter = _threads.begin(); iter != _threads.end();)
			{
				auto	thread = iter->lock();

				if ( not thread or thread->IsDestroyed() ) {
					iter = _threads.erase( iter );
					continue;
				}

				CHECK( thread->SyncOnBegin( &_submissionGraph ));
				++iter;
			}
		}


		_debugger.OnBeginFrame( graph );
		_resourceMngr.OnBeginFrame( _frameId );

		CHECK_ERR( _SetState( EState::Begin, EState::RunThreads ));
		return true;
	}
	
/*
=================================================
	SkipBatch
----
	thread safe and lock-free
=================================================
*/
	bool  VFrameGraphInstance::SkipBatch (const CommandBatchID &batchId, uint indexInBatch)
	{
		CHECK_ERR( _GetState() == EState::RunThreads );

		CHECK_ERR( _submissionGraph.SkipSubBatch( batchId, indexInBatch ));
		return true;
	}
	
/*
=================================================
	SubmitBatch
----
	thread safe and wait-free
=================================================
*/
	bool  VFrameGraphInstance::SubmitBatch (const CommandBatchID &batchId, uint indexInBatch, const ExternalCmdBatch_t &data)
	{
		CHECK_ERR( _GetState() == EState::RunThreads );

		auto*	batch_data = UnionGetIf<VulkanCommandBatch>( &data );
		CHECK_ERR( batch_data );

		for (auto& sem : batch_data->signalSemaphores) {
			CHECK( _submissionGraph.SignalSemaphore( batchId, BitCast<VkSemaphore>(sem) ));
		}

		for (auto& wait_sem : batch_data->waitSemaphores) {
			CHECK( _submissionGraph.WaitSemaphore( batchId, BitCast<VkSemaphore>(wait_sem.first), BitCast<VkPipelineStageFlags>(wait_sem.second) ));
		}

		VDeviceQueueInfoPtr		queue_ptr = null;

		for (auto& queue : _device.GetVkQueues())
		{
			if ( queue.handle == BitCast<VkQueue>(batch_data->queue) )
				queue_ptr = &queue;
		}
		CHECK_ERR( queue_ptr );

		CHECK_ERR( _submissionGraph.Submit( queue_ptr, batchId, indexInBatch,
											ArrayView{ Cast<VkCommandBuffer>(batch_data->commands.data()), batch_data->commands.size() }
					));
		return true;
	}

/*
=================================================
	EndFrame
=================================================
*/
	bool  VFrameGraphInstance::EndFrame ()
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::RunThreads, EState::End ));

		CHECK_ERR( _submissionGraph.IsAllBatchesSubmitted() );

		// complete thread execution
		{
			EXLOCK( _threadLock );
			for (auto iter = _threads.begin(); iter != _threads.end();)
			{
				auto	thread = iter->lock();

				if ( not thread or thread->IsDestroyed() ) {
					iter = _threads.erase( iter );
					continue;
				}

				CHECK( thread->SyncOnExecute( INOUT _statistics ));
				++iter;
			}
		}

		_resourceMngr.OnEndFrame();
		_debugger.OnEndFrame();
		
		CHECK_ERR( _SetState( EState::End, EState::Idle ));
		return true;
	}

/*
=================================================
	WaitIdle
=================================================
*/
	bool  VFrameGraphInstance::WaitIdle ()
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _GetState() == EState::Idle );

		return _WaitIdle();
	}

	bool  VFrameGraphInstance::_WaitIdle ()
	{
		EXLOCK( _threadLock );

		for (uint i = 0; i < _ringBufferSize; ++i)
		{
			const uint	frame_id = (_frameId + i) % _ringBufferSize;
			
			CHECK_ERR( _submissionGraph.WaitFences( frame_id ));

			for (auto iter = _threads.begin(); iter != _threads.end();)
			{
				auto	thread = iter->lock();
				
				if ( not thread or thread->IsDestroyed() ) {
					iter = _threads.erase( iter );
					continue;
				}

				CHECK( thread->OnWaitIdle() );
				++iter;
			}
		}
		return true;
	}

/*
=================================================
	GetStatistics
=================================================
*/
	bool  VFrameGraphInstance::GetStatistics (OUT Statistics &result) const
	{
		SHAREDLOCK( _rcCheck );
		CHECK_ERR( _GetState() == EState::Idle );
		
		result = _statistics;
		return true;
	}
	
/*
=================================================
	DumpToString
=================================================
*/
	bool  VFrameGraphInstance::DumpToString (OUT String &result) const
	{
		SHAREDLOCK( _rcCheck );
		CHECK_ERR( _GetState() == EState::Idle );

		_debugger.GetFrameDump( OUT result );
		return true;
	}
	
/*
=================================================
	DumpToGraphViz
=================================================
*/
	bool  VFrameGraphInstance::DumpToGraphViz (OUT String &result) const
	{
		SHAREDLOCK( _rcCheck );
		CHECK_ERR( _GetState() == EState::Idle );

		_debugger.GetGraphDump( OUT result );
		return true;
	}
	
/*
=================================================
	_GetState / _SetState
=================================================
*/
	inline VFrameGraphInstance::EState  VFrameGraphInstance::_GetState () const
	{
		return _state.load( memory_order_acquire );
	}

	inline void  VFrameGraphInstance::_SetState (EState newState)
	{
		_state.store( newState, memory_order_release );
	}

	inline bool  VFrameGraphInstance::_SetState (EState expected, EState newState)
	{
		return _state.compare_exchange_strong( INOUT expected, newState, memory_order_release, memory_order_relaxed );
	}
	
/*
=================================================
	_IsUnique
=================================================
*/
	bool  VFrameGraphInstance::_IsUnique (VDeviceQueueInfoPtr ptr) const
	{
		for (auto& q : _queueMap) {
			if ( q.second == ptr )
				return false;
		}
		return true;
	}

/*
=================================================
	_AddGraphicsQueue
=================================================
*/
	bool  VFrameGraphInstance::_AddGraphicsQueue ()
	{
		VDeviceQueueInfoPtr		best_match;
		VDeviceQueueInfoPtr		compatible;

		for (auto& queue : _device.GetVkQueues())
		{
			const bool	is_unique		= _IsUnique( &queue );
			const bool	has_graphics	= EnumEq( queue.familyFlags, VK_QUEUE_GRAPHICS_BIT );

			if ( has_graphics )
			{
				compatible = &queue;

				if ( is_unique ) {
					best_match = &queue;
					break;
				}
			}
		}
		
		if ( not best_match )
			best_match = compatible;

		if ( best_match )
		{
			_queueMap.insert({ EQueueUsage::Graphics, best_match });
			return true;
		}
		return false;
	}
	
/*
=================================================
	_AddAsyncComputeQueue
=================================================
*/
	bool  VFrameGraphInstance::_AddAsyncComputeQueue ()
	{
		VDeviceQueueInfoPtr		unique;
		VDeviceQueueInfoPtr		best_match;
		VDeviceQueueInfoPtr		compatible;

		for (auto& queue : _device.GetVkQueues())
		{
			const bool	is_unique		= _IsUnique( &queue );
			const bool	has_compute		= EnumEq( queue.familyFlags, VK_QUEUE_COMPUTE_BIT );
			const bool	has_graphics	= EnumEq( queue.familyFlags, VK_QUEUE_GRAPHICS_BIT );

			// compute without graphics
			if ( has_compute and not has_graphics )
			{
				compatible = &queue;

				if ( is_unique ) {
					best_match = &queue;
					break;
				}
			}
			else
			
			// any unique queue that supports compute
			if ( (has_compute or has_graphics) and is_unique )
			{
				unique = &queue;
			}
		}

		// unique compute/graphics queue is better than non-unique compute queue
		if ( not best_match )
			best_match = unique;
		
		if ( not best_match )
			best_match = compatible;
		
		if ( best_match )
		{
			_queueMap.insert({ EQueueUsage::AsyncCompute, best_match });
			return true;
		}
		return false;
	}
	
/*
=================================================
	_AddAsyncTransferQueue
=================================================
*/
	bool  VFrameGraphInstance::_AddAsyncTransferQueue ()
	{
		VDeviceQueueInfoPtr		unique;
		VDeviceQueueInfoPtr		best_match;
		VDeviceQueueInfoPtr		compatible;

		for (auto& queue : _device.GetVkQueues())
		{
			const bool	is_unique			= _IsUnique( &queue );
			const bool	has_transfer		= EnumEq( queue.familyFlags, VK_QUEUE_TRANSFER_BIT );
			const bool	supports_transfer	= EnumAny( queue.familyFlags, VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT );

			// transfer without graphics or compute
			if ( has_transfer and not supports_transfer )
			{
				compatible = &queue;

				if ( is_unique ) {
					best_match = &queue;
					break;
				}
			}
			else
			
			// any unique queue that supports transfer
			if ( (has_transfer or supports_transfer) and is_unique )
			{
				unique = &queue;
			}
		}
		
		// unique compute/graphics queue is better than non-unique transfer queue
		if ( not best_match )
			best_match = unique;
		
		if ( not best_match )
			best_match = compatible;
		
		if ( best_match )
		{
			_queueMap.insert({ EQueueUsage::AsyncTransfer, best_match });
			return true;
		}
		return false;
	}

/*
=================================================
	FindQueue
=================================================
*/
	VDeviceQueueInfoPtr  VFrameGraphInstance::FindQueue (EQueueUsage type) const
	{
		auto	iter = _queueMap.find( type );
		return iter != _queueMap.end() ? iter->second : null;
	}
	
/*
=================================================
	GetQueuesMask
=================================================
*/
	EQueueFamilyMask  VFrameGraphInstance::GetQueuesMask (EQueueUsage types) const
	{
		EQueueFamilyMask	mask = Default;

		for (uint i = 0; (1u<<i) <= uint(types); ++i)
		{
			if ( not EnumEq( types, 1u<<i ) )
				continue;

			auto	iter = _queueMap.find( EQueueUsage(1u<<i) );
			if ( iter != _queueMap.end() )
				mask |= iter->second->familyIndex;
		}
		return mask;
	}


}	// FG

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
		_submissionGraph{ _device },
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
		SCOPELOCK( _rcCheck );
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
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _IsInitialized() );

		EThreadUsage	usage = desc.usage;

		// validate usage
		if ( EnumEq( usage, EThreadUsage::AsyncStreaming ) )
			usage |= (EThreadUsage::Transfer | EThreadUsage::MemAllocation);

		if ( EnumEq( usage, EThreadUsage::Transfer ) )
			usage |= EThreadUsage::MemAllocation;

		// create thread
		SharedPtr<VFrameGraphThread>	thread{ new VFrameGraphThread{ *this, usage, desc.relative, desc.name }};

		for (auto& comp : _pplnCompilers) {
			thread->AddPipelineCompiler( comp );
		}

		thread->SetCompilationFlags( _defaultCompilationFlags, _defaultDebugFlags );

		SCOPELOCK( _threadLock );
		_threads.push_back( thread );

		return thread;
	}
	
/*
=================================================
	_IsInitialized
=================================================
*/
	inline bool  VFrameGraph::_IsInitialized () const
	{
		return _ringBufferSize > 0;
	}

/*
=================================================
	Initialize
=================================================
*/
	bool  VFrameGraph::Initialize (uint ringBufferSize)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( not _IsInitialized() );

		_ringBufferSize	= ringBufferSize;
		_frameId		= 0;

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
	void  VFrameGraph::Deinitialize ()
	{
		SCOPELOCK( _rcCheck );
		CHECK( _IsInitialized() );
		
		// checks if all threads was destroyed
		{
			SCOPELOCK( _threadLock );
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
	bool  VFrameGraph::AddPipelineCompiler (const IPipelineCompilerPtr &comp)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _GetState() == EState::Idle );

		if ( not _pplnCompilers.insert( comp ).second )
			return true;
		
		SCOPELOCK( _threadLock );
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
	void  VFrameGraph::SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags)
	{
		SCOPELOCK( _rcCheck );

		_defaultCompilationFlags	= flags;
		_defaultDebugFlags			= debugFlags;
	}
	
/*
=================================================
	Begin
=================================================
*/
	bool  VFrameGraph::Begin (const SubmissionGraph &graph)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::Idle, EState::Begin ));

		_frameId = (_frameId + 1) % _ringBufferSize;

		CHECK_ERR( _submissionGraph.WaitFences( _frameId ));
		CHECK_ERR( _submissionGraph.Recreate( _frameId, graph ));

		// begin thread execution
		{
			SCOPELOCK( _threadLock );
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

			CHECK_ERR( _submissionGraph.TotalCount() <= _threads.size() );
		}


		_debugger.OnBeginFrame( graph );
		_resourceMngr.OnBeginFrame( _frameId );

		CHECK_ERR( _SetState( EState::Begin, EState::RunThreads ));
		return true;
	}

/*
=================================================
	Execute
=================================================
*/
	bool  VFrameGraph::Execute ()
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _SetState( EState::RunThreads, EState::Execute ));

		Array< VFrameGraphThread *>		present_threads;

		// complete thread execution
		{
			SCOPELOCK( _threadLock );
			for (auto iter = _threads.begin(); iter != _threads.end();)
			{
				auto	thread = iter->lock();

				if ( not thread or thread->IsDestroyed() ) {
					iter = _threads.erase( iter );
					continue;
				}

				if ( EnumEq( thread->GetThreadUsage(), EThreadUsage::Present ) )
					present_threads.push_back( thread.get() );

				CHECK( thread->SyncOnExecute() );
				++iter;
			}
		}

		CHECK_ERR( _submissionGraph.IsAllBatchesSubmitted() );

		// present swapchains
		{
			for (auto& thread : present_threads)
			{
				CHECK( thread->Present() );
			}
		}

		_resourceMngr.OnEndFrame();
		_debugger.OnEndFrame();
		
		CHECK_ERR( _SetState( EState::Execute, EState::Idle ));
		return true;
	}

/*
=================================================
	WaitIdle
=================================================
*/
	bool  VFrameGraph::WaitIdle ()
	{
		SCOPELOCK( _rcCheck );
		return _WaitIdle();
	}

	bool  VFrameGraph::_WaitIdle ()
	{
		SCOPELOCK( _threadLock );

		for (uint i = 0; i < _ringBufferSize; ++i)
		{
			const uint	frame_id = (_frameId + i) % _ringBufferSize;
			
			CHECK_ERR( _submissionGraph.WaitFences( frame_id ));

			for (auto iter = _threads.begin(); iter != _threads.end(); ++iter)
			{
				auto	thread = iter->lock();
				if ( thread ) {
					CHECK( thread->OnWaitIdle() );
				}
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
		SHAREDLOCK( _rcCheck );
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
		SHAREDLOCK( _rcCheck );

		_debugger.GetFrameDump( OUT result );
		return true;
	}
	
/*
=================================================
	DumpToGraphViz
=================================================
*/
	bool  VFrameGraph::DumpToGraphViz (EGraphVizFlags, OUT String &result) const
	{
		SHAREDLOCK( _rcCheck );

		_debugger.GetGraphDump( OUT result );
		return true;
	}
	
/*
=================================================
	_GetState / _SetState
=================================================
*/
	inline VFrameGraph::EState  VFrameGraph::_GetState () const
	{
		return _state.load( memory_order_acquire );
	}

	inline void  VFrameGraph::_SetState (EState newState)
	{
		_state.store( newState, memory_order_release );
	}

	inline bool  VFrameGraph::_SetState (EState expected, EState newState)
	{
		return _state.compare_exchange_strong( INOUT expected, newState, memory_order_release, memory_order_relaxed );
	}


}	// FG

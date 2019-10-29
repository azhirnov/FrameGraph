// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VLocalRTScene.h"
#include "VEnumCast.h"
#include "VTaskGraph.h"
#include "VBarrierManager.h"
#include "VResourceManager.h"
#include "VLocalDebugger.h"

namespace FG
{

/*
=================================================
	destructor
=================================================
*/
	VLocalRTScene::~VLocalRTScene ()
	{
		ASSERT( _rtSceneData == null );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VLocalRTScene::Create (const VRayTracingScene *sceneData)
	{
		CHECK_ERR( _rtSceneData == null );
		CHECK_ERR( sceneData );

		_rtSceneData = sceneData;

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VLocalRTScene::Destroy ()
	{
		_rtSceneData = null;
		
		// check for uncommited barriers
		ASSERT( _pendingAccesses.empty() );
		ASSERT( _accessForReadWrite.empty() );
		
		_pendingAccesses	= Default;
		_accessForReadWrite	= Default;
	}

/*
=================================================
	AddPendingState
=================================================
*/
	void VLocalRTScene::AddPendingState (const SceneState &st) const
	{
		ASSERT( st.task );

		_pendingAccesses.stages		= EResourceState_ToPipelineStages( st.state );
		_pendingAccesses.access		= EResourceState_ToAccess( st.state );
		_pendingAccesses.isReadable	= EResourceState_IsReadable( st.state );
		_pendingAccesses.isWritable	= EResourceState_IsWritable( st.state );
		_pendingAccesses.index		= st.task->ExecutionOrder();
	}

/*
=================================================
	ResetState
=================================================
*/
	void VLocalRTScene::ResetState (ExeOrderIndex index, VBarrierManager &barrierMngr, Ptr<VLocalDebugger> debugger) const
	{
		ASSERT( _pendingAccesses.empty() );	// you must commit all pending states before reseting
		
		// add full range barrier
		_pendingAccesses.isReadable	= true;
		_pendingAccesses.isWritable	= false;
		_pendingAccesses.stages		= 0;
		_pendingAccesses.access		= 0;
		_pendingAccesses.index		= index;

		CommitBarrier( barrierMngr, debugger );

		// flush
		_accessForReadWrite = Default;
	}
	
/*
=================================================
	CommitBarrier
=================================================
*/
	void VLocalRTScene::CommitBarrier (VBarrierManager &barrierMngr, Ptr<VLocalDebugger> debugger) const
	{
		const bool	is_modified =	(_accessForReadWrite.isReadable and _pendingAccesses.isWritable) or	// read -> write
									_accessForReadWrite.isWritable;										// write -> read/write

		if ( is_modified )
		{
			VkMemoryBarrier		barrier = {};
			barrier.sType			= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask	= _accessForReadWrite.access;
			barrier.dstAccessMask	= _pendingAccesses.access;
			barrierMngr.AddMemoryBarrier( _accessForReadWrite.stages, _pendingAccesses.stages, barrier );

			if ( debugger ) {
				debugger->AddRayTracingBarrier( _rtSceneData.get(), _accessForReadWrite.index, _pendingAccesses.index,
												_accessForReadWrite.stages, _pendingAccesses.stages, 0, barrier );
			}

			_accessForReadWrite = _pendingAccesses;
		}
		else
		{
			_accessForReadWrite.stages		|= _pendingAccesses.stages;
			_accessForReadWrite.access		 = _pendingAccesses.access;
			_accessForReadWrite.index		 = _pendingAccesses.index;
			_accessForReadWrite.isReadable	|= _pendingAccesses.isReadable;
			_accessForReadWrite.isWritable	|= _pendingAccesses.isWritable;
		}

		_pendingAccesses = Default;
	}


}	// FG

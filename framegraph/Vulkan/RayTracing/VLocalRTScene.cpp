// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VLocalRTScene.h"
#include "VEnumCast.h"
#include "VTaskGraph.h"
#include "VBarrierManager.h"
#include "VFrameGraphDebugger.h"

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
		SCOPELOCK( _rcCheck );
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
	void VLocalRTScene::Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t)
	{
		SCOPELOCK( _rcCheck );

		_rtSceneData			= null;
		_hitShadersPerGeometry	= 0;
		_maxHitShaderCount		= 0;
		_geometryInstances.clear();
		
		// check for uncommited barriers
		ASSERT( _pendingAccesses.empty() );
		ASSERT( _accessForReadWrite.empty() );
		
		_pendingAccesses	= Default;
		_accessForReadWrite	= Default;
	}
	
/*
=================================================
	SetGeometryInstances
=================================================
*/
	void VLocalRTScene::SetGeometryInstances (ArrayView<RawRTGeometryID> instances, uint hitShadersPerGeometry, uint maxHitShaders) const
	{
		_geometryInstances.assign( instances.begin(), instances.end() );
		_hitShadersPerGeometry	= hitShadersPerGeometry;
		_maxHitShaderCount		= maxHitShaders;
	}

/*
=================================================
	AddPendingState
=================================================
*/
	void VLocalRTScene::AddPendingState (const SceneState &st) const
	{
		ASSERT( st.task );
		SCOPELOCK( _rcCheck );

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
	void VLocalRTScene::ResetState (ExeOrderIndex index, VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const
	{
		SCOPELOCK( _rcCheck );
		ASSERT( _pendingAccesses.empty() );	// you must commit all pending states before reseting
		
		// add full range barrier
		_pendingAccesses.isReadable	= true;
		_pendingAccesses.isWritable	= false;
		_pendingAccesses.stages		= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
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
	void VLocalRTScene::CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const
	{
		SCOPELOCK( _rcCheck );
		
		const bool	is_modified =	(_accessForReadWrite.isReadable and _pendingAccesses.isWritable) or	// read -> write
									_accessForReadWrite.isWritable;										// write -> read/write

		if ( is_modified )
		{
			VkMemoryBarrier		barrier = {};
			barrier.sType			= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask	= _accessForReadWrite.access;
			barrier.dstAccessMask	= _pendingAccesses.access;
			barrierMngr.AddMemoryBarrier( _accessForReadWrite.stages, _pendingAccesses.stages, 0, barrier );

			if ( debugger )
				debugger->AddRayTracingBarrier( _rtSceneData, _accessForReadWrite.index, _pendingAccesses.index,
											    _accessForReadWrite.stages, _pendingAccesses.stages, 0, barrier );

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

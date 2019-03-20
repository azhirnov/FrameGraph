// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VLocalRTScene.h"
#include "VEnumCast.h"
#include "VTaskGraph.h"
#include "VBarrierManager.h"
#include "VResourceManager.h"

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
	void VLocalRTScene::Destroy (VResourceManager &resMngr, ExeOrderIndex batchExeOrder, uint frameIndex)
	{
		if ( _rtSceneData and _instancesData.has_value() )
		{
			_rtSceneData->Merge( INOUT _instancesData.value(), batchExeOrder, frameIndex );
		}
		else
			ASSERT( not _instancesData.has_value() );

		if ( _instancesData.has_value() )
		{
			for (auto& inst : _instancesData->geometryInstances) {
				resMngr.ReleaseResource( inst.geometry.Release() );
			}
		}

		_rtSceneData	= null;
		_instancesData	= {};
		
		// check for uncommited barriers
		ASSERT( _pendingAccesses.empty() );
		ASSERT( _accessForReadWrite.empty() );
		
		_pendingAccesses	= Default;
		_accessForReadWrite	= Default;
	}
	
/*
=================================================
	SetGeometryInstances
----
	'instances' is sorted by instance ID and contains the strong references for geometries
=================================================
*/
	void VLocalRTScene::SetGeometryInstances (Tuple<InstanceID, RTGeometryID, uint> *instances, uint instanceCount, uint hitShadersPerInstance, uint maxHitShaders) const
	{
		_instancesData = InstancesData{};
		_instancesData->geometryInstances.reserve( instanceCount );

		for (uint i = 0; i < instanceCount; ++i) {
			_instancesData->geometryInstances.emplace_back( std::get<0>(instances[i]), std::move(std::get<1>(instances[i])), std::get<2>(instances[i]) );
		}

		_instancesData->hitShadersPerInstance	= hitShadersPerInstance;
		_instancesData->maxHitShaderCount		= maxHitShaders;
	}
	
/*
=================================================
	FindInstance
=================================================
*/
	VLocalRTScene::Instance const*  VLocalRTScene::FindInstance (const InstanceID &id) const
	{
		auto	instances	= GeometryInstances();
		size_t	pos			= BinarySearch( instances, id );

		return pos < instances.size() ? &instances[pos] : null;
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
		_pendingAccesses.index		= Cast<VFrameGraphTask>(st.task)->ExecutionOrder();
	}

/*
=================================================
	ResetState
=================================================
*/
	void VLocalRTScene::ResetState (ExeOrderIndex index, VBarrierManager &barrierMngr, Ptr<VFrameGraphDebugger> debugger) const
	{
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
	void VLocalRTScene::CommitBarrier (VBarrierManager &barrierMngr, Ptr<VFrameGraphDebugger> debugger) const
	{
		const bool	is_modified =	(_accessForReadWrite.isReadable and _pendingAccesses.isWritable) or	// read -> write
									_accessForReadWrite.isWritable;										// write -> read/write

		if ( is_modified )
		{
			VkMemoryBarrier		barrier = {};
			barrier.sType			= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask	= _accessForReadWrite.access;
			barrier.dstAccessMask	= _pendingAccesses.access;
			barrierMngr.AddMemoryBarrier( _accessForReadWrite.stages, _pendingAccesses.stages, 0, barrier );

			//if ( debugger ) {
			//	debugger->AddRayTracingBarrier( _rtSceneData, _accessForReadWrite.index, _pendingAccesses.index,
			//								    _accessForReadWrite.stages, _pendingAccesses.stages, 0, barrier );
			//}

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

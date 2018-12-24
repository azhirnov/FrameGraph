// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/EResourceState.h"
#include "VRayTracingScene.h"

namespace FG
{

	//
	// Vulkan Ray Tracing Scene thread local
	//

	class VLocalRTScene final
	{
	// types
	public:
		struct SceneState
		{
			EResourceState		state;
			Task				task;
		};
		
	private:
		struct SceneAccess
		{
			VkPipelineStageFlags	stages		= 0;
			VkAccessFlags			access		= 0;
			ExeOrderIndex			index		= ExeOrderIndex::Initial;
			bool					isReadable : 1;
			bool					isWritable : 1;

			SceneAccess () : isReadable{false}, isWritable{false} {}

			ND_ bool empty ()	const	{ return not (isReadable or isWritable); }
		};

		using AccessRecords_t	= FixedArray< SceneAccess, 16 >;
		using InstancesData_t	= VRayTracingScene::InstancesData;
		using CurrentData_t		= Optional< InstancesData_t >;


	// variables
	private:
		VRayTracingScene const *		_rtSceneData			= null;		// readonly access is thread safe

		mutable CurrentData_t			_instancesData;

		mutable SceneAccess				_pendingAccesses;
		mutable SceneAccess				_accessForReadWrite;

		RWRaceConditionCheck			_rcCheck;


	// methods
	public:
		VLocalRTScene () {}
		VLocalRTScene (VLocalRTScene &&) = delete;
		~VLocalRTScene ();
		
		bool Create (const VRayTracingScene *);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t, ExeOrderIndex, uint);
		
		void AddPendingState (const SceneState &state) const;
		void CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const;
		void ResetState (ExeOrderIndex index, VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const;

		void SetGeometryInstances (ArrayView<RawRTGeometryID> instances, uint hitShadersPerGeometry, uint maxHitShaders) const;

		ND_ VkAccelerationStructureNV	Handle ()					const	{ SHAREDLOCK( _rcCheck );  return _rtSceneData->Handle(); }
		ND_ ERayTracingFlags			GetFlags ()					const	{ SHAREDLOCK( _rcCheck );  return _rtSceneData->GetFlags(); }
		ND_ uint						InstanceCount ()			const	{ SHAREDLOCK( _rcCheck );  return _rtSceneData->InstanceCount(); }
		ND_ ArrayView<RawRTGeometryID>	GeometryInstances ()		const	{ SHAREDLOCK( _rcCheck );  return _GetData().geometryInstances; }
		ND_ uint						HitShadersPerGeometry ()	const	{ SHAREDLOCK( _rcCheck );  return _GetData().hitShadersPerGeometry; }
		ND_ uint						MaxHitShaderCount ()		const	{ SHAREDLOCK( _rcCheck );  return _GetData().maxHitShaderCount; }
		ND_ VRayTracingScene const*		ToGlobal ()					const	{ SHAREDLOCK( _rcCheck );  return _rtSceneData; }
		ND_ bool						HasUncommitedChanges ()		const	{ SHAREDLOCK( _rcCheck );  return _instancesData.has_value(); }

	private:
		ND_ InstancesData_t const&		_GetData ()					const	{ return _instancesData.has_value() ? _instancesData.value() : _rtSceneData->CurrentData(); }
	};


}	// FG

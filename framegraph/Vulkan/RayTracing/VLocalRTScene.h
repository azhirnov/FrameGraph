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

		using Instance = VRayTracingScene::Instance;
		
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


	// variables
	private:
		VRayTracingScene const *		_rtSceneData			= null;		// readonly access is thread safe

		mutable Array<RawRTGeometryID>	_geometryInstances;
		mutable uint					_hitShadersPerGeometry	= 0;
		mutable uint					_maxHitShaderCount		= 0;

		mutable SceneAccess				_pendingAccesses;
		mutable SceneAccess				_accessForReadWrite;

		RWRaceConditionCheck			_rcCheck;


	// methods
	public:
		VLocalRTScene () {}
		VLocalRTScene (VLocalRTScene &&) = delete;
		~VLocalRTScene ();
		
		bool Create (const VRayTracingScene *);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		
		void AddPendingState (const SceneState &state) const;
		void CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const;
		void ResetState (ExeOrderIndex index, VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const;

		void SetGeometryInstances (ArrayView<RawRTGeometryID> instances, uint hitShadersPerGeometry, uint maxHitShaders) const;

		ND_ VkAccelerationStructureNV	Handle ()					const	{ SHAREDLOCK( _rcCheck );  return _rtSceneData->Handle(); }
		ND_ ERayTracingFlags			GetFlags ()					const	{ SHAREDLOCK( _rcCheck );  return _rtSceneData->GetFlags(); }
		ND_ uint						InstanceCount ()			const	{ SHAREDLOCK( _rcCheck );  return _rtSceneData->InstanceCount(); }
		ND_ ArrayView<RawRTGeometryID>	GeometryInstances ()		const	{ SHAREDLOCK( _rcCheck );  return _geometryInstances; }
		ND_ uint						HitShadersPerGeometry ()	const	{ SHAREDLOCK( _rcCheck );  return _hitShadersPerGeometry; }
		ND_ uint						MaxHitShaderCount ()		const	{ SHAREDLOCK( _rcCheck );  return _maxHitShaderCount; }
		ND_ VRayTracingScene const*		ToGlobal ()					const	{ SHAREDLOCK( _rcCheck );  return _rtSceneData; }
	};


}	// FG

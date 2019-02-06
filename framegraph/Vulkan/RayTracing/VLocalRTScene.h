// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
		using Instance			= VRayTracingScene::Instance;
		using InstancesData		= VRayTracingScene::InstancesData;
		using CurrentData_t		= Optional< InstancesData >;


	// variables
	private:
		VRayTracingScene const *	_rtSceneData			= null;		// readonly access is thread safe

		mutable CurrentData_t		_instancesData;

		mutable SceneAccess			_pendingAccesses;
		mutable SceneAccess			_accessForReadWrite;


	// methods
	public:
		VLocalRTScene () {}
		VLocalRTScene (VLocalRTScene &&) = delete;
		~VLocalRTScene ();
		
		bool Create (const VRayTracingScene *);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t, ExeOrderIndex, uint);
		
		void AddPendingState (const SceneState &state) const;
		void CommitBarrier (VBarrierManager &barrierMngr, Ptr<VFrameGraphDebugger> debugger) const;
		void ResetState (ExeOrderIndex index, VBarrierManager &barrierMngr, Ptr<VFrameGraphDebugger> debugger) const;

		void SetGeometryInstances (Pair<InstanceID, RTGeometryID> *instances, uint instanceCount, uint hitShadersPerInstance, uint maxHitShaders) const;

		ND_ Instance const*  FindInstance (const InstanceID &id) const;

		ND_ VkAccelerationStructureNV	Handle ()					const	{ return _rtSceneData->Handle(); }
		ND_ ERayTracingFlags			GetFlags ()					const	{ return _rtSceneData->GetFlags(); }
		ND_ uint						MaxInstanceCount ()			const	{ return _rtSceneData->MaxInstanceCount(); }
		ND_ ArrayView<Instance>			GeometryInstances ()		const	{ return _GetData().geometryInstances; }
		ND_ uint						HitShadersPerInstance ()	const	{ return _GetData().hitShadersPerInstance; }
		ND_ uint						MaxHitShaderCount ()		const	{ return _GetData().maxHitShaderCount; }
		ND_ VRayTracingScene const*		ToGlobal ()					const	{ return _rtSceneData; }
		ND_ bool						HasUncommitedChanges ()		const	{ return _instancesData.has_value(); }

	private:
		ND_ InstancesData const&		_GetData ()					const	{ return _instancesData.has_value() ? _instancesData.value() : _rtSceneData->CurrentData(); }
	};


}	// FG

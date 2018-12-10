// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/EResourceState.h"
#include "VRayTracingGeometry.h"

namespace FG
{

	//
	// Vulkan Ray Tracing Geometry thread local
	//

	class VLocalRTGeometry final
	{
	// types
	public:
		struct GeometryState
		{
			EResourceState	state;
			Task			task;
		};
		
		using Triangles	= VRayTracingGeometry::Triangles;
		using AABB		= VRayTracingGeometry::AABB;
		
	private:
		struct GeometryAccess
		{
			VkPipelineStageFlags	stages		= 0;
			VkAccessFlags			access		= 0;
			ExeOrderIndex			index		= ExeOrderIndex::Initial;
			bool					isReadable : 1;
			bool					isWritable : 1;

			GeometryAccess () : isReadable{false}, isWritable{false} {}

			ND_ bool empty ()	const	{ return not (isReadable or isWritable); }
		};


	// variables
	private:
		VRayTracingGeometry const*	_rtGeometryData		= null;		// readonly access is thread safe
		
		mutable GeometryAccess		_pendingAccesses;
		mutable GeometryAccess		_accessForReadWrite;

		RWRaceConditionCheck		_rcCheck;


	// methods
	public:
		VLocalRTGeometry () {}
		VLocalRTGeometry (VLocalRTGeometry &&) = delete;
		~VLocalRTGeometry ();
		
		bool Create (const VRayTracingGeometry *);
		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);
		
		void AddPendingState (const GeometryState &state) const;
		void CommitBarrier (VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const;
		void ResetState (ExeOrderIndex index, VBarrierManager &barrierMngr, VFrameGraphDebugger *debugger) const;
		
		ND_ BLASHandle_t				BLASHandle ()		const	{ SHAREDLOCK( _rcCheck );  return _rtGeometryData->BLASHandle(); }
		ND_ VkAccelerationStructureNV	Handle ()			const	{ SHAREDLOCK( _rcCheck );  return _rtGeometryData->Handle(); }

		ND_ ArrayView<Triangles>		GetTriangles ()		const	{ SHAREDLOCK( _rcCheck );  return _rtGeometryData->GetTriangles(); }
		ND_ ArrayView<AABB>				GetAABBs ()			const	{ SHAREDLOCK( _rcCheck );  return _rtGeometryData->GetAABBs(); }
		ND_ uint						MaxGeometryCount ()	const	{ SHAREDLOCK( _rcCheck );  return uint(GetTriangles().size() + GetAABBs().size()); }
		ND_ ERayTracingFlags			GetFlags ()			const	{ SHAREDLOCK( _rcCheck );  return _rtGeometryData->GetFlags(); }

		ND_ VRayTracingGeometry const*	ToGlobal ()			const	{ SHAREDLOCK( _rcCheck );  return _rtGeometryData; }
	};


}	// FG

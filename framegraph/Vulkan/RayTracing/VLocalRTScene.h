// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/EResourceState.h"
#include "VRayTracingScene.h"

#ifdef VK_NV_ray_tracing

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
			VTask				task;
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


	// variables
	private:
		Ptr<VRayTracingScene const>	_rtSceneData;		// readonly access is thread safe

		mutable SceneAccess			_pendingAccesses;
		mutable SceneAccess			_accessForReadWrite;


	// methods
	public:
		VLocalRTScene () {}
		VLocalRTScene (VLocalRTScene &&) = delete;
		~VLocalRTScene ();
		
		bool Create (const VRayTracingScene *);
		void Destroy ();
		
		void AddPendingState (const SceneState &state) const;
		void CommitBarrier (VBarrierManager &barrierMngr, Ptr<VLocalDebugger> debugger) const;
		void ResetState (ExeOrderIndex index, VBarrierManager &barrierMngr, Ptr<VLocalDebugger> debugger) const;

		ND_ VkAccelerationStructureNV	Handle ()					const	{ return _rtSceneData->Handle(); }
		ND_ ERayTracingFlags			GetFlags ()					const	{ return _rtSceneData->GetFlags(); }
		ND_ uint						MaxInstanceCount ()			const	{ return _rtSceneData->MaxInstanceCount(); }
		ND_ VRayTracingScene const*		ToGlobal ()					const	{ return _rtSceneData.get(); }
	};


}	// FG

#endif	// VK_NV_ray_tracing

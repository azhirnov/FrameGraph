// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/RayTracingSceneDesc.h"
#include "VRayTracingGeometry.h"

namespace FG
{

	//
	// Vulkan Ray Tracing Scene immutable data
	//

	class VRayTracingScene final
	{
	// types
	public:
		using BLASHandle_t = VRayTracingGeometry::BLASHandle_t;

		struct VkGeometryInstance
		{
			// 4x3 row-major matrix
			float4			transformRow0;
			float4			transformRow1;
			float4			transformRow2;

			uint			instanceId		: 24;
			uint			mask			: 8;
			uint			instanceOffset	: 24;
			uint			flags			: 8;
			BLASHandle_t	blasHandle;
		};


	// variables
	private:
		VkAccelerationStructureNV	_topLevelAS		= VK_NULL_HANDLE;
		MemoryID					_memoryId;
		
		Array<VkGeometryInstance>	_instances;

		EQueueFamily				_currQueueFamily	= Default;
		DebugName_t					_debugName;

		RWRaceConditionCheck		_rcCheck;


	// methods
	public:
		VRayTracingScene () {}
		~VRayTracingScene ();
		
		bool Create (const VResourceManagerThread &rm, const RayTracingSceneDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
					 EQueueFamily queueFamily, StringView dbgName);

		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);


		ND_ VkAccelerationStructureNV		Handle ()				const	{ SHAREDLOCK( _rcCheck );  return _topLevelAS; }
		ND_ ArrayView<VkGeometryInstance>	GetInstances ()			const	{ SHAREDLOCK( _rcCheck );  return _instances; }
		
		ND_ EQueueFamily					CurrentQueueFamily ()	const	{ SHAREDLOCK( _rcCheck );  return _currQueueFamily; }
		ND_ StringView						GetDebugName ()			const	{ SHAREDLOCK( _rcCheck );  return _debugName; }
	};


}	// FG

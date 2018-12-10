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
		struct Instance
		{
			RawRTGeometryID		geometryId;
			uint				geometryCount	= 0;
		};


	// variables
	private:
		VkAccelerationStructureNV	_topLevelAS			= VK_NULL_HANDLE;
		MemoryID					_memoryId;
		
		Array< Instance> 			_instances;
		ERayTracingFlags			_flags				= Default;

		EQueueFamily				_currQueueFamily	= Default;
		DebugName_t					_debugName;

		RWRaceConditionCheck		_rcCheck;


	// methods
	public:
		VRayTracingScene () {}
		~VRayTracingScene ();
		
		bool Create (const VDevice &dev, const RayTracingSceneDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
					 EQueueFamily queueFamily, StringView dbgName);

		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);


		ND_ VkAccelerationStructureNV	Handle ()				const	{ SHAREDLOCK( _rcCheck );  return _topLevelAS; }
		ND_ uint						InstanceCount ()		const	{ SHAREDLOCK( _rcCheck );  return uint(_instances.size()); }
		ND_ ArrayView< Instance >		Instances ()			const	{ SHAREDLOCK( _rcCheck );  return _instances; }

		ND_ ERayTracingFlags			GetFlags ()				const	{ SHAREDLOCK( _rcCheck );  return _flags; }
		ND_ EQueueFamily				CurrentQueueFamily ()	const	{ SHAREDLOCK( _rcCheck );  return _currQueueFamily; }
		ND_ StringView					GetDebugName ()			const	{ SHAREDLOCK( _rcCheck );  return _debugName; }
	};


}	// FG

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/RayTracingGeometryDesc.h"
#include "VCommon.h"

namespace FG
{

	//
	// Vulkan Ray Tracing Geometry immutable data
	//

	class VRayTracingGeometry final
	{
	// types
	public:
		enum BLASHandle_t : uint64_t {};


	// variables
	private:
		VkAccelerationStructureNV	_bottomLevelAS		= VK_NULL_HANDLE;
		MemoryID					_memoryId;
		BLASHandle_t				_handle				= BLASHandle_t(0);
		
		Array<VkGeometryNV>			_geometry;

		EQueueFamily				_currQueueFamily	= Default;
		DebugName_t					_debugName;

		RWRaceConditionCheck		_rcCheck;


	// methods
	public:
		VRayTracingGeometry () {}
		~VRayTracingGeometry ();

		bool Create (const VResourceManagerThread &rm, const RayTracingGeometryDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
					 EQueueFamily queueFamily, StringView dbgName);

		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);


		ND_ BLASHandle_t				Handle ()				const	{ SHAREDLOCK( _rcCheck );  return _handle; }
		ND_ ArrayView<VkGeometryNV>		GetData ()				const	{ SHAREDLOCK( _rcCheck );  return _geometry; }
		
		ND_ EQueueFamily				CurrentQueueFamily ()	const	{ SHAREDLOCK( _rcCheck );  return _currQueueFamily; }
		ND_ StringView					GetDebugName ()			const	{ SHAREDLOCK( _rcCheck );  return _debugName; }
	};


}	// FG

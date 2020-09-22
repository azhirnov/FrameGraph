// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/RayTracingSceneDesc.h"
#include "VRayTracingGeometry.h"

#ifdef VK_NV_ray_tracing

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
			InstanceID			id;
			RTGeometryID		geometry;
			uint				indexOffset	= UMax;

			Instance () {}
			Instance (const InstanceID &inst, RTGeometryID &&geom, uint offset): id{inst}, geometry{std::move(geom)}, indexOffset{offset} {}

			// for sorting and searching
			ND_ bool  operator <  (const InstanceID &rhs) const	{ return id < rhs; }
			ND_ bool  operator == (const InstanceID &rhs) const	{ return id == rhs; }
		};
		
		struct InstancesData
		{
			SharedMutex			guard;
			Array<Instance>		geometryInstances;
			uint				hitShadersPerInstance	= 0;
			uint				maxHitShaderCount		= 0;
		};


	// variables
	private:
		VkAccelerationStructureNV	_topLevelAS			= VK_NULL_HANDLE;
		MemoryID					_memoryId;
		
		uint						_maxInstanceCount	= 0;
		ERayTracingFlags			_flags				= Default;

		mutable InstancesData		_instanceData;

		DebugName_t					_debugName;

		RWDataRaceCheck				_drCheck;


	// methods
	public:
		VRayTracingScene () {}
		VRayTracingScene (VRayTracingScene &&) = delete;
		VRayTracingScene (const VRayTracingScene &) = delete;
		~VRayTracingScene ();
		
		bool Create (VResourceManager &, const RayTracingSceneDesc &desc, RawMemoryID memId, VMemoryObj &memObj, StringView dbgName);
		void Destroy (VResourceManager &);

		void SetGeometryInstances (VResourceManager &, Tuple<InstanceID, RTGeometryID, uint> *instances, uint instanceCount, uint hitShadersPerInstance, uint maxHitShaders) const;

		ND_ VkAccelerationStructureNV	Handle ()				const	{ SHAREDLOCK( _drCheck );  return _topLevelAS; }
		ND_ uint						MaxInstanceCount ()		const	{ SHAREDLOCK( _drCheck );  return _maxInstanceCount; }
		ND_ InstancesData &				CurrentData ()			const	{ SHAREDLOCK( _drCheck );  return _instanceData; }

		ND_ ERayTracingFlags			GetFlags ()				const	{ SHAREDLOCK( _drCheck );  return _flags; }
		ND_ StringView					GetDebugName ()			const	{ SHAREDLOCK( _drCheck );  return _debugName; }
	};


}	// FG

#endif	// VK_NV_ray_tracing

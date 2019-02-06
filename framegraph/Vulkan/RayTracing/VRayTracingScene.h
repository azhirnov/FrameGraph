// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
			InstanceID			id;
			RTGeometryID		geometry;

			Instance () {}
			Instance (Pair<InstanceID, RTGeometryID> &&other) : id{other.first}, geometry{std::move(other.second)} {}

			ND_ bool  operator <  (const InstanceID &rhs) const	{ return id < rhs; }
			ND_ bool  operator == (const InstanceID &rhs) const	{ return id == rhs; }
		};
		
		struct InstancesData
		{
			Array<Instance>		geometryInstances;
			uint				hitShadersPerInstance	= 0;
			uint				maxHitShaderCount		= 0;
		};

	private:
		struct InstancesData2 : InstancesData
		{
			SpinLock				lock;
			ExeOrderIndex			exeOrder	= Default;
			uint					frameIdx	= 0;
		};


	// variables
	private:
		VkAccelerationStructureNV	_topLevelAS			= VK_NULL_HANDLE;
		MemoryID					_memoryId;
		
		uint						_maxInstanceCount	= 0;
		ERayTracingFlags			_flags				= Default;

		mutable InstancesData2		_instanceData [2];
		mutable uint				_currStateIndex : 1;

		DebugName_t					_debugName;

		RWRaceConditionCheck		_rcCheck;


	// methods
	public:
		VRayTracingScene () : _currStateIndex{0} {}
		~VRayTracingScene ();
		
		bool Create (const VDevice &dev, const RayTracingSceneDesc &desc, RawMemoryID memId, VMemoryObj &memObj, StringView dbgName);

		void Destroy (OUT AppendableVkResources_t, OUT AppendableResourceIDs_t);

		void Merge (INOUT InstancesData &, ExeOrderIndex, uint frameIndex) const;
		void CommitChanges (uint frameIndex) const;

		ND_ VkAccelerationStructureNV	Handle ()				const	{ SHAREDLOCK( _rcCheck );  return _topLevelAS; }
		ND_ uint						MaxInstanceCount ()		const	{ SHAREDLOCK( _rcCheck );  return _maxInstanceCount; }
		ND_ InstancesData const&		CurrentData ()			const	{ SHAREDLOCK( _rcCheck );  return _instanceData[_currStateIndex]; }

		ND_ ERayTracingFlags			GetFlags ()				const	{ SHAREDLOCK( _rcCheck );  return _flags; }
		ND_ StringView					GetDebugName ()			const	{ SHAREDLOCK( _rcCheck );  return _debugName; }
	};


}	// FG

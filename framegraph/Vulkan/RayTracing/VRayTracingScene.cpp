// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRayTracingScene.h"
#include "VDevice.h"
#include "VMemoryObj.h"
#include "VEnumCast.h"

namespace FG
{

/*
=================================================
	destructor
=================================================
*/
	VRayTracingScene::~VRayTracingScene ()
	{
		ASSERT( not _topLevelAS );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VRayTracingScene::Create (const VDevice &dev, const RayTracingSceneDesc &desc, RawMemoryID memId, VMemoryObj &memObj, StringView dbgName)
	{
		EXLOCK( _rcCheck );
		CHECK_ERR( _topLevelAS == VK_NULL_HANDLE );
		CHECK_ERR( not _memoryId );
		CHECK_ERR( desc.maxInstanceCount > 0 );

		VkAccelerationStructureCreateInfoNV		info = {};
		info.sType				= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		info.info.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		info.info.type			= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		info.info.instanceCount	= desc.maxInstanceCount;
		info.info.flags			= VEnumCast( desc.flags );

		VK_CHECK( dev.vkCreateAccelerationStructureNV( dev.GetVkDevice(), &info, null, OUT &_topLevelAS ));

		CHECK_ERR( memObj.AllocateForAccelStruct( _topLevelAS ));

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_topLevelAS), dbgName, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV );
		}
		
		_maxInstanceCount	= desc.maxInstanceCount;
		_memoryId			= MemoryID{ memId };
		_debugName			= dbgName;
		_flags				= desc.flags;

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void VRayTracingScene::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		EXLOCK( _rcCheck );

		if ( _topLevelAS ) {
			readyToDelete.emplace_back( VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV, uint64_t(_topLevelAS) );
		}
		
		if ( _memoryId ) {
			unassignIDs.emplace_back( _memoryId.Release() );
		}

		for (auto& data : _instanceData)
		{
			EXLOCK( data.lock );

			Array<Instance>  temp;
			std::swap( data.geometryInstances, temp );

			for (auto& inst : temp) {
				unassignIDs.emplace_back( inst.geometry.Release() );
			}

			data.frameIdx	= 0;
			data.exeOrder	= Default;

			data.lock.unlock();
		}

		_topLevelAS			= VK_NULL_HANDLE;
		_memoryId			= Default;
		_flags				= Default;
		_maxInstanceCount	= 0;

		_debugName.clear();
	}
	
/*
=================================================
	Merge
=================================================
*/
	void VRayTracingScene::Merge (INOUT InstancesData &newData, ExeOrderIndex batchExeOrder, uint frameIndex) const
	{
		const uint	idx		= (_currStateIndex + 1) & 1;
		auto&		pending	= _instanceData[idx];

		EXLOCK( pending.lock );

		if ( frameIndex == pending.frameIdx and batchExeOrder <= pending.exeOrder )
			return;	// 'newData' is older than 'pending'

		std::swap( pending.geometryInstances, newData.geometryInstances );
		pending.exeOrder				= batchExeOrder;
		pending.frameIdx				= frameIndex;
		pending.hitShadersPerInstance	= newData.hitShadersPerInstance;
		pending.maxHitShaderCount		= newData.maxHitShaderCount;
	}
	
/*
=================================================
	CommitChanges
=================================================
*/
	void VRayTracingScene::CommitChanges (uint frameIndex) const
	{
		const uint	idx		= (_currStateIndex + 1) & 1;
		auto&		curr	= _instanceData[ _currStateIndex ];
		auto&		pending	= _instanceData[ idx ];

		if ( curr.frameIdx != frameIndex and pending.frameIdx == frameIndex )
		{
			++_currStateIndex;
		}
	}


}	// FG

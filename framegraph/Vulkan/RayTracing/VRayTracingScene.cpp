// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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
	bool VRayTracingScene::Create (const VDevice &dev, const RayTracingSceneDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
								   EQueueFamily queueFamily, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
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
		
		_instances.resize( desc.maxInstanceCount );

		_memoryId			= MemoryID{ memId };
		_currQueueFamily	= queueFamily;
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
		SCOPELOCK( _rcCheck );

		if ( _topLevelAS ) {
			readyToDelete.emplace_back( VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV, uint64_t(_topLevelAS) );
		}
		
		if ( _memoryId ) {
			unassignIDs.emplace_back( _memoryId.Release() );
		}

		_topLevelAS			= VK_NULL_HANDLE;
		_memoryId			= Default;
		_currQueueFamily	= Default;
		_flags				= Default;
		
		_instances.clear();
		_debugName.clear();
	}


}	// FG

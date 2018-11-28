// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRayTracingScene.h"
#include "VResourceManagerThread.h"
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
	bool VRayTracingScene::Create (const VResourceManagerThread &resMngr, const RayTracingSceneDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
								   EQueueFamily queueFamily, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _topLevelAS == VK_NULL_HANDLE );
		CHECK_ERR( not _memoryId );
		CHECK_ERR( not desc.instances.empty() );

		_instances.resize( desc.instances.size() );

		for (size_t i = 0; i < desc.instances.size(); ++i)
		{
			auto&	src = desc.instances[i];
			auto&	dst = _instances[i];

			ASSERT( src.geometryId );
			ASSERT( (src.instanceID >> 24) == 0 );
			ASSERT( (src.instanceOffset >> 24) == 0 );

			dst.blasHandle		= resMngr.GetResource( src.geometryId )->Handle();
			dst.transformRow0	= src.transform[0];
			dst.transformRow1	= src.transform[1];
			dst.transformRow2	= src.transform[2];
			dst.instanceId		= src.instanceID;
			dst.mask			= src.mask;
			dst.instanceOffset	= src.instanceOffset;
			dst.flags			= VEnumCast( src.flags );
		}

		VkAccelerationStructureCreateInfoNV		info = {};
		info.sType				= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		info.info.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		info.info.type			= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		info.info.instanceCount	= uint(_instances.size());
		
		VDevice const&	dev = resMngr.GetDevice();

		VK_CHECK( dev.vkCreateAccelerationStructureNV( dev.GetVkDevice(), &info, null, OUT &_topLevelAS ));
		CHECK_ERR( memObj.AllocateForAccelStruct( _topLevelAS ));

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_topLevelAS), dbgName, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV );
		}
		
		_memoryId			= MemoryID{ memId };
		_currQueueFamily	= queueFamily;
		_debugName			= dbgName;

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
		
		_debugName.clear();
	}


}	// FG

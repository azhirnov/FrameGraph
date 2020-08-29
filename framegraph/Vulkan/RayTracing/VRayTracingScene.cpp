// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRayTracingScene.h"
#include "VDevice.h"
#include "VMemoryObj.h"
#include "VEnumCast.h"
#include "VResourceManager.h"

#ifdef VK_NV_ray_tracing

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
	bool VRayTracingScene::Create (VResourceManager &resMngr, const RayTracingSceneDesc &desc, RawMemoryID memId, VMemoryObj &memObj, StringView dbgName)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( _topLevelAS == VK_NULL_HANDLE );
		CHECK_ERR( not _memoryId );
		CHECK_ERR( desc.maxInstanceCount > 0 );

		VkAccelerationStructureCreateInfoNV		info = {};
		info.sType				= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		info.info.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		info.info.type			= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		info.info.instanceCount	= desc.maxInstanceCount;
		info.info.flags			= VEnumCast( desc.flags );

		auto&	dev = resMngr.GetDevice();
		VK_CHECK( dev.vkCreateAccelerationStructureNV( dev.GetVkDevice(), &info, null, OUT &_topLevelAS ));

		CHECK_ERR( memObj.AllocateForAccelStruct( resMngr.GetMemoryManager(), _topLevelAS ));

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
	void VRayTracingScene::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		if ( _topLevelAS ) {
			auto&	dev = resMngr.GetDevice();
			dev.vkDestroyAccelerationStructureNV( dev.GetVkDevice(), _topLevelAS, null );
		}
		
		if ( _memoryId ) {
			resMngr.ReleaseResource( _memoryId.Release() );
		}

		{
			EXLOCK( _instanceData.guard );

			Array<Instance>  temp;
			std::swap( _instanceData.geometryInstances, temp );

			for (auto& inst : temp) {
				resMngr.ReleaseResource( inst.geometry.Release() );
			}
		}

		_topLevelAS			= VK_NULL_HANDLE;
		_memoryId			= Default;
		_flags				= Default;
		_maxInstanceCount	= 0;

		_debugName.clear();
	}
	
/*
=================================================
	SetGeometryInstances
----
	'instances' is sorted by instance ID and contains the strong references for geometries
=================================================
*/
	void VRayTracingScene::SetGeometryInstances (VResourceManager &resMngr, Tuple<InstanceID, RTGeometryID, uint> *instances, uint instanceCount, uint hitShadersPerInstance, uint maxHitShaders) const
	{
		EXLOCK( _drCheck );
		EXLOCK( _instanceData.guard );
		
		// release previous geometries
		for (auto& geom : _instanceData.geometryInstances) {
			resMngr.ReleaseResource( geom.geometry.Release() );
		}

		_instanceData.geometryInstances.clear();
		_instanceData.geometryInstances.reserve( instanceCount );

		for (uint i = 0; i < instanceCount; ++i)
		{
			CHECK( resMngr.AcquireResource( std::get<1>(instances[i]).Get() ));
			_instanceData.geometryInstances.emplace_back( std::get<0>(instances[i]), std::move(std::get<1>(instances[i])), std::get<2>(instances[i]) );
		}

		_instanceData.hitShadersPerInstance	= hitShadersPerInstance;
		_instanceData.maxHitShaderCount		= maxHitShaders;
	}


}	// FG

#endif	// VK_NV_ray_tracing

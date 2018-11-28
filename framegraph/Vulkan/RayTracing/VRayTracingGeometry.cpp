// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRayTracingGeometry.h"
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
	VRayTracingGeometry::~VRayTracingGeometry ()
	{
		ASSERT( not _bottomLevelAS );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VRayTracingGeometry::Create (const VResourceManagerThread &resMngr, const RayTracingGeometryDesc &desc, RawMemoryID memId, VMemoryObj &memObj,
									  EQueueFamily queueFamily, StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		CHECK_ERR( _bottomLevelAS == VK_NULL_HANDLE );
		CHECK_ERR( not _memoryId );
		CHECK_ERR( desc.geometry.size() or desc.aabb.size() );

		_geometry.resize( desc.geometry.size() + desc.aabb.size() );

		for (size_t i = 0; i < desc.geometry.size(); ++i)
		{
			auto&	src = desc.geometry[i];
			auto&	dst = _geometry[i];

			ASSERT( src.vertexBuffer );
			ASSERT( src.vertexCount > 0 );

			dst = {};
			dst.sType								= VK_STRUCTURE_TYPE_GEOMETRY_NV;
			dst.geometryType						= VK_GEOMETRY_TYPE_TRIANGLES_NV;
			dst.flags								= VEnumCast( src.flags );
			dst.geometry.aabbs.sType				= VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
			dst.geometry.triangles.sType			= VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
			dst.geometry.triangles.vertexData		= resMngr.GetResource( src.vertexBuffer )->Handle();
			dst.geometry.triangles.vertexOffset		= VkDeviceSize(src.vertexOffset);
			dst.geometry.triangles.vertexCount		= src.vertexCount;
			dst.geometry.triangles.vertexStride		= VkDeviceSize(src.vertexStride);
			dst.geometry.triangles.vertexFormat		= VEnumCast( src.vertexFormat );

			if ( src.indexBuffer )
			{
				ASSERT( src.indexCount > 0 );
				dst.geometry.triangles.indexData	= resMngr.GetResource( src.indexBuffer )->Handle();
				dst.geometry.triangles.indexOffset	= VkDeviceSize(src.indexOffset);
				dst.geometry.triangles.indexCount	= src.indexCount;
				dst.geometry.triangles.indexType	= VEnumCast( src.indexType );
			}
			else
			{
				ASSERT( src.indexCount == 0 );
				ASSERT( src.indexType == EIndex::Unknown );
				dst.geometry.triangles.indexType	= VK_INDEX_TYPE_NONE_NV;
			}

			if ( src.transformBuffer )
			{
				dst.geometry.triangles.transformData	= resMngr.GetResource( src.transformBuffer )->Handle();
				dst.geometry.triangles.transformOffset	= VkDeviceSize(src.transformOffset);
			}
		}

		for (size_t i = 0; i < desc.aabb.size(); ++i)
		{
			auto&	src = desc.aabb[i];
			auto&	dst = _geometry[i + desc.geometry.size()];
			
			ASSERT( src.aabbBuffer );
			ASSERT( src.aabbCount );

			dst = {};
			dst.sType						= VK_STRUCTURE_TYPE_GEOMETRY_NV;
			dst.geometryType				= VK_GEOMETRY_TYPE_AABBS_NV;
			dst.flags						= VEnumCast( src.flags );
			dst.geometry.triangles.sType	= VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
			dst.geometry.aabbs.sType		= VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
			dst.geometry.aabbs.aabbData		= resMngr.GetResource( src.aabbBuffer )->Handle();
			dst.geometry.aabbs.numAABBs		= src.aabbCount;
			dst.geometry.aabbs.offset		= VkDeviceSize(src.aabbOffset);
			dst.geometry.aabbs.stride		= uint(src.aabbStride);
		}
		
		VkAccelerationStructureCreateInfoNV		info = {};
		info.sType				= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		info.info.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		info.info.type			= VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
		info.info.geometryCount	= uint(_geometry.size());
		info.info.pGeometries	= _geometry.data();
		
		VDevice const&	dev = resMngr.GetDevice();

		VK_CHECK( dev.vkCreateAccelerationStructureNV( dev.GetVkDevice(), &info, null, OUT &_bottomLevelAS ));
		CHECK_ERR( memObj.AllocateForAccelStruct( _bottomLevelAS ));

		VK_CHECK( dev.vkGetAccelerationStructureHandleNV( dev.GetVkDevice(), _bottomLevelAS, sizeof(_handle), OUT Cast<uint64_t>(&_handle) ));

		if ( not dbgName.empty() )
		{
			dev.SetObjectName( BitCast<uint64_t>(_bottomLevelAS), dbgName, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV );
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
	void VRayTracingGeometry::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		SCOPELOCK( _rcCheck );

		if ( _bottomLevelAS ) {
			readyToDelete.emplace_back( VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV, uint64_t(_bottomLevelAS) );
		}
		
		if ( _memoryId ) {
			unassignIDs.emplace_back( _memoryId.Release() );
		}

		_bottomLevelAS		= VK_NULL_HANDLE;
		_memoryId			= Default;
		_handle				= BLASHandle_t(0);
		_currQueueFamily	= Default;
		
		Array<VkGeometryNV>		temp;
		std::swap( _geometry, temp );

		_debugName.clear();
	}


}	// FG

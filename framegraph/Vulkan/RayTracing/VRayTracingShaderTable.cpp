// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRayTracingShaderTable.h"

namespace FG
{

/*
=================================================
	destructor
=================================================
*/
	VRayTracingShaderTable::~VRayTracingShaderTable ()
	{
		ASSERT( _tables.empty() );
		ASSERT( not _bufferId );
		ASSERT( not _pipelineId );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool VRayTracingShaderTable::Create (StringView dbgName)
	{
		SCOPELOCK( _rcCheck );
		CHECK( not (_bufferId or _pipelineId) );

		_debugName = dbgName;
		return true;
	}
	
/*
=================================================
	GetBindings
=================================================
*/
	bool VRayTracingShaderTable::GetBindings (EShaderDebugMode mode,
											  OUT RawPipelineLayoutID &layout, OUT VkPipeline &pipeline,
											  OUT VkDeviceSize &blockSize, OUT VkDeviceSize &rayGenOffset,
											  OUT VkDeviceSize &rayMissOffset, OUT VkDeviceSize &rayMissStride,
											  OUT VkDeviceSize &rayHitOffset, OUT VkDeviceSize &rayHitStride,
											  OUT VkDeviceSize &callableOffset, OUT VkDeviceSize &callableStride) const
	{
		SHAREDLOCK( _rcCheck );
		SHAREDLOCK( _guard );

		for (auto& table : _tables)
		{
			if ( table.mode == mode )
			{
				pipeline		= table.pipeline;
				layout			= table.layoutId.Get();
				rayGenOffset	= VkDeviceSize(_rayGenOffset + table.bufferOffset);
				rayMissOffset	= VkDeviceSize(_rayMissOffset + table.bufferOffset);
				rayMissStride	= VkDeviceSize(_rayMissStride);
				rayHitOffset	= VkDeviceSize(_rayHitOffset + table.bufferOffset);
				rayHitStride	= VkDeviceSize(_rayHitStride);
				callableOffset	= VkDeviceSize(_callableOffset + table.bufferOffset);
				callableStride	= VkDeviceSize(_callableStride);
				blockSize		= VkDeviceSize(_blockSize + table.bufferOffset);
				return true;
			}
		}
		return false;
	}

/*
=================================================
	Destroy
=================================================
*/
	void VRayTracingShaderTable::Destroy (OUT AppendableVkResources_t readyToDelete, OUT AppendableResourceIDs_t unassignIDs)
	{
		SCOPELOCK( _rcCheck );

		for (auto& table : _tables) {
			readyToDelete.emplace_back( VK_OBJECT_TYPE_PIPELINE, uint64_t(table.pipeline) );
			unassignIDs.emplace_back( table.layoutId.Release() );
		}

		if ( _bufferId ) {
			unassignIDs.emplace_back( _bufferId.Release() );
		}

		if ( _pipelineId ) {
			unassignIDs.emplace_back( _pipelineId.Release() );
		}

		_tables.clear();
		_debugName.clear();
	}


}	// FG

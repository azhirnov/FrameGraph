// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VRayTracingShaderTable.h"
#include "VResourceManager.h"
#include "VDevice.h"

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
		EXLOCK( _drCheck );
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
		SHAREDLOCK( _drCheck );
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
	void VRayTracingShaderTable::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		auto&	dev = resMngr.GetDevice();

		for (auto& table : _tables) {
			dev.vkDestroyPipeline( dev.GetVkDevice(), table.pipeline, null );
			resMngr.ReleaseResource( table.layoutId.Release() );
		}

		if ( _bufferId ) {
			resMngr.ReleaseResource( _bufferId.Release() );
		}

		if ( _pipelineId ) {
			resMngr.ReleaseResource( _pipelineId.Release() );
		}

		_tables.clear();
		_debugName.clear();
	}


}	// FG

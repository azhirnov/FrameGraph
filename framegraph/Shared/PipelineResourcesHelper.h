// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/PipelineResources.h"
#include "stl/Memory/MemWriter.h"

namespace FG
{


	//
	// Pipeline Resources Helper
	//

	class PipelineResourcesHelper
	{
	public:
		using DynamicData		= PipelineResources::DynamicData;
		using DynamicDataPtr	= PipelineResources::DynamicDataPtr;

		ND_ static DynamicDataPtr  CreateDynamicData (const PipelineDescription::UniformMapPtr &uniforms,
													  uint resourceCount, uint arrayElemCount, uint bufferDynamicOffsetCount);
		
		ND_ static DynamicDataPtr  CloneDynamicData (const PipelineResources &);
		ND_ static DynamicDataPtr  RemoveDynamicData (INOUT PipelineResources &);

		static bool Initialize (OUT PipelineResources &, RawDescriptorSetLayoutID layoutId, const DynamicDataPtr &);

		ND_ static RawPipelineResourcesID  GetCached (const PipelineResources &res)
		{
			return res._GetCachedID();
		}
		
		static void SetCache (const PipelineResources &res, const RawPipelineResourcesID &id)
		{
			res._SetCachedID( id );
		}
	};


}	// FG

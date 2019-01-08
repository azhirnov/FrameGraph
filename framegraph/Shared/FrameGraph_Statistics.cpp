// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/Public/FrameGraphInstance.h"

namespace FG
{

/*
=================================================
	MergeRenderStatistic
=================================================
*/
	inline void MergeRenderStatistic (const FrameGraphInstance::RenderingStatistics &src, INOUT FrameGraphInstance::RenderingStatistics &dst)
	{
		dst.descriptorBinds				+= src.descriptorBinds;
		dst.pushConstants				+= src.pushConstants;
		dst.pipelineBarriers			+= src.pipelineBarriers;
		dst.transferOps					+= src.transferOps;

		dst.indexBufferBindings			+= src.indexBufferBindings;
		dst.vertexBufferBindings		+= src.vertexBufferBindings;
		dst.drawCalls					+= src.drawCalls;
		dst.graphicsPipelineBindings	+= src.graphicsPipelineBindings;
		dst.dynamicStateChanges			+= src.dynamicStateChanges;
		
		dst.dispatchCalls				+= src.dispatchCalls;
		dst.computePipelineBindings		+= src.computePipelineBindings;

		dst.rayTracingPipelineBindings	+= src.rayTracingPipelineBindings;
		dst.traceRaysCalls				+= src.traceRaysCalls;
		dst.buildASCalls				+= src.buildASCalls;

		dst.frameTime					+= src.frameTime;
	}
	
/*
=================================================
	MergeRenderStatistic
=================================================
*/
	inline void MergeResourceStatistic (const FrameGraphInstance::ResourceStatistics &src, INOUT FrameGraphInstance::ResourceStatistics &dst)
	{
		dst.newComputePipelineCount		+= src.newComputePipelineCount;
		dst.newGraphicsPipelineCount	+= src.newGraphicsPipelineCount;
		dst.newRayTracingPipelineCount	+= src.newRayTracingPipelineCount;
	}

/*
=================================================
	Merge
=================================================
*/
	void FrameGraphInstance::Statistics::Merge (const Statistics &newStat)
	{
		MergeRenderStatistic( newStat.renderer, INOUT this->renderer );
		MergeResourceStatistic( newStat.resources, INOUT this->resources );
	}


}	// FG

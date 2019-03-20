// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/Public/FrameGraph.h"

namespace FG
{

/*
=================================================
	MergeRenderStatistic
=================================================
*/
	inline void MergeRenderStatistic (const IFrameGraph::RenderingStatistics &src, INOUT IFrameGraph::RenderingStatistics &dst)
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

		dst.gpuTime						+= src.gpuTime;
		dst.cpuTime						+= src.cpuTime;
	}
	
/*
=================================================
	MergeRenderStatistic
=================================================
*/
	inline void MergeResourceStatistic (const IFrameGraph::ResourceStatistics &src, INOUT IFrameGraph::ResourceStatistics &dst)
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
	void IFrameGraph::Statistics::Merge (const Statistics &newStat)
	{
		MergeRenderStatistic( newStat.renderer, INOUT this->renderer );
		MergeResourceStatistic( newStat.resources, INOUT this->resources );
	}


}	// FG

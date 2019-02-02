// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/FrameGraphThread.h"
#include "framegraph/Public/IPipelineCompiler.h"
#include "framegraph/Public/SubmissionGraph.h"

namespace FG
{

	//
	// Frame Graph Instance interface
	//

	class FrameGraphInstance : public std::enable_shared_from_this<FrameGraphInstance>
	{
	// types
	public:
		using DeviceInfo_t			= Union< NullUnion, VulkanDeviceInfo >;
		using ExternalCmdBatch_t	= Union< NullUnion, VulkanCommandBatch >;

		struct RenderingStatistics
		{
			uint		descriptorBinds				= 0;
			uint		pushConstants				= 0;
			uint		pipelineBarriers			= 0;
			uint		transferOps					= 0;

			uint		indexBufferBindings			= 0;
			uint		vertexBufferBindings		= 0;
			uint		drawCalls					= 0;
			uint		graphicsPipelineBindings	= 0;
			uint		dynamicStateChanges			= 0;

			uint		dispatchCalls				= 0;
			uint		computePipelineBindings		= 0;

			uint		rayTracingPipelineBindings	= 0;
			uint		traceRaysCalls				= 0;
			uint		buildASCalls				= 0;

			Nanoseconds	gpuTime						{0};	// for (currentFrame - ringBufferSize)
			Nanoseconds	cpuTime						{0};	// for (currentFrame - ringBufferSize)
		};

		struct ResourceStatistics
		{
			uint		newGraphicsPipelineCount	= 0;
			uint		newComputePipelineCount		= 0;
			uint		newRayTracingPipelineCount	= 0;
		};

		struct Statistics
		{
			RenderingStatistics		renderer;
			ResourceStatistics		resources;

			void Merge (const Statistics &);
		};
		
		ND_ static FGInstancePtr  CreateFrameGraph (const DeviceInfo_t &);


	// interface
	public:

			// Creates framegraph thread (worker).
			// Must be externally synchronized.
		ND_ virtual FGThreadPtr	CreateThread (const ThreadDesc &) = 0;


		// initialization //

			// Initialize instance systems.
			// Must be externally synchronized.
			virtual bool		Initialize (uint ringBufferSize) = 0;

			// Deinitialize instance systems.
			// All threads must be deinitialized.
			// Must be externally synchronized.
			virtual void		Deinitialize () = 0;
			
			// Add pipeline compiler for all framegraph threads.
			// Must be externally synchronized with all framegraph threads.
			virtual bool		AddPipelineCompiler (const IPipelineCompilerPtr &comp) = 0;
			
			// Set compilation flags for all framegraph threads.
			// Must be externally synchronized with all framegraph threads.
			virtual void		SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags = Default) = 0;


		// frame execution //

			// Enter to asynchronious mode.
			// Must be externally synchronized with all framegraph threads.
			virtual bool		BeginFrame (const SubmissionGraph &) = 0;
			
			// Complete frame execution and exit from asynchronious mode.
			// Must be externally synchronized with all framegraph threads.
			virtual bool		EndFrame () = 0;
			
			// Wait until all commands will complete their work on GPU, trigger events for 'ReadImage' and 'ReadBuffer' tasks.
			// Must be externally synchronized with all framegraph threads.
			virtual bool		WaitIdle () = 0;

			// Skip subbatch.
			// 'batchId' and 'indexInBatch' must be unique.
			// This method is thread safe and lock-free.
			virtual bool		SkipBatch (const CommandBatchID &batchId, uint indexInBatch) = 0;
			
			// Add external command buffer as subbatch.
			// 'batchId' and 'indexInBatch' must be unique.
			// This method is thread safe and lock-free except one call that submit commands to the GPU.
			virtual bool		SubmitBatch (const CommandBatchID &batchId, uint indexInBatch, const ExternalCmdBatch_t &data) = 0;


		// debugging //

			// Returns framegraph statistics.
			// Must be externally synchronized with all framegraph threads.
			virtual bool	GetStatistics (OUT Statistics &result) const = 0;

			// Returns serialized tasks, resource usage and barriers, can be used for regression testing.
			// Must be externally synchronized with all framegraph threads.
			virtual bool	DumpToString (OUT String &result) const = 0;

			// Returns graph written on dot language, can be used for graph visualization with graphviz.
			// Must be externally synchronized with all framegraph threads.
			virtual bool	DumpToGraphViz (OUT String &result) const = 0;
	};


}	// FG

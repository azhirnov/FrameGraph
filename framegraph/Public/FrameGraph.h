// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Required external synchronization for rendering:
	1.
		BeginFrame()
		cv.notify_all()		// wake up all threads
		for each thread
		{
			thread.Begin()
			...
			thread.Execute()
			barrier			// all threads must complete work before calling 'EndFrame()'
		}
		EndFrame()

	2.
		for each thread
		{
			// single-thread mode
			if (some thread id)
			{
				BeginFrame()
				thread.CreateResource()		// resource can be used in any thread
			}

			// multi-thread mode
			barrier
			thread.Begin()
			...
			if (another thread id)
				thread.CreateResource()		// resource can be used in any thread, but memory barrier required
			...
			thread.Execute()
			barrier
			
			// single-thread mode
			if (some thread id)
			{
				EndFrame()
				thread.ReleaseResource()
			}
			barrier
			...
		}
*/

#pragma once

#include "framegraph/Public/FrameGraphThread.h"
#include "framegraph/Public/IPipelineCompiler.h"
#include "framegraph/Public/SubmissionGraph.h"

namespace FG
{

	//
	// Frame Graph interface
	//

	class FrameGraph : public std::enable_shared_from_this<FrameGraph>
	{
	// types
	public:
		using DeviceInfo_t			= Union< std::monostate, VulkanDeviceInfo >;
		using ExternalCmdBatch_t	= Union< std::monostate, VulkanCommandBatch >;

		struct Statistics
		{
		};
		
		ND_ static FrameGraphPtr  CreateFrameGraph (const DeviceInfo_t &);


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

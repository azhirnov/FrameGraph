// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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
		using DeviceInfo_t		= Union< std::monostate, VulkanDeviceInfo >;

		struct Statistics
		{
		};
		
		ND_ static FrameGraphPtr  CreateFrameGraph (const DeviceInfo_t &);


	// interface
	public:
			virtual ~FrameGraph () {}

		ND_ virtual FGThreadPtr	CreateThread (const ThreadDesc &) = 0;

		// initialization
			virtual bool		Initialize (uint ringBufferSize) = 0;
			virtual void		Deinitialize () = 0;
			virtual bool		AddPipelineCompiler (const IPipelineCompilerPtr &comp) = 0;
			virtual void		SetCompilationFlags (ECompilationFlags flags, ECompilationDebugFlags debugFlags = Default) = 0;

		// frame execution
			virtual bool		Begin (const SubmissionGraph &) = 0;
			virtual bool		SkipSubBatch (const CommandBatchID &batchId, uint indexInBatch) = 0;
			virtual bool		Execute () = 0;
			virtual bool		WaitIdle () = 0;

		// debugging
		ND_ virtual Statistics const&	GetStatistics () const = 0;

			// can be used for regression testing
			virtual bool				DumpToString (OUT String &result) const = 0;

			// for graph visualization
			virtual bool				DumpToGraphViz (EGraphVizFlags flags, OUT String &result) const = 0;
	};


}	// FG

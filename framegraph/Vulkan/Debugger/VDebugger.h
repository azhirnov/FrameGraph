// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"
#include "framegraph/Public/SubmissionGraph.h"

namespace FG
{

	//
	// Debugger
	//

	class VDebugger
	{
	// types
	public:
		struct SubBatchGraph
		{
			String		body;
			String		firstNode;
			String		lastNode;
		};

	private:
		static constexpr uint	MaxSubBatches	= 32;
		using SubBatchName_t	= StaticString< 32 >;

		struct SubBatch
		{
			mutable String			dump;
			mutable SubBatchGraph	graph;
			RaceConditionCheck		rcCheck;
			SubBatchName_t			name;
		};

		struct Batch;
		using Dependencies_t	= FixedArray< const Batch*, FG_MaxCommandBatchDependencies >;
		using SubBatches_t		= StaticArray< SubBatch, MaxSubBatches >;

		struct Batch
		{
			SubBatches_t			subBatches;
			uint					threadCount	= 0;
			Dependencies_t			input;
			Dependencies_t			output;

			Batch () {}
			Batch (Batch &&) {}
		};
		
		using Batches_t = FixedMap< CommandBatchID, Batch, FG_MaxCommandBatchCount >;


	// variables
	private:
		Batches_t	_batches;


	// methods
	public:
		VDebugger ();

		void OnBeginFrame (const SubmissionGraph &);
		void OnEndFrame ();

		void GetSubBatchInfo (const CommandBatchID &batchId, uint indexInBatch,
							  OUT StringView &uid, OUT String* &dump, OUT SubBatchGraph* &graph) const;

		void GetFrameDump (OUT String &) const;
		void GetGraphDump (OUT String &) const;

		ND_ static String  BuildSubBatchName (const CommandBatchID &batchId, uint indexInBatch);
	};


}	// FG

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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
	private:
		static constexpr uint	MaxSubBatches	= 32;

		struct SubBatch
		{
			String				dump;
			String				graph;
			RaceConditionCheck	rcCheck;
		};

		struct Batch;
		using Dependencies_t	= FixedArray< const Batch*, FG_MaxCommandBatchDependencies >;
		using SubBatches_t		= StaticArray< SubBatch, MaxSubBatches >;

		struct Batch
		{
			mutable SubBatches_t	subBatches;
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

		void AddFrameDump (const CommandBatchID &batchId, uint indexInBatch, INOUT String &str) const;
		void AddGraphDump (const CommandBatchID &batchId, uint indexInBatch, INOUT String &str) const;

		void GetFrameDump (OUT String &) const;
		void GetGraphDump (OUT String &) const;

		ND_ static String  BuildSubBatchName (const CommandBatchID &batchId, uint indexInBatch);
	};


}	// FG

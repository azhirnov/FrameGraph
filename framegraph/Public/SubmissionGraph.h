// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'
/*
  cb      - command buffer
  signal  - semaphore signaling operation
  wait    - semaphore waiting operation
  thread* - CPU thread


  .-----------------------------.
  |          cmd batch 1        |--------.
  |-----------------------------| signal |
  | thread0 | thread1 | thread2 |--------|---------------------------------------.
  |---------|---------|---------|  wait  |              cmd batch 2              |--------.
  | cb | cb |   cb    |   cb    |--------|---------------------------------------| signal |
  |---------'---------'---------'        | thread3 | thread4 | thread5 | thread6 |--------|----------------.
  |                                      |---------|---------|---------|---------|  wait  |   cmd batch 3  |--------------.
  |                                      |    cb   | cb | cb | cb | cb |   cb    |--------|----------------| fence signal |
  |                                      '---------'---------'---------'---------|        |     thread7    |--------------'            .-------------.
  |                                                                              |        |----------------|    '--------------------> | wait fences |
  |                                                                              |        |       cb       |      some frames later    '-------------'
  |------------------------------------------------------------------------------|        |----------------|
  |                      graphics, compute, present queue                        |        | compute queue  |
  '------------------------------------------------------------------------------'        '----------------'


  Each batch may be sabmitted independently from any CPU thread.
  The recommended number of batches is 3-5, it can be used for optimal GPU workload.
*/

#pragma once

#include "framegraph/Public/FGEnums.h"
#include "framegraph/Public/IDs.h"

namespace FG
{

	//
	// Submission Graph
	//

	struct SubmissionGraph
	{
	// types
	private:
		using Dependencies_t = FixedArray< CommandBatchID, FG_MaxCommandBatchDependencies >;

		struct Batch
		{
			friend struct SubmissionGraph;

		private:
			mutable uint	_acquired	= 0;
		public:
			uint			threadCount	= 0;
			EThreadUsage	usage		= Default;
			Dependencies_t	dependsOn;

			Batch () {}
			Batch (uint threadCount, EThreadUsage usage, ArrayView<CommandBatchID> dependsOn) :
				threadCount{threadCount}, usage{usage}, dependsOn{dependsOn} {}
		};

		using Batches_t = FixedMap< CommandBatchID, Batch, FG_MaxCommandBatchCount >;


	// variables
	private:
		Batches_t		_batches;


	// methods
	public:
		SubmissionGraph ()
		{}


		SubmissionGraph&  AddBatch (const CommandBatchID &batchId,
									uint threadCount = 1,
									EThreadUsage usage = EThreadUsage::Graphics,
									ArrayView<CommandBatchID> dependsOn = Default)
		{
			DEBUG_ONLY(
			for (auto& dep : dependsOn) {
				ASSERT( _batches.count( dep ) );
			})

			ASSERT( not batchId.GetName().empty() );
			ASSERT( !!(usage & EThreadUsage::_QueueMask) );

			CHECK( _batches.insert_or_assign( batchId, Batch{threadCount, usage, dependsOn} ).second );
			return *this;
		}


		// helper function to acquire batch for thread
		bool Acquire (INOUT EThreadUsage &usage, OUT CommandBatchID &batchId, OUT uint &index) const
		{
			for (auto& batch : _batches) {
				if ( batch.second._acquired < batch.second.threadCount and EnumEq( usage, batch.second.usage )) {
					usage	= batch.second.usage;
					batchId	= batch.first;
					index	= batch.second._acquired++;
					return true;
				}
			}
			return false;
		}


		void Reset () const
		{
			for (auto& batch : _batches) {
				batch.second._acquired = 0;
			}
		}


		void Clear ()
		{
			_batches.clear();
		}


		ND_ Batches_t const&	Batches ()	const	{ return _batches; }
	};


}	// FG

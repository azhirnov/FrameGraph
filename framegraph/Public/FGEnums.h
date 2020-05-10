// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	enum class ESwapchainImage : uint
	{
		Primary,

		// for VR:
		LeftEye,
		RightEye,
	};
	

	enum class EDebugFlags : uint
	{
		LogTasks						= 1 << 0,	// 
		LogBarriers						= 1 << 1,	//
		LogResourceUsage				= 1 << 2,	// 

		VisTasks						= 1 << 10,
		VisDrawTasks					= 1 << 11,
		VisResources					= 1 << 12,
		VisBarriers						= 1 << 13,
		VisBarrierLabels				= 1 << 14,
		VisTaskDependencies				= 1 << 15,

		FullBarrier						= 1u << 30,	// use global memory barrier addtionally to per-resource barriers
		QueueSync						= 1u << 31,	// after each submit wait until queue complete execution

		/*LogUnreleasedResources			= 1 << 3,	// 
		
		CheckNonOptimalLayouts			= 1 << 16,	// if used 'General' layout instead optimal layout.
		CheckWritingToImmutable			= 1 << 17,	// immutable resource supports only read access, immutable images must be in 'General' layout.

		CheckDiscardingResult			= 1 << 18,	// performance/logic warning, triggered if you write to image/buffer in one task,
													// then write to same region in next task that discarding previous result,
													// used for pattern: ReadWrite/WriteOnly -> WriteDiscard.
		CheckPossibleDiscardingResult	= 1 << 19,	// same as 'CheckDiscardingResult', used for pattern: ReadWrite -> WriteOnly.
		
		SuppressWarnings				= 1 << 30,	// debugger may generate warning messages in log, use this flag to disable warnings*/
		Unknown							= 0,

		Default		= LogTasks | LogBarriers | LogResourceUsage |
					  VisTasks | VisDrawTasks | VisResources | VisBarriers,
	};
	FG_BIT_OPERATORS( EDebugFlags );


}	// FG

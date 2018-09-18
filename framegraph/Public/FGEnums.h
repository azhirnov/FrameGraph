// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/LowLevel/Types.h"

namespace FG
{

	enum class EGraphVizFlags : uint
	{
		/*Tasks					= 1 << 0,
		DrawTasks				= 1 << 1,

		Barriers				= 1 << 2,
		BarrierLabels			= 1 << 3,

		ResourceState			= 1 << 4,
		LogicalResources		= 1 << 5,
		RenderTargets			= 1 << 6,
		Textures				= 1 << 7,
		ReadOnlyImages			= 1 << 8,
		WritebleImages			= 1 << 9,
		DrawBuffers				= 1 << 10,	// vertex, index, indirect
		UniformBuffers			= 1 << 11,
		ReadOnlyStorageBuffers	= 1 << 12,
		WritableStoregeBuffers	= 1 << 13,
		_Last,

		All						= ((_Last-1) << 1) - 1,
		AllImages				= RenderTargets | Textures | ReadOnlyImages | WritebleImages,
		AllBuffers				= DrawBuffers | UniformBuffers | ReadOnlyStorageBuffers | WritableStoregeBuffers,
		AllResources			= AllImages | AllBuffers,*/
	};
	FG_BIT_OPERATORS( EGraphVizFlags );
	

	enum class ECompilationDebugFlags : uint
	{
		LogTasks						= 1 << 0,	// 
		LogBarriers						= 1 << 1,	//
		LogResources					= 1 << 2,	// 
		LogUnreleasedResources			= 1 << 3,	// 
		
		CheckNonOptimalLayouts			= 1 << 16,	// if used 'General' layout instead optimal layout.
		CheckWritingToImmutable			= 1 << 17,	// immutable resource supports only read access, immutable images must be in 'General' layout.

		CheckDiscardingResult			= 1 << 18,	// performance/logic warning, triggered if you write to image/buffer in one task,
													// then write to same region in next task that discarding previous result,
													// used for pattern: ReadWrite/WriteOnly -> WriteDiscard.
		CheckPossibleDiscardingResult	= 1 << 19,	// same as 'CheckDiscardingResult', used for pattern: ReadWrite -> WriteOnly.
		
		SuppressWarnings				= 1 << 30,	// debugger may generate warning messages in log, use this flag to disable warnings
		Unknown							= 0,
	};
	FG_BIT_OPERATORS( ECompilationDebugFlags );


	enum class ECompilationFlags : uint
	{
		// debugging
		EnableDebugger			= 1 << 0,	// setup debugger with 'ECompilationDebugFlags'
		InsertExecutionBarriers	= 1 << 1,	// use execution barriers instead of resource barriers
		EnableStatistic			= 1 << 2,	//  ...
		EnableDetailStatistic	= 1 << 3,	// use pipeline statistic and other...

		// optimizations
		OptimizeLayouts			= 1 << 16,	// change image layout to optimal
		OptimizeRenderPass		= 1 << 17,	// merge render passes
		ParallelizeRendering	= 1 << 18,	// may create additional image/buffers to skip some barriers
		MinimizeMemoryUsage		= 1 << 19,	// may disable other optimizations

		_Last,
		
		OptimizeAll				= OptimizeLayouts | OptimizeRenderPass | ParallelizeRendering,
		Unknown					= 0,
	};
	FG_BIT_OPERATORS( ECompilationFlags );


}	// FG

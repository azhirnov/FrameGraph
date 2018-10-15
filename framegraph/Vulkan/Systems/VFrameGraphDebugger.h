// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"
#include "Public/FGEnums.h"
#include "VImage.h"
#include "VBuffer.h"

namespace FG
{

	//
	// Vulkan Frame Graph Debugger
	//

	class VFrameGraphDebugger
	{
	// types
	private:
		using TaskPtr	= class IFrameGraphTask const*;

		template <typename BarrierType>
		struct Barrier
		{
			ExeOrderIndex				srcIndex		= ExeOrderIndex::Initial;
			ExeOrderIndex				dstIndex		= ExeOrderIndex::Initial;
			VkPipelineStageFlags		srcStageMask	= 0;
			VkPipelineStageFlags		dstStageMask	= 0;
			VkDependencyFlags			dependencyFlags	= 0;
			BarrierType					barrier			= {};
		};
		
		template <typename BarrierType>
		struct ResourceInfo
		{
			Array< Barrier<BarrierType>>	barriers;
		};

		using ImageInfo_t		= ResourceInfo< VkImageMemoryBarrier >;
		using BufferInfo_t		= ResourceInfo< VkBufferMemoryBarrier >;

		using ImageUsage_t		= Pair< VImagePtr,  VImage::ImageState   >;
		using BufferUsage_t		= Pair< VBufferPtr, VBuffer::BufferState >;
		using ResourceUsage_t	= Union< std::monostate, ImageUsage_t, BufferUsage_t >;

		using ImageResources_t	= HashMap< VImagePtr, ImageInfo_t >;
		using BufferResources_t	= HashMap< VBufferPtr, BufferInfo_t >;

		struct TaskInfo
		{
			TaskPtr					task	= null;
			Array<ResourceUsage_t>	resources;

			explicit TaskInfo (TaskPtr task) : task{task} {}
		};

		using TaskMap_t			= HashMap< ExeOrderIndex, TaskInfo >;
		using TaskPerQueue_t	= HashMap< VCommandQueue const*, TaskMap_t >;


	// variables
	private:
		TaskPerQueue_t			_tasksPerQueue;
		ImageResources_t		_images;
		BufferResources_t		_buffers;
		ExeOrderIndex			_minOrderIndex;

		// settings
		ECompilationDebugFlags	_flags;


	// methods
	public:
		VFrameGraphDebugger ();

		void Setup (ECompilationDebugFlags flags);
		void DumpFrame (OUT String &str) const;
		void Clear ();
		
		void AddBufferBarrier (VBuffer *					buffer,
							   ExeOrderIndex				srcIndex,
							   ExeOrderIndex				dstIndex,
							   VkPipelineStageFlags			srcStageMask,
							   VkPipelineStageFlags			dstStageMask,
							   VkDependencyFlags			dependencyFlags,
							   const VkBufferMemoryBarrier	&barrier);

		
		void AddImageBarrier (VImage *						image,
							  ExeOrderIndex					srcIndex,
							  ExeOrderIndex					dstIndex,
							  VkPipelineStageFlags			srcStageMask,
							  VkPipelineStageFlags			dstStageMask,
							  VkDependencyFlags				dependencyFlags,
							  const VkImageMemoryBarrier	&barrier);

		void AddBufferUsage (VBuffer *buffer, const VBuffer::BufferState &state);
		void AddImageUsage (VImage *image, const VImage::ImageState &state);

		void RunTask (VCommandQueue const* queue, TaskPtr task);


	private:
		void _DumpImages (OUT String &str) const;
		void _DumpImage (const VImagePtr &image, const ImageInfo_t &info, OUT String &str) const;

		void _DumpBuffers (OUT String &str) const;
		void _DumpBuffer (const VBufferPtr &buffer, const BufferInfo_t &info, OUT String &str) const;

		void _DumpQueue (VCommandQueue const* queue, const TaskMap_t &tasks, OUT String &str) const;
		void _DumpResourceUsage (ArrayView<ResourceUsage_t> resources, OUT String &str) const;

		ND_ String _GetTaskName (ExeOrderIndex idx) const;
	};

}	// FG

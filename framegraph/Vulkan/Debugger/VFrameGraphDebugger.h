// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"
#include "Public/FGEnums.h"
#include "VLocalImage.h"
#include "VLocalBuffer.h"

namespace FG
{

	//
	// Vulkan Frame Graph Debugger
	//

	class VFrameGraphDebugger final
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
			BarrierType					info			= {};
		};
		
		template <typename BarrierType>
		struct ResourceInfo
		{
			Array< Barrier<BarrierType>>	barriers;
		};

		using ImageInfo_t		= ResourceInfo< VkImageMemoryBarrier >;
		using BufferInfo_t		= ResourceInfo< VkBufferMemoryBarrier >;

		using ImageUsage_t		= Pair< const VImage *,  VLocalImage::ImageState   >;
		using BufferUsage_t		= Pair< const VBuffer *, VLocalBuffer::BufferState >;
		using ResourceUsage_t	= Union< std::monostate, ImageUsage_t, BufferUsage_t >;

		using ImageResources_t	= HashMap< const VImage *, ImageInfo_t >;
		using BufferResources_t	= HashMap< const VBuffer *, BufferInfo_t >;

		struct TaskInfo
		{
			TaskPtr					task	= null;
			Array<ResourceUsage_t>	resources;

			TaskInfo () {}
			explicit TaskInfo (TaskPtr task) : task{task} {}
		};

		using TaskMap_t	= Array< TaskInfo >;
		
		struct NodeUniqueNames;
		struct ColorScheme;


	// variables
	private:
		TaskMap_t				_tasks;
		ImageResources_t		_images;
		BufferResources_t		_buffers;

		VDebugger const&		_mainDbg;
		StringView				_subBatchUID;

		// settings
		ECompilationDebugFlags	_flags;


	// methods
	public:
		explicit VFrameGraphDebugger (const VDebugger &);

		void Setup (ECompilationDebugFlags flags);

		void OnBeginFrame ();
		void OnEndFrame (const CommandBatchID &batchId, uint indexInBatch);
		
		void AddBufferBarrier (const VBuffer *				buffer,
							   ExeOrderIndex				srcIndex,
							   ExeOrderIndex				dstIndex,
							   VkPipelineStageFlags			srcStageMask,
							   VkPipelineStageFlags			dstStageMask,
							   VkDependencyFlags			dependencyFlags,
							   const VkBufferMemoryBarrier	&barrier);

		void AddImageBarrier (const VImage *				image,
							  ExeOrderIndex					srcIndex,
							  ExeOrderIndex					dstIndex,
							  VkPipelineStageFlags			srcStageMask,
							  VkPipelineStageFlags			dstStageMask,
							  VkDependencyFlags				dependencyFlags,
							  const VkImageMemoryBarrier	&barrier);

		void AddBufferUsage (const VBuffer* bufferId, const VLocalBuffer::BufferState &state);
		void AddImageUsage (const VImage* imageId, const VLocalImage::ImageState &state);

		void RunTask (TaskPtr task);


	// dump to string
	private:
		void _DumpFrame (const CommandBatchID &batchId, uint indexInBatch, OUT String &str) const;

		void _DumpImages (INOUT String &str) const;
		void _DumpImageInfo (const VImage *image, const ImageInfo_t &info, INOUT String &str) const;

		void _DumpBuffers (INOUT String &str) const;
		void _DumpBufferInfo (const VBuffer *buffer, const BufferInfo_t &info, INOUT String &str) const;

		void _DumpQueue (const TaskMap_t &tasks, INOUT String &str) const;
		void _DumpResourceUsage (ArrayView<ResourceUsage_t> resources, INOUT String &str) const;


	// dump to graphviz format
	private:
		void _DumpGraph (const CommandBatchID &batchId, uint indexInBatch, OUT String &str) const;

		void _GetResourceUsage (ArrayView<ResourceUsage_t> resources, OUT String &resStyle,
								INOUT String &style, INOUT String &deps, INOUT HashSet<String> &existingBarriers) const;

		void _GetBufferBarrier (const VBuffer *buffer, TaskPtr task, INOUT String &style, INOUT String &deps,
								INOUT HashSet<String> &existingBarriers) const;

		void _GetImageBarrier (const VImage *image, TaskPtr task, INOUT String &style, INOUT String &deps,
							   INOUT HashSet<String> &existingBarriers) const;

		template <typename T, typename B>
		void _GetResourceBarrier (const T *res, const Barrier<B> &bar, VkImageLayout oldLayout, VkImageLayout newLayout,
								  INOUT String &style, INOUT String &deps, INOUT HashSet<String> &existingBarriers) const;


	// utils
	private:
		ND_ String  _GetTaskName (ExeOrderIndex idx) const;
		ND_ String  _GetTaskName (TaskPtr task) const;
		ND_ TaskPtr  _GetTask (ExeOrderIndex idx) const;
	};


}	// FG

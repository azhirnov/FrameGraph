// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"
#include "Public/FGEnums.h"
#include "VLocalImage.h"
#include "VLocalBuffer.h"
#include "VLocalRTGeometry.h"
#include "VLocalRTScene.h"
#include "VTaskGraph.h"
#include "VDebugger.h"

namespace FG
{

	//
	// Vulkan Frame Graph Debugger
	//

	class VFrameGraphDebugger final
	{
	// types
	private:
		using TaskPtr	= Ptr<const IFrameGraphTask>;

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

		using ImageInfo_t			= ResourceInfo< VkImageMemoryBarrier >;
		using BufferInfo_t			= ResourceInfo< VkBufferMemoryBarrier >;
		using RTSceneInfo_t			= ResourceInfo< VkMemoryBarrier >;
		using RTGeometryInfo_t		= ResourceInfo< VkMemoryBarrier >;

		using ImageUsage_t			= Pair< const VImage *, VLocalImage::ImageState >;
		using BufferUsage_t			= Pair< const VBuffer *, VLocalBuffer::BufferState >;
		using RTSceneUsage_t		= Pair< const VRayTracingScene *, VLocalRTScene::SceneState >;
		using RTGeometryUsage_t		= Pair< const VRayTracingGeometry *, VLocalRTGeometry::GeometryState >;
		using ResourceUsage_t		= Union< NullUnion, ImageUsage_t, BufferUsage_t, RTSceneUsage_t, RTGeometryUsage_t >;

		using ImageResources_t		= HashMap< const VImage *, ImageInfo_t >;
		using BufferResources_t		= HashMap< const VBuffer *, BufferInfo_t >;
		using RTSceneResources_t	= HashMap< const VRayTracingScene *, RTSceneInfo_t >;
		using RTGeometryResources_t	= HashMap< const VRayTracingGeometry *, RTGeometryInfo_t >;

		struct TaskInfo
		{
			TaskPtr					task		= null;
			Array<ResourceUsage_t>	resources;
			mutable String			anyNode;

			TaskInfo () {}
			explicit TaskInfo (TaskPtr task) : task{task} {}
		};

		using TaskMap_t	= Array< TaskInfo >;


	// variables
	private:
		TaskMap_t					_tasks;
		ImageResources_t			_images;
		BufferResources_t			_buffers;
		RTSceneResources_t			_rtScenes;			// top-level AS
		RTGeometryResources_t		_rtGeometries;		// bottom-level AS
		mutable HashSet<String>		_existingNodes;

		VDebugger const&			_mainDbg;
		VFrameGraphThread const&	_frameGraph;
		StringView					_subBatchUID;

		// settings
		ECompilationDebugFlags		_flags;


	// methods
	public:
		explicit VFrameGraphDebugger (const VDebugger &, const VFrameGraphThread &);

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
		
		void AddRayTracingBarrier (const VRayTracingGeometry*	rtGeometry,
								   ExeOrderIndex				srcIndex,
								   ExeOrderIndex				dstIndex,
								   VkPipelineStageFlags			srcStageMask,
								   VkPipelineStageFlags			dstStageMask,
								   VkDependencyFlags			dependencyFlags,
								   const VkMemoryBarrier		&barrier);
		
		void AddRayTracingBarrier (const VRayTracingScene*		rtScene,
								   ExeOrderIndex				srcIndex,
								   ExeOrderIndex				dstIndex,
								   VkPipelineStageFlags			srcStageMask,
								   VkPipelineStageFlags			dstStageMask,
								   VkDependencyFlags			dependencyFlags,
								   const VkMemoryBarrier		&barrier);

		void AddHostWriteAccess (const VBuffer *buffer, BytesU offset, BytesU size);
		void AddHostReadAccess (const VBuffer *buffer, BytesU offset, BytesU size);

		void AddBufferUsage (const VBuffer* bufferId, const VLocalBuffer::BufferState &state);
		void AddImageUsage (const VImage* imageId, const VLocalImage::ImageState &state);
		void AddRTGeometryUsage (const VRayTracingGeometry *, const VLocalRTGeometry::GeometryState &state);
		void AddRTSceneUsage (const VRayTracingScene *, const VLocalRTScene::SceneState &state);

		void AddTask (TaskPtr task);


	// dump to string
	private:
		void _DumpFrame (const CommandBatchID &batchId, uint indexInBatch, OUT String &str) const;

		void _DumpImages (INOUT String &str) const;
		void _DumpImageInfo (const VImage *image, const ImageInfo_t &info, INOUT String &str) const;

		void _DumpBuffers (INOUT String &str) const;
		void _DumpBufferInfo (const VBuffer *buffer, const BufferInfo_t &info, INOUT String &str) const;

		void _DumpQueue (const TaskMap_t &tasks, INOUT String &str) const;
		void _DumpResourceUsage (ArrayView<ResourceUsage_t> resources, INOUT String &str) const;
		void _DumpTaskData (TaskPtr task, INOUT String &str) const;
		
		void _SubmitRenderPassTaskToString (Ptr<const VFgTask<SubmitRenderPass>>, INOUT String &) const;
		void _DispatchComputeTaskToString (Ptr<const VFgTask<DispatchCompute>>, INOUT String &) const;
		void _DispatchComputeIndirectTaskToString (Ptr<const VFgTask<DispatchComputeIndirect>>, INOUT String &) const;
		void _CopyBufferTaskToString (Ptr<const VFgTask<CopyBuffer>>, INOUT String &) const;
		void _CopyImageTaskToString (Ptr<const VFgTask<CopyImage>>, INOUT String &) const;
		void _CopyBufferToImageTaskToString (Ptr<const VFgTask<CopyBufferToImage>>, INOUT String &) const;
		void _CopyImageToBufferTaskToString (Ptr<const VFgTask<CopyImageToBuffer>>, INOUT String &) const;
		void _BlitImageTaskToString (Ptr<const VFgTask<BlitImage>>, INOUT String &) const;
		void _ResolveImageTaskToString (Ptr<const VFgTask<ResolveImage>>, INOUT String &) const;
		void _FillBufferTaskToString (Ptr<const VFgTask<FillBuffer>>, INOUT String &) const;
		void _ClearColorImageTaskToString (Ptr<const VFgTask<ClearColorImage>>, INOUT String &) const;
		void _ClearDepthStencilImageTaskToString (Ptr<const VFgTask<ClearDepthStencilImage>>, INOUT String &) const;
		void _UpdateBufferTaskToString (Ptr<const VFgTask<UpdateBuffer>>, INOUT String &) const;
		void _PresentTaskToString (Ptr<const VFgTask<Present>>, INOUT String &) const;
		void _PresentVRTaskToString (Ptr<const VFgTask<PresentVR>>, INOUT String &) const;
		void _BuildRayTracingGeometryTaskToString (Ptr<const VFgTask<BuildRayTracingGeometry>>, INOUT String &) const;
		void _BuildRayTracingSceneTaskToString (Ptr<const VFgTask<BuildRayTracingScene>>, INOUT String &) const;
		void _UpdateRayTracingShaderTableTaskToString (Ptr<const VFgTask<UpdateRayTracingShaderTable>>, INOUT String &) const;
		void _TraceRaysTaskToString (Ptr<const VFgTask<TraceRays>>, INOUT String &) const;


	// dump to graphviz format
	private:
		void _DumpGraph (const CommandBatchID &batchId, uint indexInBatch, OUT VDebugger::SubBatchGraph &str) const;
		void _AddInitialStates (INOUT String &str, OUT String &firstNode) const;
		void _AddFinalStates (INOUT String &str, INOUT String &deps, OUT String &lastNode) const;

		void _GetResourceUsage (const TaskInfo &info, OUT String &resStyle, OUT String &barStyle, INOUT String &deps) const;

		void _GetBufferBarrier (const VBuffer *buffer, TaskPtr task, INOUT String &barStyle, INOUT String &deps) const;
		void _GetImageBarrier (const VImage *image, TaskPtr task, INOUT String &barStyle, INOUT String &deps) const;

		template <typename T, typename B>
		void _GetResourceBarrier (const T *res, const Barrier<B> &bar, VkImageLayout oldLayout, VkImageLayout newLayout,
								  INOUT String &style, INOUT String &deps) const;

		// graphviz node names
		ND_ String  _VisTaskName (TaskPtr task) const;
		ND_ String  _VisBarrierGroupName (TaskPtr task) const;
		ND_ String  _VisBarrierGroupName (ExeOrderIndex index) const;
		ND_ String  _VisDrawTaskName (TaskPtr task) const;
		ND_ String  _VisResourceName (const VBuffer *buffer, TaskPtr task) const;
		ND_ String  _VisResourceName (const VBuffer *buffer, ExeOrderIndex index) const;
		ND_ String  _VisResourceName (const VImage *image, TaskPtr task) const;
		ND_ String  _VisResourceName (const VImage *image, ExeOrderIndex index) const;
		ND_ String  _VisBarrierName (const VBuffer *buffer, ExeOrderIndex srcIndex, ExeOrderIndex dstIndex) const;
		ND_ String  _VisBarrierName (const VImage *image, ExeOrderIndex srcIndex, ExeOrderIndex dstIndex) const;

		// color style
		ND_ String  _SubBatchBG () const;
		ND_ String  _TaskLabelColor (RGBA8u) const;
		ND_ String  _DrawTaskBG () const;
		ND_ String  _DrawTaskLabelColor () const;
		ND_ String  _ResourceBG (const VBuffer *) const;
		ND_ String  _ResourceBG (const VImage *) const;
		ND_ String  _ResourceToResourceEdgeColor (TaskPtr task) const;
		ND_ String  _ResourceGroupBG (TaskPtr task) const;
		ND_ String  _BarrierGroupBorderColor () const;
		ND_ String  _GroupBorderColor () const;


	// utils
	private:
		ND_ String  _GetTaskName (ExeOrderIndex idx) const;
		ND_ String  _GetTaskName (TaskPtr task) const;
		ND_ String  _GetTaskName (Task task) const			{ return _GetTaskName( TaskPtr(task) ); }

		ND_ TaskPtr  _GetTask (ExeOrderIndex idx) const;
	};


}	// FG

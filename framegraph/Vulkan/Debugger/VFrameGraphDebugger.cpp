// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraphDebugger.h"
#include "VFrameGraphThread.h"
#include "VEnumToString.h"
#include "Shared/EnumToString.h"
#include "VDebugger.h"

namespace FG
{
namespace {
	static constexpr char	indent[] = "\t";
}

/*
=================================================
	constructor
=================================================
*/
	VFrameGraphDebugger::VFrameGraphDebugger (const VDebugger &dbg, const VFrameGraphThread &fg) :
		_mainDbg{ dbg },		_frameGraph{ fg },
		_flags{ Default }
	{
		_tasks.reserve( 256 );
	}
	
/*
=================================================
	Setup
=================================================
*/
	void VFrameGraphDebugger::Setup (ECompilationDebugFlags flags)
	{
		_flags = flags;
	}

/*
=================================================
	OnBeginFrame
=================================================
*/
	void VFrameGraphDebugger::OnBeginFrame ()
	{
	}
	
/*
=================================================
	OnEndFrame
=================================================
*/
	void VFrameGraphDebugger::OnEndFrame (const CommandBatchID &batchId, uint indexInBatch)
	{
		constexpr auto	DumpFlags =	ECompilationDebugFlags::LogTasks		|
									ECompilationDebugFlags::LogBarriers		|
									ECompilationDebugFlags::LogResourceUsage;
		
		if ( EnumEq( _flags, DumpFlags ) )
		{
			String *	dump	= null;
			String *	graph	= null;

			_mainDbg.GetSubBatchInfo( batchId, indexInBatch, OUT _subBatchUID, OUT dump, OUT graph );

			_DumpFrame( batchId, indexInBatch, OUT *dump );
			_DumpGraph( batchId, indexInBatch, OUT *graph );
		}

		_subBatchUID = Default;
		_tasks.clear();
		_images.clear();
		_buffers.clear();
	}

/*
=================================================
	RunTask
=================================================
*/
	void VFrameGraphDebugger::RunTask (TaskPtr task)
	{
		if ( not EnumEq( _flags, ECompilationDebugFlags::LogTasks ) )
			return;

		const size_t	idx = size_t(task->ExecutionOrder());

		_tasks.resize( idx+1 );

		ASSERT( _tasks[idx].task == null );
		_tasks[idx] = TaskInfo{task};
	}

/*
=================================================
	AddBufferBarrier
=================================================
*/	
	void VFrameGraphDebugger::AddBufferBarrier (const VBuffer *				buffer,
												ExeOrderIndex				srcIndex,
												ExeOrderIndex				dstIndex,
												VkPipelineStageFlags		srcStageMask,
												VkPipelineStageFlags		dstStageMask,
												VkDependencyFlags			dependencyFlags,
												const VkBufferMemoryBarrier	&barrier)
	{
		if ( not EnumEq( _flags, ECompilationDebugFlags::LogBarriers ) )
			return;

		auto&	barriers = _buffers.insert({ buffer, {} }).first->second.barriers;

		barriers.push_back({ srcIndex, dstIndex, srcStageMask, dstStageMask, dependencyFlags, barrier });
	}
	
/*
=================================================
	AddImageBarrier
=================================================
*/
	void VFrameGraphDebugger::AddImageBarrier (const VImage *				image,
											   ExeOrderIndex				srcIndex,
											   ExeOrderIndex				dstIndex,
											   VkPipelineStageFlags			srcStageMask,
											   VkPipelineStageFlags			dstStageMask,
											   VkDependencyFlags			dependencyFlags,
											   const VkImageMemoryBarrier	&barrier)
	{
		if ( not EnumEq( _flags, ECompilationDebugFlags::LogBarriers ) )
			return;

		auto&	barriers = _images.insert({ image, {} }).first->second.barriers;

		barriers.push_back({ srcIndex, dstIndex, srcStageMask, dstStageMask, dependencyFlags, barrier });
	}
	
/*
=================================================
	AddBufferUsage
=================================================
*/
	void VFrameGraphDebugger::AddBufferUsage (const VBuffer* buffer, const VLocalBuffer::BufferState &state)
	{
		if ( not EnumEq( _flags, ECompilationDebugFlags::LogResourceUsage ) )
			return;
		
		ASSERT( buffer and state.task );
		const size_t	idx = size_t(state.task->ExecutionOrder());
		
		if ( idx >= _tasks.size() or _tasks[idx].task == null )
		{
			ASSERT( !"task doesn't exists!" );
			return;
		}

		_tasks[idx].resources.push_back( BufferUsage_t{ buffer, state });
	}
	
/*
=================================================
	AddImageUsage
=================================================
*/
	void VFrameGraphDebugger::AddImageUsage (const VImage* image, const VLocalImage::ImageState &state)
	{
		if ( not EnumEq( _flags, ECompilationDebugFlags::LogResourceUsage ) )
			return;

		ASSERT( image and state.task );
		const size_t	idx = size_t(state.task->ExecutionOrder());
		
		if ( idx >= _tasks.size() or _tasks[idx].task == null )
		{
			ASSERT( !"task doesn't exists!" );
			return;
		}
		
		_tasks[idx].resources.push_back(ImageUsage_t{ image, state });
	}

/*
=================================================
	_DumpFrame
----
	- sort resources by name (this needed for correct dump comparison in tests).
	- dump resources info & frame barriers.
=================================================
*/
	void VFrameGraphDebugger::_DumpFrame (const CommandBatchID &batchId, uint indexInBatch, OUT String &str) const
	{
		str.clear();
		str << "Thread {\n"
			<< "	batch:         \"" << batchId.GetName() << "\"\n"
			<< "	indexInBatch:  " << ToString(indexInBatch) << '\n';

		_DumpImages( INOUT str );
		_DumpBuffers( INOUT str );
		
		str << "	-----------------------------------------------------------\n";

		_DumpQueue( _tasks, INOUT str );

		str << "}\n"
			<< "===============================================================\n\n";
	}

/*
=================================================
	_DumpImages
=================================================
*/
	void VFrameGraphDebugger::_DumpImages (INOUT String &str) const
	{
		Array< ImageResources_t::value_type const* >	sorted;

		for (auto& img : _images) {
			sorted.push_back( &img );
		}

		std::sort( sorted.begin(), sorted.end(),
					[] (auto* lhs, auto* rhs)
					{
						if ( lhs->first->GetDebugName() != rhs->first->GetDebugName() )
							return lhs->first->GetDebugName() < rhs->first->GetDebugName();

						return lhs->first < rhs->first;
					});

		for (auto& img : sorted)
		{
			_DumpImageInfo( img->first, img->second, INOUT str );
		}
	}
	
/*
=================================================
	_DumpImageInfo
=================================================
*/
	void VFrameGraphDebugger::_DumpImageInfo (const VImage *image, const ImageInfo_t &info, INOUT String &str) const
	{
		str << indent << "Image {\n"
			<< indent << "	name:         \"" << image->GetDebugName() << "\"\n"
			<< indent << "	iamgeType:    " << ToString( image->Description().imageType ) << '\n'
			<< indent << "	dimension:    " << ToString( image->Description().dimension ) << '\n'
			<< indent << "	format:       " << ToString( image->Description().format ) << '\n'
			<< indent << "	usage:        " << ToString( image->Description().usage ) << '\n'
			<< indent << "	arrayLayers:  " << ToString( image->Description().arrayLayers.Get() ) << '\n'
			<< indent << "	maxLevel:     " << ToString( image->Description().maxLevel.Get() ) << '\n'
			<< indent << "	samples:      " << ToString( image->Description().samples.Get() ) << '\n';

		if ( not info.barriers.empty() )
		{
			str << indent << "	barriers = {\n";

			for (auto& bar : info.barriers)
			{
				str << indent << "\t\t	ImageMemoryBarrier {\n"
					<< indent << "\t\t		srcTask:         " << _GetTaskName( bar.srcIndex ) << '\n'
					<< indent << "\t\t		dstTask:         " << _GetTaskName( bar.dstIndex ) << '\n'
					<< indent << "\t\t		srcStageMask:    " << VkPipelineStage_ToString( bar.srcStageMask ) << '\n'
					<< indent << "\t\t		dstStageMask:    " << VkPipelineStage_ToString( bar.dstStageMask ) << '\n'
					<< indent << "\t\t		dependencyFlags: " << VkDependency_ToString( bar.dependencyFlags ) << '\n'
					<< indent << "\t\t		srcAccessMask:   " << VkAccess_ToString( bar.info.srcAccessMask ) << '\n'
					<< indent << "\t\t		dstAccessMask:   " << VkAccess_ToString( bar.info.dstAccessMask ) << '\n'
					//<< indent << "\t\t	srcQueueFamilyIndex"	// TODO: get debug name from queue
					//<< indent << "\t\t	dstQueueFamilyIndex"
					<< indent << "\t\t		oldLayout:       " << VkImageLayout_ToString( bar.info.oldLayout ) << '\n'
					<< indent << "\t\t		newLayout:       " << VkImageLayout_ToString( bar.info.newLayout ) << '\n'
					<< indent << "\t\t		aspectMask:      " << VkImageAspect_ToString( bar.info.subresourceRange.aspectMask ) << '\n'
					<< indent << "\t\t		baseMipLevel:    " << ToString( bar.info.subresourceRange.baseMipLevel ) << '\n'
					<< indent << "\t\t		levelCount:      " << ToString( bar.info.subresourceRange.levelCount ) << '\n'
					<< indent << "\t\t		baseArrayLayer:  " << ToString( bar.info.subresourceRange.baseArrayLayer ) << '\n'
					<< indent << "\t\t		layerCount:      " << ToString( bar.info.subresourceRange.layerCount ) << '\n'
					<< indent << "\t\t	}\n";
			}
			str << indent << "	}\n";
		}
		
		str << indent << "}\n\n";
	}
	
/*
=================================================
	_DumpBuffers
=================================================
*/
	void VFrameGraphDebugger::_DumpBuffers (INOUT String &str) const
	{
		Array< BufferResources_t::value_type const* >	sorted;

		for (auto& buf : _buffers) {
			sorted.push_back( &buf );
		}

		std::sort( sorted.begin(), sorted.end(),
					[](auto* lhs, auto* rhs)
					{
						if ( lhs->first->GetDebugName() != rhs->first->GetDebugName() )
							return lhs->first->GetDebugName() < rhs->first->GetDebugName();

						return lhs->first < rhs->first;
					});

		for (auto& buf : sorted)
		{
			_DumpBufferInfo( buf->first, buf->second, INOUT str );
		}
	}
	
/*
=================================================
	_DumpBufferInfo
=================================================
*/
	void VFrameGraphDebugger::_DumpBufferInfo (const VBuffer *buffer, const BufferInfo_t &info, INOUT String &str) const
	{
		str << indent << "Buffer {\n"
			<< indent << "	name:    \"" << buffer->GetDebugName() << "\"\n"
			<< indent << "	size:    " << ToString( buffer->Description().size ) << '\n'
			<< indent << "	usage:   " << ToString( buffer->Description().usage ) << '\n';
	
		if ( not info.barriers.empty() )
		{
			str << indent << "	barriers = {\n";

			for (auto& bar : info.barriers)
			{
				str << indent << "\t\t	BufferMemoryBarrier {\n"
					<< indent << "\t\t		srcTask:         " << _GetTaskName( bar.srcIndex ) << '\n'
					<< indent << "\t\t		dstTask:         " << _GetTaskName( bar.dstIndex ) << '\n'
					<< indent << "\t\t		srcStageMask:    " << VkPipelineStage_ToString( bar.srcStageMask ) << '\n'
					<< indent << "\t\t		dstStageMask:    " << VkPipelineStage_ToString( bar.dstStageMask ) << '\n'
					<< indent << "\t\t		dependencyFlags: " << VkDependency_ToString( bar.dependencyFlags ) << '\n'
					<< indent << "\t\t		srcAccessMask:   " << VkAccess_ToString( bar.info.srcAccessMask ) << '\n'
					<< indent << "\t\t		dstAccessMask:   " << VkAccess_ToString( bar.info.dstAccessMask ) << '\n'
					<< indent << "\t\t		offset:          " << ToString( BytesU(bar.info.offset) ) << '\n'
					<< indent << "\t\t		size:            " << ToString( BytesU(bar.info.size) ) << '\n'
					<< indent << "\t\t	}\n";
			}
			str << indent << "	}\n";
		}
		
		str << indent << "}\n\n";
	}
	
/*
=================================================
	_GetTask
=================================================
*/
	VFrameGraphDebugger::TaskPtr  VFrameGraphDebugger::_GetTask (ExeOrderIndex idx) const
	{
		return size_t(idx) < _tasks.size() ? _tasks[size_t(idx)].task : null;
	}

/*
=================================================
	_GetTaskName
=================================================
*/
	inline String  VFrameGraphDebugger::_GetTaskName (ExeOrderIndex idx) const
	{
		if ( idx == ExeOrderIndex::Initial )
			return "<initial>";

		if ( idx == ExeOrderIndex::Final )
			return "<final>";

		if ( size_t(idx) >= _tasks.size() or _tasks[size_t(idx)].task == null )
		{
			ASSERT(false);
			return "<unknown>";
		}

		return _GetTaskName( _tasks[size_t(idx)].task );
	}
	
/*
=================================================
	_GetTaskName
=================================================
*/
	inline String  VFrameGraphDebugger::_GetTaskName (TaskPtr task) const
	{
		return String(task->Name()) << " (#" << ToString( task->ExecutionOrder() ) << ')';
	}

/*
=================================================
	_DumpQueue
=================================================
*/
	void VFrameGraphDebugger::_DumpQueue (const TaskMap_t &tasks, INOUT String &str) const
	{
		for (auto& info : tasks)
		{
			if ( not info.task )
				continue;

			str << indent << "Task {\n"
				<< indent << "	name:    \"" << _GetTaskName( info.task ) << "\"\n"
				<< indent << "	input =  { ";

			for (auto& in : info.task->Inputs())
			{
				if ( &in != info.task->Inputs().begin() )
					str << ", ";

				str << _GetTaskName( in );
			}

			str << " }\n"
				<< indent << "	output = { ";

			for (auto& out : info.task->Outputs())
			{
				if ( &out != info.task->Outputs().begin() )
					str << ", ";

				str << _GetTaskName( out );
			}
			str << " }\n";

			_DumpResourceUsage( info.resources, INOUT str );

			_DumpTaskData( info.task, INOUT str );
			
			str << indent << "}\n";
		}
	}
	
/*
=================================================
	_DumpResourceUsage
=================================================
*/
	void VFrameGraphDebugger::_DumpResourceUsage (ArrayView<ResourceUsage_t> resources, INOUT String &str) const
	{
		struct ResInfo
		{
			StringView				name;
			ResourceUsage_t const*	usage	= null;
		};

		if ( resources.empty() )
			return;

		Array<ResInfo>	sorted;		sorted.reserve( resources.size() );

		// prepare for sorting
		for (auto& res : resources)
		{
			if ( auto* image = std::get_if<ImageUsage_t>( &res ) )
			{
				sorted.push_back({ image->first->GetDebugName(), &res });
			}
			else
			if ( auto* buffer = std::get_if<BufferUsage_t>( &res ) )
			{
				sorted.push_back({ buffer->first->GetDebugName(), &res });
			}
			else
			{
				ASSERT( !"unknown resource type!" );
			}
		}

		// sort
		std::sort( sorted.begin(), sorted.end(),
					[] (auto& lhs, auto& rhs) { return lhs.name != rhs.name ? lhs.name < rhs.name : lhs.usage < rhs.usage; } );

		// serialize
		str << indent << "\tresource_usage = {\n";

		for (auto& res : sorted)
		{
			if ( auto* image = std::get_if<ImageUsage_t>( res.usage ) )
			{
				str << indent << "\t	ImageUsage {\n"
					<< indent << "\t		name:           \"" << res.name << "\"\n"
					<< indent << "\t		usage:          " << ToString( image->second.state ) << '\n'
					<< indent << "\t		baseMipLevel:   " << ToString( image->second.range.Mipmaps().begin ) << '\n'
					<< indent << "\t		levelCount:     " << ToString( image->second.range.Mipmaps().Count() ) << '\n'
					<< indent << "\t		baseArrayLayer: " << ToString( image->second.range.Layers().begin ) << '\n'
					<< indent << "\t		layerCount:     " << ToString( image->second.range.Layers().Count() ) << '\n'
					<< indent << "\t	}\n";
			}
			else
			if ( auto* buffer = std::get_if<BufferUsage_t>( res.usage ) )
			{
				str << indent << "\t	BufferUsage {\n"
					<< indent << "\t		name:     \"" << res.name << "\"\n"
					<< indent << "\t		usage:    " << ToString( buffer->second.state ) << '\n'
					<< indent << "\t		offset:   " << ToString( BytesU(buffer->second.range.begin) ) << '\n'
					<< indent << "\t		size:     " << ToString( BytesU(buffer->second.range.Count()) ) << '\n'
					<< indent << "\t	}\n";
			}
		}

		str << indent << "\t}\n";
	}

/*
=================================================
	_SubmitRenderPassTaskToString
=================================================
*/
	void VFrameGraphDebugger::_SubmitRenderPassTaskToString (Ptr<const VFgTask<SubmitRenderPass>>, INOUT String &) const
	{
	}
	
/*
=================================================
	_DispatchComputeTaskToString
=================================================
*/
	void VFrameGraphDebugger::_DispatchComputeTaskToString (Ptr<const VFgTask<DispatchCompute>> task, INOUT String &str) const
	{
		str << indent << "	pipeline:    \"" << task->pipeline->GetDebugName() << "\"\n"
			<< indent << "	groupCount:  " << ToString( task->groupCount ) << '\n';

		if ( task->localGroupSize.has_value() )
			str << indent << "	localGroupSize:  " << ToString( *task->localGroupSize ) << '\n';
	}
	
/*
=================================================
	_DispatchComputeIndirectTaskToString
=================================================
*/
	void VFrameGraphDebugger::_DispatchComputeIndirectTaskToString (Ptr<const VFgTask<DispatchComputeIndirect>> task, INOUT String &str) const
	{
		str << indent << "	pipeline:       \"" << task->pipeline->GetDebugName() << "\"\n"
			<< indent << "	indirectBuffer: \"" << task->indirectBuffer->GetDebugName() << "\"\n"
			<< indent << "	indirectBufferOffset: " << ToString( BytesU{task->indirectBufferOffset} ) << '\n';
		
		if ( task->localGroupSize.has_value() )
			str << indent << "	localGroupSize:  " << ToString( *task->localGroupSize ) << '\n';
	}
	
/*
=================================================
	_CopyBufferTaskToString
=================================================
*/
	void VFrameGraphDebugger::_CopyBufferTaskToString (Ptr<const VFgTask<CopyBuffer>> task, INOUT String &str) const
	{
		str << indent << "	srcBuffer:    \"" << task->srcBuffer->GetDebugName() << "\"\n"
			<< indent << "	dstBuffer:    \"" << task->dstBuffer->GetDebugName() << "\"\n"
			<< indent << "	regions = {\n";

		for (auto& reg : task->regions) {
			str << indent << "\t\t{ " << ToString( reg.srcOffset ) << ", " << ToString( reg.dstOffset ) << ", " << ToString( reg.size ) << " }\n";
		}
		str << indent << "	}\n";
	}
	
/*
=================================================
	_CopyImageTaskToString
=================================================
*/
	void VFrameGraphDebugger::_CopyImageTaskToString (Ptr<const VFgTask<CopyImage>> task, INOUT String &str) const
	{
		str << indent << "	srcImage:    \"" << task->srcImage->GetDebugName() << "\"\n"
			<< indent << "	srcLayout:   " << VkImageLayout_ToString( task->srcLayout ) << '\n'
			<< indent << "	dstImage:    \"" << task->dstImage->GetDebugName() << "\"\n"
			<< indent << "	dstLayout:   " << VkImageLayout_ToString( task->dstLayout ) << '\n'
			<< indent << "	regions = {\n";

		for (auto& reg : task->regions) {
			str << indent << "\t\tRegion {\n"
				<< indent << "\t\t	src.aspectMask:  " << ToString( reg.srcSubresource.aspectMask ) << '\n'
				<< indent << "\t\t	src.mipLevel:    " << ToString( reg.srcSubresource.mipLevel.Get() ) << '\n'
				<< indent << "\t\t	src.baseLayer:   " << ToString( reg.srcSubresource.baseLayer.Get() ) << '\n'
				<< indent << "\t\t	src.layerCount:  " << ToString( reg.srcSubresource.layerCount ) << '\n'
				<< indent << "\t\t	src.offset:      " << ToString( reg.srcOffset ) << '\n'
				<< indent << "\t\t	dst.aspectMask:  " << ToString( reg.dstSubresource.aspectMask ) << '\n'
				<< indent << "\t\t	dst.mipLevel:    " << ToString( reg.dstSubresource.mipLevel.Get() ) << '\n'
				<< indent << "\t\t	dst.baseLayer:   " << ToString( reg.dstSubresource.baseLayer.Get() ) << '\n'
				<< indent << "\t\t	dst.layerCount:  " << ToString( reg.dstSubresource.layerCount ) << '\n'
				<< indent << "\t\t	dst.offset:      " << ToString( reg.dstOffset ) << '\n'
				<< indent << "\t\t	size:            " << ToString( reg.size ) << '\n'
				<< indent << "\t\t}\n";
		}
		str << indent << "	}\n";
	}
	
/*
=================================================
	_CopyBufferToImageTaskToString
=================================================
*/
	void VFrameGraphDebugger::_CopyBufferToImageTaskToString (Ptr<const VFgTask<CopyBufferToImage>> task, INOUT String &str) const
	{
		str << indent << "	srcBuffer:   \"" << task->srcBuffer->GetDebugName() << "\"\n"
			<< indent << "	dstImage:    \"" << task->dstImage->GetDebugName() << "\"\n"
			<< indent << "	dstLayout:   " << VkImageLayout_ToString( task->dstLayout ) << '\n'
			<< indent << "	regions = {\n";

		for (auto& reg : task->regions) {
			str << indent << "\t\tRegion {\n"
				<< indent << "\t\t	bufferOffset:        " << ToString( reg.bufferOffset ) << '\n'
				<< indent << "\t\t	bufferRowLength:     " << ToString( reg.bufferRowLength ) << '\n'
				<< indent << "\t\t	bufferImageHeight:   " << ToString( reg.bufferImageHeight ) << '\n'
				<< indent << "\t\t	imageAspectMask:     " << ToString( reg.imageLayers.aspectMask ) << '\n'
				<< indent << "\t\t	imageMipLevel:       " << ToString( reg.imageLayers.mipLevel.Get() ) << '\n'
				<< indent << "\t\t	imageBaseArrayLayer: " << ToString( reg.imageLayers.baseLayer.Get() ) << '\n'
				<< indent << "\t\t	imageLayerCount:     " << ToString( reg.imageLayers.layerCount ) << '\n'
				<< indent << "\t\t	imageOffset:         " << ToString( reg.imageOffset ) << '\n'
				<< indent << "\t\t	imageSize:           " << ToString( reg.imageSize ) << '\n'
				<< indent << "\t\t}\n";
		}
		str << indent << "	}\n";
	}
	
/*
=================================================
	_CopyImageToBufferTaskToString
=================================================
*/
	void VFrameGraphDebugger::_CopyImageToBufferTaskToString (Ptr<const VFgTask<CopyImageToBuffer>> task, INOUT String &str) const
	{
		str << indent << "	srcImage:    \"" << task->srcImage->GetDebugName() << "\"\n"
			<< indent << "	srcLayout:   " << VkImageLayout_ToString( task->srcLayout ) << '\n'
			<< indent << "	dstBuffer:   \"" << task->dstBuffer->GetDebugName() << "\"\n"
			<< indent << "	regions = {\n";

		for (auto& reg : task->regions) {
			str << indent << "\t\tRegion {\n"
				<< indent << "\t\t	bufferOffset:        " << ToString( reg.bufferOffset ) << '\n'
				<< indent << "\t\t	bufferRowLength:     " << ToString( reg.bufferRowLength ) << '\n'
				<< indent << "\t\t	bufferImageHeight:   " << ToString( reg.bufferImageHeight ) << '\n'
				<< indent << "\t\t	imageAspectMask:     " << ToString( reg.imageLayers.aspectMask ) << '\n'
				<< indent << "\t\t	imageMipLevel:       " << ToString( reg.imageLayers.mipLevel.Get() ) << '\n'
				<< indent << "\t\t	imageBaseArrayLayer: " << ToString( reg.imageLayers.baseLayer.Get() ) << '\n'
				<< indent << "\t\t	imageLayerCount:     " << ToString( reg.imageLayers.layerCount ) << '\n'
				<< indent << "\t\t	imageOffset:         " << ToString( reg.imageOffset ) << '\n'
				<< indent << "\t\t	imageSize:           " << ToString( reg.imageSize ) << '\n'
				<< indent << "\t\t}\n";
		}
		str << indent << "	}\n";
	}
	
/*
=================================================
	_BlitImageTaskToString
=================================================
*/
	void VFrameGraphDebugger::_BlitImageTaskToString (Ptr<const VFgTask<BlitImage>> task, INOUT String &str) const
	{
		str << indent << "	srcImage:    \"" << task->srcImage->GetDebugName() << "\"\n"
			<< indent << "	srcLayout:   " << VkImageLayout_ToString( task->srcLayout ) << '\n'
			<< indent << "	dstImage:    \"" << task->dstImage->GetDebugName() << "\"\n"
			<< indent << "	dstLayout:   " << VkImageLayout_ToString( task->dstLayout ) << '\n'
			<< indent << "	filter:      " << VkFilter_ToString( task->filter ) << '\n'
			<< indent << "	regions = {\n";

		for (auto& reg : task->regions) {
			str << indent << "\t\tRegion {\n"
				<< indent << "\t\t	src.aspectMask:  " << ToString( reg.srcSubresource.aspectMask ) << '\n'
				<< indent << "\t\t	src.mipLevel:    " << ToString( reg.srcSubresource.mipLevel.Get() ) << '\n'
				<< indent << "\t\t	src.baseLayer:   " << ToString( reg.srcSubresource.baseLayer.Get() ) << '\n'
				<< indent << "\t\t	src.layerCount:  " << ToString( reg.srcSubresource.layerCount ) << '\n'
				<< indent << "\t\t	src.offset0:     " << ToString( reg.srcOffset0 ) << '\n'
				<< indent << "\t\t	src.offset1:     " << ToString( reg.srcOffset1 ) << '\n'
				<< indent << "\t\t	dst.aspectMask:  " << ToString( reg.dstSubresource.aspectMask ) << '\n'
				<< indent << "\t\t	dst.mipLevel:    " << ToString( reg.dstSubresource.mipLevel.Get() ) << '\n'
				<< indent << "\t\t	dst.baseLayer:   " << ToString( reg.dstSubresource.baseLayer.Get() ) << '\n'
				<< indent << "\t\t	dst.layerCount:  " << ToString( reg.dstSubresource.layerCount ) << '\n'
				<< indent << "\t\t	dst.offset0:     " << ToString( reg.dstOffset0 ) << '\n'
				<< indent << "\t\t	dst.offset1:     " << ToString( reg.dstOffset1 ) << '\n'
				<< indent << "\t\t}\n";
		}
		str << indent << "	}\n";
	}
	
/*
=================================================
	_ResolveImageTaskToString
=================================================
*/
	void VFrameGraphDebugger::_ResolveImageTaskToString (Ptr<const VFgTask<ResolveImage>> task, INOUT String &str) const
	{
		str << indent << "	srcImage:   \"" << task->srcImage->GetDebugName() << "\"\n"
			<< indent << "	srcLayout:  " << VkImageLayout_ToString( task->srcLayout ) << '\n'
			<< indent << "	dstImage:   \"" << task->dstImage->GetDebugName() << "\"\n"
			<< indent << "	dstLayout:  " << VkImageLayout_ToString( task->dstLayout ) << '\n'
			<< indent << "	regions = {\n";

		for (auto& reg : task->regions) {
			str << indent << "\t\tRegion {\n"
				<< indent << "\t\t	src.aspectMask:  " << ToString( reg.srcSubresource.aspectMask ) << '\n'
				<< indent << "\t\t	src.mipLevel:    " << ToString( reg.srcSubresource.mipLevel.Get() ) << '\n'
				<< indent << "\t\t	src.baseLayer:   " << ToString( reg.srcSubresource.baseLayer.Get() ) << '\n'
				<< indent << "\t\t	src.layerCount:  " << ToString( reg.srcSubresource.layerCount ) << '\n'
				<< indent << "\t\t	src.offset:      " << ToString( reg.srcOffset ) << '\n'
				<< indent << "\t\t	dst.aspectMask:  " << ToString( reg.dstSubresource.aspectMask ) << '\n'
				<< indent << "\t\t	dst.mipLevel:    " << ToString( reg.dstSubresource.mipLevel.Get() ) << '\n'
				<< indent << "\t\t	dst.baseLayer:   " << ToString( reg.dstSubresource.baseLayer.Get() ) << '\n'
				<< indent << "\t\t	dst.layerCount:  " << ToString( reg.dstSubresource.layerCount ) << '\n'
				<< indent << "\t\t	dst.offset:      " << ToString( reg.dstOffset ) << '\n'
				<< indent << "\t\t	extent:          " << ToString( reg.extent ) << '\n'
				<< indent << "\t\t}\n";
		}

		str << indent << "	}\n";
	}
	
/*
=================================================
	_FillBufferTaskToString
=================================================
*/
	void VFrameGraphDebugger::_FillBufferTaskToString (Ptr<const VFgTask<FillBuffer>> task, INOUT String &str) const
	{
		str << indent << "	dstBuffer:   \"" << task->dstBuffer->GetDebugName() << "\"\n"
			<< indent << "	dstOffset:   " << ToString( BytesU{task->dstOffset} ) << '\n'
			<< indent << "	size:        " << ToString( BytesU{task->size} ) << '\n'
			<< indent << "	pattern:     " << ToString<16>( task->pattern ) << '\n';
	}
	
/*
=================================================
	_ClearColorImageTaskToString
=================================================
*/
	void VFrameGraphDebugger::_ClearColorImageTaskToString (Ptr<const VFgTask<ClearColorImage>> task, INOUT String &str) const
	{
		str << indent << "	dstImage:   \"" << task->dstImage->GetDebugName() << "\"\n"
			<< indent << "	dstLayout:  " << VkImageLayout_ToString( task->dstLayout ) << '\n'
			<< indent << "	ranges = {\n";

		for (auto& range : task->ranges) {
			str << indent << "\t\tRange {\n"
				<< indent << "\t\t	aspectMask:     " << ToString( range.aspectMask ) << '\n'
				<< indent << "\t\t	baseMipLevel:   " << ToString( range.baseMipLevel.Get() ) << '\n'
				<< indent << "\t\t	levelCount:     " << ToString( range.levelCount ) << '\n'
				<< indent << "\t\t	baseArrayLayer: " << ToString( range.baseLayer.Get() ) << '\n'
				<< indent << "\t\t	layerCount:     " << ToString( range.layerCount ) << '\n'
				<< indent << "\t\t}\n";
		}
		str << indent << "	}\n";
	}
	
/*
=================================================
	_ClearDepthStencilImageTaskToString
=================================================
*/
	void VFrameGraphDebugger::_ClearDepthStencilImageTaskToString (Ptr<const VFgTask<ClearDepthStencilImage>> task, INOUT String &str) const
	{
		str << indent << "	dstImage:   \"" << task->dstImage->GetDebugName() << "\"\n"
			<< indent << "	dstLayout:  " << VkImageLayout_ToString( task->dstLayout ) << '\n'
			<< indent << "	clearValue: " << ToString( task->clearValue.depth ) << ", " << ToString( task->clearValue.stencil ) << '\n'
			<< indent << "	ranges = {\n";

		for (auto& range : task->ranges) {
			str << indent << "\t\tRange {\n"
				<< indent << "\t\t	aspectMask:     " << ToString( range.aspectMask ) << '\n'
				<< indent << "\t\t	baseMipLevel:   " << ToString( range.baseMipLevel.Get() ) << '\n'
				<< indent << "\t\t	levelCount:     " << ToString( range.levelCount ) << '\n'
				<< indent << "\t\t	baseArrayLayer: " << ToString( range.baseLayer.Get() ) << '\n'
				<< indent << "\t\t	layerCount:     " << ToString( range.layerCount ) << '\n'
				<< indent << "\t\t}\n";
		}
		str << indent << "	}\n";
	}
	
/*
=================================================
	_UpdateBufferTaskToString
=================================================
*/
	void VFrameGraphDebugger::_UpdateBufferTaskToString (Ptr<const VFgTask<UpdateBuffer>> task, INOUT String &str) const
	{
		str << indent << "	dstBuffer:   \"" << task->dstBuffer->GetDebugName() << "\"\n"
			<< indent << "	dstOffset:   " << ToString( BytesU{task->dstOffset} ) << '\n'
			<< indent << "	size:        " << ToString( BytesU{task->DataSize()} ) << '\n';
	}
	
/*
=================================================
	_PresentTaskToString
=================================================
*/
	void VFrameGraphDebugger::_PresentTaskToString (Ptr<const VFgTask<Present>> task, INOUT String &str) const
	{
		str << indent << "	srcImage:   \"" << task->srcImage->GetDebugName() << "\"\n"
			<< indent << "	layer:      " << ToString( task->layer.Get() ) << '\n';
	}
	
/*
=================================================
	_PresentVRTaskToString
=================================================
*/
	void VFrameGraphDebugger::_PresentVRTaskToString (Ptr<const VFgTask<PresentVR>> task, INOUT String &str) const
	{
		str << indent << "	leftEyeImage:   \"" << task->leftEyeImage->GetDebugName() << "\"\n"
			<< indent << "	leftEyeLayer:   " << ToString( task->leftEyeLayer.Get() ) << '\n'
			<< indent << "	rightEyeImage:  \"" << task->rightEyeImage->GetDebugName() << "\"\n"
			<< indent << "	rightEyeLayer:  " << ToString( task->rightEyeLayer.Get() ) << '\n';
	}
	
/*
=================================================
	_DumpTaskData
=================================================
*/
	void VFrameGraphDebugger::_DumpTaskData (TaskPtr taskPtr, INOUT String &str) const
	{
		if ( auto task = DynCast< VFgTask<SubmitRenderPass> >(taskPtr) )
			return _SubmitRenderPassTaskToString( task, INOUT str );

		if ( auto task = DynCast< VFgTask<DispatchCompute> >(taskPtr) )
			return _DispatchComputeTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<DispatchComputeIndirect> >(taskPtr) )
			return _DispatchComputeIndirectTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<CopyBuffer> >(taskPtr) )
			return _CopyBufferTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<CopyImage> >(taskPtr) )
			return _CopyImageTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<CopyBufferToImage> >(taskPtr) )
			return _CopyBufferToImageTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<CopyImageToBuffer> >(taskPtr) )
			return _CopyImageToBufferTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<BlitImage> >(taskPtr) )
			return _BlitImageTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<ResolveImage> >(taskPtr) )
			return _ResolveImageTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<FillBuffer> >(taskPtr) )
			return _FillBufferTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<ClearColorImage> >(taskPtr) )
			return _ClearColorImageTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<ClearDepthStencilImage> >(taskPtr) )
			return _ClearDepthStencilImageTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<UpdateBuffer> >(taskPtr) )
			return _UpdateBufferTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<Present> >(taskPtr) )
			return _PresentTaskToString( task, INOUT str );
		
		if ( auto task = DynCast< VFgTask<PresentVR> >(taskPtr) )
			return _PresentVRTaskToString( task, INOUT str );
		
		ASSERT(false);
		// TODO
	}


}	// FG

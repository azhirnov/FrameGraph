// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraphDebugger.h"
#include "VFrameGraph.h"
#include "VEnumToString.h"
#include "Shared/EnumToString.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VFrameGraphDebugger::VFrameGraphDebugger () :
		_flags{ Default }
	{
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
	Clear
=================================================
*/
	void VFrameGraphDebugger::Clear ()
	{
		_tasksPerQueue.clear();
		_images.clear();
		_buffers.clear();
	}
	
/*
=================================================
	RunTask
=================================================
*/
	void VFrameGraphDebugger::RunTask (VCommandQueue const* queue, TaskPtr task)
	{
		auto	iter = _tasksPerQueue.find( queue );

		if ( iter == _tasksPerQueue.end() )
			iter = _tasksPerQueue.insert({ queue, {} }).first;

		iter->second.insert({ task->ExecutionOrder(), TaskInfo{task} });
	}

/*
=================================================
	AddBufferBarrier
=================================================
*/	
	void VFrameGraphDebugger::AddBufferBarrier (VBuffer *					buffer,
												ExeOrderIndex				srcIndex,
												ExeOrderIndex				dstIndex,
												VkPipelineStageFlags		srcStageMask,
												VkPipelineStageFlags		dstStageMask,
												VkDependencyFlags			dependencyFlags,
												const VkBufferMemoryBarrier	&barrier)
	{
		VBufferPtr	ptr		= buffer->GetSelfShared();
		auto		iter	= _buffers.find( ptr );

		if ( iter == _buffers.end() )
			iter = _buffers.insert({ ptr, {} }).first;

		iter->second.barriers.push_back({ srcIndex, dstIndex, srcStageMask, dstStageMask, dependencyFlags, barrier });
	}
	
/*
=================================================
	AddImageBarrier
=================================================
*/
	void VFrameGraphDebugger::AddImageBarrier (VImage *						image,
											   ExeOrderIndex				srcIndex,
											   ExeOrderIndex				dstIndex,
											   VkPipelineStageFlags			srcStageMask,
											   VkPipelineStageFlags			dstStageMask,
											   VkDependencyFlags			dependencyFlags,
											   const VkImageMemoryBarrier	&barrier)
	{
		VImagePtr	ptr		= image->GetSelfShared();
		auto		iter	= _images.find( ptr );

		if ( iter == _images.end() )
			iter = _images.insert({ ptr, {} }).first;

		iter->second.barriers.push_back({ srcIndex, dstIndex, srcStageMask, dstStageMask, dependencyFlags, barrier });
	}
	
/*
=================================================
	AddBufferUsage
=================================================
*/
	void VFrameGraphDebugger::AddBufferUsage (VBuffer *buffer, const VBuffer::BufferState &state)
	{
		ASSERT( buffer and state.task );

		for (auto& q : _tasksPerQueue)
		{
			auto	iter = q.second.find( state.task->ExecutionOrder() );

			if ( iter == q.second.end() )
				continue;

			iter->second.resources.push_back(BufferUsage_t{ buffer->GetSelfShared(), state });
			return;
		}

		ASSERT( !"task doesn't exist!" );
	}
	
/*
=================================================
	AddImageUsage
=================================================
*/
	void VFrameGraphDebugger::AddImageUsage (VImage *image, const VImage::ImageState &state)
	{
		ASSERT( image and state.task );
		
		for (auto& q : _tasksPerQueue)
		{
			auto	iter = q.second.find( state.task->ExecutionOrder() );

			if ( iter == q.second.end() )
				continue;

			iter->second.resources.push_back(ImageUsage_t{ image->GetSelfShared(), state });
			return;
		}

		ASSERT( !"task doesn't exist!" );
	}

/*
=================================================
	DumpFrame
----
	- sort resources by name (this needed for correct dump comparison in tests).
	- dump resources info & frame barriers.
=================================================
*/
	void VFrameGraphDebugger::DumpFrame (OUT String &str) const
	{
		_DumpImages( OUT str );
		_DumpBuffers( OUT str );

		for (auto& q : _tasksPerQueue) {
			_DumpQueue( q.first, q.second, OUT str );
		}
	}
	
/*
=================================================
	_DumpImages
=================================================
*/
	void VFrameGraphDebugger::_DumpImages (OUT String &str) const
	{
		Array< ImageResources_t::value_type const* >	sorted;

		for (auto& img : _images) {
			sorted.push_back( &img );
		}

		std::sort( sorted.begin(), sorted.end(),
					[](auto* lhs, auto* rhs)
					{
						if ( lhs->first->GetDebugName() != rhs->first->GetDebugName() )
							return lhs->first->GetDebugName() < rhs->first->GetDebugName();

						return lhs->first < rhs->first;
					});

		for (auto& img : sorted)
		{
			_DumpImage( img->first, img->second, OUT str );
		}
	}
	
/*
=================================================
	_DumpBuffers
=================================================
*/
	void VFrameGraphDebugger::_DumpBuffers (OUT String &str) const
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
			_DumpBuffer( buf->first, buf->second, OUT str );
		}
	}
	
/*
=================================================
	_DumpImage
=================================================
*/
	void VFrameGraphDebugger::_DumpImage (const VImagePtr &image, const ImageInfo_t &info, OUT String &str) const
	{
		str << "Image {"
			<< "\n	name:         " << image->GetDebugName()
			<< "\n	iamgeType:    " << ToString( image->Description().imageType )
			<< "\n	dimension:    " << ToString( image->Description().dimension )
			<< "\n	format:       " << ToString( image->Description().format )
			<< "\n	usage:        " << ToString( image->Description().usage )
			<< "\n	arrayLayers:  " << ToString( image->Description().arrayLayers.Get() )
			<< "\n	maxLevel:     " << ToString( image->Description().maxLevel.Get() )
			<< "\n	samples:      " << ToString( image->Description().samples.Get() );

		if ( not info.barriers.empty() )
		{
			str << "\n	barriers = {";

			for (auto& bar : info.barriers)
			{
				str << "\n\t\t	ImageMemoryBarrier {"
					<< "\n\t\t\t	srcTask:         " << _GetTaskName( bar.srcIndex )
					<< "\n\t\t\t	dstTask:         " << _GetTaskName( bar.dstIndex )
					<< "\n\t\t\t	srcStageMask:    " << VkPipelineStage_ToString( bar.srcStageMask )
					<< "\n\t\t\t	dstStageMask:    " << VkPipelineStage_ToString( bar.dstStageMask )
					<< "\n\t\t\t	dependencyFlags: " << VkDependency_ToString( bar.dependencyFlags )
					<< "\n\t\t\t	srcAccessMask:   " << VkAccess_ToString( bar.barrier.srcAccessMask )
					<< "\n\t\t\t	dstAccessMask:   " << VkAccess_ToString( bar.barrier.dstAccessMask )
					//<< "\n\t\t\t	srcQueueFamilyIndex"	// TODO: get debug name from queue
					//<< "\n\t\t\t	dstQueueFamilyIndex"
					<< "\n\t\t\t	oldLayout:       " << VkImageLayout_ToString( bar.barrier.oldLayout )
					<< "\n\t\t\t	newLayout:       " << VkImageLayout_ToString( bar.barrier.newLayout )
					<< "\n\t\t\t	aspectMask:      " << VkImageAspect_ToString( bar.barrier.subresourceRange.aspectMask )
					<< "\n\t\t\t	baseMipLevel:    " << ToString( bar.barrier.subresourceRange.baseMipLevel )
					<< "\n\t\t\t	levelCount:      " << ToString( bar.barrier.subresourceRange.levelCount )
					<< "\n\t\t\t	baseArrayLayer:  " << ToString( bar.barrier.subresourceRange.baseArrayLayer )
					<< "\n\t\t\t	layerCount:      " << ToString( bar.barrier.subresourceRange.layerCount )
					<< "\n\t\t	}";
			}
			str << "\n	}";
		}
		
		str << "\n}\n\n";
	}
	
/*
=================================================
	_DumpBuffer
=================================================
*/
	void VFrameGraphDebugger::_DumpBuffer (const VBufferPtr &buffer, const BufferInfo_t &info, OUT String &str) const
	{
		str << "Buffer {"
			<< "\n	name:    " << buffer->GetDebugName()
			<< "\n	size:    " << ToString( uint64_t(buffer->Description().size) )
			<< "\n	usage:   " << ToString( buffer->Description().usage );
	
		if ( not info.barriers.empty() )
		{
			str << "\n	barriers = {";

			for (auto& bar : info.barriers)
			{
				str << "\n\t\t	BufferMemoryBarrier {"
					<< "\n\t\t\t	srcTask:         " << _GetTaskName( bar.srcIndex )
					<< "\n\t\t\t	dstTask:         " << _GetTaskName( bar.dstIndex )
					<< "\n\t\t\t	srcStageMask:    " << VkPipelineStage_ToString( bar.srcStageMask )
					<< "\n\t\t\t	dstStageMask:    " << VkPipelineStage_ToString( bar.dstStageMask )
					<< "\n\t\t\t	dependencyFlags: " << VkDependency_ToString( bar.dependencyFlags )
					<< "\n\t\t\t	srcAccessMask:   " << VkAccess_ToString( bar.barrier.srcAccessMask )
					<< "\n\t\t\t	dstAccessMask:   " << VkAccess_ToString( bar.barrier.dstAccessMask )
					<< "\n\t\t\t	offset:          " << ToString( bar.barrier.offset )
					<< "\n\t\t\t	size:            " << ToString( bar.barrier.size )
					<< "\n\t\t	}";
			}
			str << "\n	}";
		}
		
		str << "\n}\n\n";
	}

/*
=================================================
	_GetTaskName
=================================================
*/
	String  VFrameGraphDebugger::_GetTaskName (ExeOrderIndex idx) const
	{
		for (auto& q : _tasksPerQueue)
		{
			auto	iter = q.second.find( idx );

			if ( iter == q.second.end() )
				continue;

			return String(iter->second.task->Name()) << " (" << ToString( idx ) << ')';
		}

		ASSERT(false);
		return "<unknown>";
	}

/*
=================================================
	_DumpQueue
=================================================
*/
	void VFrameGraphDebugger::_DumpQueue (VCommandQueue const* queue, const TaskMap_t &tasks, OUT String &str) const
	{
		str.clear();
		str << "CommandQueue {"
			<< "\n	name:  " << queue->GetDebugName()
			<< "\n	tasks = {";
		
		String	temp;

		for (auto& info : tasks)
		{
			auto&	task = info.second.task;

			str << "\n\t\t	Task {"
				<< "\n\t\t\t	name:    " << task->Name()
				<< "\n\t\t\t	input =  { ";

			for (auto& in : task->Inputs())
			{
				if ( &in != task->Inputs().begin() )
					str << ", ";

				str += in->Name();
			}

			str << " }"
				<< "\n\t\t\t	output = { ";

			for (auto& out : task->Outputs())
			{
				if ( &out != task->Outputs().begin() )
					str << ", ";

				str += out->Name();
			}
			str << " }";

			_DumpResourceUsage( info.second.resources, OUT temp );
			
			str << temp << "\n\t\t	}\n";
		}

		str << "\n	}\n}\n\n";
	}
	
/*
=================================================
	_DumpResourceUsage
=================================================
*/
	void VFrameGraphDebugger::_DumpResourceUsage (ArrayView<ResourceUsage_t> resources, OUT String &str) const
	{
		struct ResInfo
		{
			StringView				name;
			ResourceUsage_t const*	usage	= null;
		};

		str.clear();

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
		str << "\n\t\t\t	resource_usage = {";

		for (auto& res : sorted)
		{
			if ( auto* image = std::get_if<ImageUsage_t>( res.usage ) )
			{
				str << "\n\t\t\t\t\t	ImageUsage {"
					<< "\n\t\t\t\t\t\t	name:           " << res.name
					<< "\n\t\t\t\t\t\t	usage:          " << ToString( image->second.state )
					<< "\n\t\t\t\t\t\t	baseMipLevel:   " << ToString( image->second.range.Mipmaps().begin )
					<< "\n\t\t\t\t\t\t	levelCount:     " << ToString( image->second.range.Mipmaps().Count() )
					<< "\n\t\t\t\t\t\t	baseArrayLayer: " << ToString( image->second.range.Layers().begin )
					<< "\n\t\t\t\t\t\t	layerCount:     " << ToString( image->second.range.Layers().Count() )
					<< "\n\t\t\t\t\t	}";
			}
			else
			if ( auto* buffer = std::get_if<BufferUsage_t>( res.usage ) )
			{
				str << "\n\t\t\t\t\t	BufferUsage {"
					<< "\n\t\t\t\t\t\t	name:     " << res.name
					<< "\n\t\t\t\t\t\t	usage:    " << ToString( buffer->second.state )
					<< "\n\t\t\t\t\t\t	offset:   " << ToString( buffer->second.range.begin )
					<< "\n\t\t\t\t\t\t	size:     " << ToString( buffer->second.range.Count() )
					<< "\n\t\t\t\t\t	}";
			}
		}

		str << "\n\t\t\t	}";
	}


}	// FG

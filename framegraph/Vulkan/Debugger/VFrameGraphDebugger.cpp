// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraphDebugger.h"
#include "VEnumToString.h"
#include "Shared/EnumToString.h"
#include "VTaskGraph.h"
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
	VFrameGraphDebugger::VFrameGraphDebugger (const VDebugger &dbg) :
		_mainDbg{ dbg },
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
	void VFrameGraphDebugger::OnEndFrame (const CommandBatchID &batchId, uint index)
	{
		constexpr auto	DumpFlags =	ECompilationDebugFlags::LogTasks		|
									ECompilationDebugFlags::LogBarriers		|
									ECompilationDebugFlags::LogResourceUsage;
		
		_cmdBatchId		= batchId;
		_indexInBatch	= index;

		if ( EnumEq( _flags, DumpFlags ) )
		{
			_DumpFrame( OUT _frameDump );
			_DumpGraph( OUT _graphViz );
		}

		_tasks.clear();
		_images.clear();
		_buffers.clear();
	}
	
/*
=================================================
	OnSync
=================================================
*/
	void VFrameGraphDebugger::OnSync ()
	{
		_mainDbg.AddFrameDump( _cmdBatchId, _indexInBatch, INOUT _frameDump );
		_mainDbg.AddGraphDump( _cmdBatchId, _indexInBatch, INOUT _graphViz );
		
		_frameDump.clear();
		_graphViz.clear();
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
	void VFrameGraphDebugger::_DumpFrame (OUT String &str) const
	{
		str.clear();
		str << "Thread {\n"
			<< "	batch:         \"" << _cmdBatchId.GetName() << "\"\n"
			<< "	indexInBatch:  " << ToString(_indexInBatch) << '\n';

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
			<< indent << "	name:         " << image->GetDebugName() << '\n'
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
			<< indent << "	name:    " << buffer->GetDebugName() << '\n'
			<< indent << "	size:    " << ToString( uint64_t(buffer->Description().size) ) << '\n'
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
					<< indent << "\t\t		offset:          " << ToString( bar.info.offset ) << '\n'
					<< indent << "\t\t		size:            " << ToString( bar.info.size ) << '\n'
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
	inline VFrameGraphDebugger::TaskPtr  VFrameGraphDebugger::_GetTask (ExeOrderIndex idx) const
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
				<< indent << "	name:    " << _GetTaskName( info.task ) << '\n'
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
					<< indent << "\t		name:           " << res.name << '\n'
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
					<< indent << "\t		name:     " << res.name << '\n'
					<< indent << "\t		usage:    " << ToString( buffer->second.state ) << '\n'
					<< indent << "\t		offset:   " << ToString( buffer->second.range.begin ) << '\n'
					<< indent << "\t		size:     " << ToString( buffer->second.range.Count() ) << '\n'
					<< indent << "\t	}\n";
			}
		}

		str << indent << "\t}\n";
	}

/*
=================================================
	ColToStr
=================================================
*/
	ND_ inline String  ColToStr (RGBA8u col)
	{
		uint	val = (uint(col.b) << 16) | (uint(col.g) << 8) | uint(col.r);
		String	str = ToString<16>( val );

		for (; str.length() < 6;) {
			str.insert( str.begin(), '0' );
		}
		return str;
	}

/*
=================================================
	NodeUniqueNames
=================================================
*/
	struct VFrameGraphDebugger::NodeUniqueNames
	{
		// task
		ND_ String  operator () (TaskPtr task) const
		{
			return "n"s << ToString<16>( uint(task ? task->ExecutionOrder() : ExeOrderIndex::Initial) );
		}

		// resource
		ND_ String  operator () (const VBuffer *buffer, TaskPtr task) const
		{
			return operator() ( buffer, task ? task->ExecutionOrder() : ExeOrderIndex::Initial );
		}

		ND_ String  operator () (const VBuffer *buffer, ExeOrderIndex index) const
		{
			return "buf"s << ToString<16>( uint64_t(buffer->Handle()) ) << '_' << ToString<16>( uint(index) );
		}

		ND_ String  operator () (const VImage *image, TaskPtr task) const
		{
			return operator() ( image, task ? task->ExecutionOrder() : ExeOrderIndex::Initial );
		}

		ND_ String  operator () (const VImage *image, ExeOrderIndex index) const
		{
			return "img"s << ToString<16>( uint64_t(image->Handle()) ) << '_' << ToString<16>( uint(index) );
		}

		// resource -> resource barrier name
		ND_ String  operator () (const VBuffer *buffer, ExeOrderIndex srcIndex, ExeOrderIndex dstIndex) const
		{
			return "bufBar"s << ToString<16>( uint64_t(buffer->Handle()) )
							<< '_' << ToString<16>( uint(srcIndex) )
							<< '_' << ToString<16>( uint(dstIndex) );
		}

		ND_ String  operator () (const VImage *image, ExeOrderIndex srcIndex, ExeOrderIndex dstIndex) const
		{
			return "imgBuf"s << ToString<16>( uint64_t(image->Handle()) )
							<< '_' << ToString<16>( uint(srcIndex) )
							<< '_' << ToString<16>( uint(dstIndex) );
		}

		// draw task
		/*ND_ String  operator () (const UniquePtr<IDrawTask> &task) const
		{
			return "d"_str	<< String().FormatAlignedI( usize(task.RawPtr()), sizeof(usize)*2, '0', 16 );
		}*/
	};
	
/*
=================================================
	ColorScheme
=================================================
*/
	struct VFrameGraphDebugger::ColorScheme
	{
		ND_ static String  TaskLabelColor (RGBA8u)
		{
			return ColToStr( HtmlColor::White );
		}

		ND_ static String  DrawTaskBG ()
		{
			return ColToStr( HtmlColor::Bisque );
		}

		ND_ static String  DrawTaskLabelColor ()
		{
			return ColToStr( HtmlColor::Black );
		}

		ND_ static String  ResourceBG (const VBuffer *)
		{
			return ColToStr( HtmlColor::Silver );
		}

		ND_ static String  ResourceBG (const VImage *)
		{
			return ColToStr( HtmlColor::Silver );
		}

		ND_ static String  ResourceToResourceEdge (TaskPtr task)
		{
			return ColToStr( task ? task->DebugColor() : HtmlColor::Pink );
		}

		ND_ static String  ResGroupBG (TaskPtr task)
		{
			return ColToStr( task ? Lerp( HtmlColor::Black, task->DebugColor(), 0.5f ) : HtmlColor::Pink );	// TODO: use hsv
		}

		/*ND_ static String  BarrierBG (EBarrier::bits bar)
		{
			color4u	col;
			if ( bar[EBarrier::WriteRead] )		col = color4u::Html::Red();			else
			if ( bar[EBarrier::ReadWrite] )		col = color4u::Html::LimeGreen();	else
			if ( bar[EBarrier::WriteWrite] )	col = color4u::Html::DodgerBlue();	else
			if ( bar[EBarrier::Layout] )		col = color4u::Html::Yellow();		else
												col = color4u::Html::Pink();	// unknown barrier
			return ColToStr( col );
		}*/
	};

/*
=================================================
	BufferStateToString
=================================================
*/
	ND_ static String  BufferStateToString (EResourceState state)
	{
		state = state & ~EResourceState::_ShaderMask;	// TODO: convert shader flags too

		switch ( state )
		{
			case EResourceState::ShaderRead :		return "StorageBuffer-R";
			case EResourceState::ShaderWrite :		return "StorageBuffer-W";
			case EResourceState::ShaderReadWrite :	return "StorageBuffer-RW";
			case EResourceState::UniformRead :		return "UniformBuffer";
			case EResourceState::TransferSrc :		return "TransferSrc";
			case EResourceState::TransferDst :		return "TransferDst";
			case EResourceState::HostRead :			return "HostRead";
			case EResourceState::HostWrite :		return "HostWrite";
			case EResourceState::HostReadWrite :	return "HostReadWrite";
			case EResourceState::IndirectBuffer :	return "IndirectBuffer";
			case EResourceState::IndexBuffer :		return "IndexBuffer";
			case EResourceState::VertexBuffer :		return "VertexBuffer";
		}
		RETURN_ERR( "unknown state!" );
	}

/*
=================================================
	ImageStateToString
=================================================
*/
	ND_ static String  ImageStateToString (EResourceState state)
	{
		state = state & ~EResourceState::_ShaderMask;	// TODO: convert shader flags too

		switch ( state )
		{
			case EResourceState::ShaderRead :						return "StorageImage-R";
			case EResourceState::ShaderWrite :						return "StorageImage-W";
			case EResourceState::ShaderReadWrite :					return "StorageImage-RW";
			case EResourceState::ShaderSample :						return "Texture";
			case EResourceState::InputAttachment :					return "InputAttachment";
			case EResourceState::TransientAttachment :				return "TransientAttachment";
			case EResourceState::TransferSrc :						return "TransferSrc";
			case EResourceState::TransferDst :						return "TransferDst";
			case EResourceState::ColorAttachmentRead :				return "ColorAttachment-R";
			case EResourceState::ColorAttachmentWrite :				return "ColorAttachment-W";
			case EResourceState::ColorAttachmentReadWrite :			return "ColorAttachment-RW";
			case EResourceState::DepthStencilAttachmentRead :		return "DepthStencilAttachment-R";
			case EResourceState::DepthStencilAttachmentWrite :		return "DepthStencilAttachment-W";
			case EResourceState::DepthStencilAttachmentReadWrite :	return "DepthStencilAttachment-RW";
			case EResourceState::HostRead :							return "HostRead";
			case EResourceState::HostWrite :						return "HostWrite";
			case EResourceState::HostReadWrite :					return "HostReadWrite";
			case EResourceState::PresentImage :						return "Present";
		}
		RETURN_ERR( "unknown state!" );
	}
	
/*
=================================================
	VkImageLayoutToString
=================================================
*/
	ND_ static String  VkImageLayoutToString (VkImageLayout layout)
	{
		ENABLE_ENUM_CHECKS();
		switch ( layout )
		{
			case VK_IMAGE_LAYOUT_UNDEFINED :									return "Undefined";
			case VK_IMAGE_LAYOUT_GENERAL :										return "General";
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :						return "ColorAttachment";
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :				return "DepthStencilAttachment";
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL :				return "DepthStencilReadOnly";
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :						return "ShaderReadOnly";
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL :							return "TransferSrc";
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL :							return "TransferDst";
			case VK_IMAGE_LAYOUT_PREINITIALIZED :								return "Preintialized";
			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL :	return "DepthReadOnlyStencilAttachment";
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL :	return "DepthAttachmentStencilReadOnly";
			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :								return "PresentSrc";
			case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR :							return "SharedPresent";
			case VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV :						return "ShadingRate";
			case VK_IMAGE_LAYOUT_RANGE_SIZE :									// to shutup warnings
			case VK_IMAGE_LAYOUT_MAX_ENUM :
			default :															break;
		}
		DISABLE_ENUM_CHECKS();
		RETURN_ERR( "unknown image layout" );
	}

/*
=================================================
	GetBufferName
=================================================
*/
	ND_ static StringView  GetBufferName (const VBuffer *buffer)
	{
		return not buffer->GetDebugName().empty() ? buffer->GetDebugName() : "Buffer"sv;
	}

/*
=================================================
	GetImageName
=================================================
*/
	ND_ static StringView  GetImageName (const VImage *image)
	{
		return not image->GetDebugName().empty() ? image->GetDebugName() : "Image"sv;
	}

/*
=================================================
	_DumpGraph
----
	docs: https://graphviz.gitlab.io/_pages/pdf/dotguide.pdf
=================================================
*/
	void VFrameGraphDebugger::_DumpGraph (OUT String &str) const
	{
		str.clear();
		
		const NodeUniqueNames	node_name;
		const ColorScheme		color_scheme;
		HashSet<String>			existing_barriers;

		String	deps;
		String	subgraphs;

		str << indent << "subgraph cluster_Thread" << ToString<16>( size_t(this) ) << " {\n"
			<< indent << "	style=filled;\n";
		
		for (auto& info : _tasks)
		{
			if ( not info.task )
				continue;
			
			// add draw tasks
			/*if ( EnumEq( _flags, ECompilationDebugFlags::VisDrawTasks ) )
			{
				if ( auto* submit_rp = DynCast< FGTask<FGMsg::SubmitRenderPass> *>(node) )
				{
					const String	root	= node_name( node ) + "_draw";
					String			ending;

					if ( auto next_pass = submit_rp->GetNextSubpass() )
					{
						ending = node_name( next_pass );
					}
					
					str << '\t' << root << " [shape=record, label=\"";

					const auto&	draw_tasks = submit_rp->GetRenderPass()->GetDrawTasks();

					FOR( i, draw_tasks )
					{
						const auto&	draw = draw_tasks[i];

						str << (i ? "|<" : "<") << node_name( draw ) << "> " << draw->GetName();
					}
					str << "\", fontsize=12, fillcolor=\"#" << color_scheme.DrawTaskBG() << "\"];\n";

					deps << '\t' << node_name( node ) << " -> " << root << " [color=\"#" << color_scheme.DrawTaskBG() << "\", style=dotted];\n";
				}
			}*/
			

			// add task with resource usage
			if ( EnumEq( _flags, ECompilationDebugFlags::VisResources ) and
				 not info.resources.empty() )
			{
				String	res_style;
				_GetResourceUsage( info.resources, OUT res_style, INOUT str, INOUT deps, INOUT existing_barriers );

				if ( res_style.size() )
				{
					subgraphs
						<< indent << "\tsubgraph cluster_" << node_name( info.task ) << " {\n"
						<< indent << "\t	style=filled;\n"
						<< indent << "\t	color=\"#" << color_scheme.ResGroupBG( info.task ) << "\";\n"
						<< indent << "\t	fontcolor=\"#" << color_scheme.TaskLabelColor( info.task->DebugColor() ) << "\";\n"
						<< indent << "\t	label=\"" << info.task->Name() << "\";\n"
						<< res_style << '\n'
						<< indent << "\t}\n";
				}
			}
			else
			// add task
			{
				const String	color = ColToStr( info.task->DebugColor() );

				// add style
				str << indent << '\t' << node_name( info.task ) << " [label=\"" << info.task->Name() << "\", fontsize=24, fillcolor=\"#" << color
					<< "\", fontcolor=\"#" << color_scheme.TaskLabelColor( info.task->DebugColor() ) << "\"];\n";

				// add dependencies
				if ( not info.task->Outputs().empty() )
				{
					deps << indent << '\t' << node_name( info.task ) << " -> { ";
				
					for (auto& out_node : info.task->Outputs())
					{
						deps << node_name( out_node ) << "; ";
					}
					deps << "} [color=\"#" << color << "\", style=bold];\n";
				}
			}
		}
		
		str << '\n'
			<< subgraphs << '\n'
			<< deps
			<< indent << "}\n";
	}
	
/*
=================================================
	_GetResourceUsage
=================================================
*/
	void VFrameGraphDebugger::_GetResourceUsage (ArrayView<ResourceUsage_t> resources, OUT String &resStyle,
												 INOUT String &style, INOUT String &deps, INOUT HashSet<String> &existingBarriers) const
	{
		const NodeUniqueNames	node_name;
		const ColorScheme		color_scheme;

		for (auto& res : resources)
		{
			if ( auto* image = std::get_if<ImageUsage_t>( &res ) )
			{
				auto&	range = image->second.range;

				// add style
				resStyle
					<< indent << "\t\t" << node_name( image->first, image->second.task ) << " [label=\"" << GetImageName( image->first ) << "\\n"
					<< ImageStateToString( image->second.state ) << "\\n"
					//<< VkImageLayoutToString( image->second.layout ) << "\\n"
					<< "layer " << ToString(range.Layers().begin) << (range.IsWholeLayers() ? "..whole" : range.Layers().Count() > 1 ? ".."s << ToString(range.Layers().end) : "") << "\\n"
					<< "mipmap " << ToString(range.Mipmaps().begin) << (range.IsWholeMipmaps() ? "..whole" : range.Mipmaps().Count() > 1 ? ".."s << ToString(range.Mipmaps().end) : "") << "\\n"
					<< "\", fontsize=10, fillcolor=\"#" << color_scheme.ResourceBG( image->first )
					<< "\"];\n";

				_GetImageBarrier( image->first, image->second.task, INOUT style, INOUT deps, INOUT existingBarriers );
			}
			else
			if ( auto* buffer = std::get_if<BufferUsage_t>( &res ) )
			{
				auto&	range = buffer->second.range;

				// add style
				resStyle
					<< indent << "\t\t" << node_name( buffer->first, buffer->second.task ) << " [label=\"" << GetBufferName( buffer->first ) << "\\n"
					<< BufferStateToString( buffer->second.state ) << "\\n"
					<< "range " << ToString(range.begin) << ".." << (range.IsWhole() ? "whole" : ToString(range.end-1)) << "\\n"
					<< "\", fontsize=10, fillcolor=\"#" << color_scheme.ResourceBG( buffer->first )
					<< "\"];\n";

				_GetBufferBarrier( buffer->first, buffer->second.task, INOUT style, INOUT deps, INOUT existingBarriers );
			}
			else
			{
				ASSERT( !"unknown resource type!" );
			}
		}
	}
	
/*
=================================================
	GetAccessType
=================================================
*/
	enum class EAccessType {
		Read  = 1,
		Write = 2
	};
	FG_BIT_OPERATORS( EAccessType );

	ND_ static EAccessType  GetAccessType (VkAccessFlags flags)
	{
		EAccessType		result = EAccessType(0);

		for (VkAccessFlags t = 1; t < VK_ACCESS_FLAG_BITS_MAX_ENUM; t <<= 1)
		{
			if ( not EnumEq( flags, t ) )
				continue;

			ENABLE_ENUM_CHECKS();
			switch ( VkAccessFlagBits(t) )
			{
				case VK_ACCESS_INDIRECT_COMMAND_READ_BIT :					result |= EAccessType::Read;	break;
				case VK_ACCESS_INDEX_READ_BIT :								result |= EAccessType::Read;	break;
				case VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT :					result |= EAccessType::Read;	break;
				case VK_ACCESS_UNIFORM_READ_BIT :							result |= EAccessType::Read;	break;
				case VK_ACCESS_INPUT_ATTACHMENT_READ_BIT :					result |= EAccessType::Read;	break;
				case VK_ACCESS_SHADER_READ_BIT :							result |= EAccessType::Read;	break;
				case VK_ACCESS_SHADER_WRITE_BIT :							result |= EAccessType::Write;	break;
				case VK_ACCESS_COLOR_ATTACHMENT_READ_BIT :					result |= EAccessType::Read;	break;
				case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT :					result |= EAccessType::Write;	break;
				case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT :			result |= EAccessType::Read;	break;
				case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT :			result |= EAccessType::Write;	break;
				case VK_ACCESS_TRANSFER_READ_BIT :							result |= EAccessType::Read;	break;
				case VK_ACCESS_TRANSFER_WRITE_BIT :							result |= EAccessType::Write;	break;
				case VK_ACCESS_HOST_READ_BIT :								result |= EAccessType::Read;	break;
				case VK_ACCESS_HOST_WRITE_BIT :								result |= EAccessType::Write;	break;
				case VK_ACCESS_MEMORY_READ_BIT :							result |= EAccessType::Read;	break;
				case VK_ACCESS_MEMORY_WRITE_BIT :							result |= EAccessType::Write;	break;
				case VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT :			result |= EAccessType::Write;	break;
				case VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT :	result |= EAccessType::Read;	break;
				case VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT :	result |= EAccessType::Write;	break;
				case VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT :			result |= EAccessType::Read;	break;
				case VK_ACCESS_COMMAND_PROCESS_READ_BIT_NVX :				result |= EAccessType::Read;	break;
				case VK_ACCESS_COMMAND_PROCESS_WRITE_BIT_NVX :				result |= EAccessType::Write;	break;
				case VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT :	result |= EAccessType::Read;	break;
				case VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV :				result |= EAccessType::Read;	break;
				case VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NVX :		result |= EAccessType::Read;	break;
				case VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NVX :		result |= EAccessType::Write;	break;
				case VK_ACCESS_FLAG_BITS_MAX_ENUM :
				default :													ASSERT(false);  break;
			}
			DISABLE_ENUM_CHECKS();
		}
		return result;
	}

/*
=================================================
	GetBarrierType
=================================================
*/
	static void GetBarrierType (VkPipelineStageFlags /*srcStageMask*/, VkPipelineStageFlags /*dstStageMask*/,
								VkDependencyFlags /*dependencyFlags*/,
								VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
								VkImageLayout oldLayout, VkImageLayout newLayout,
								OUT String &outLabel, OUT RGBA8u &outColor)
	{
		const EAccessType	src_access = GetAccessType( srcAccessMask );
		const EAccessType	dst_access = GetAccessType( dstAccessMask );

		if ( oldLayout != newLayout )
		{
			outLabel = "L\\nA\\nY\\n";
			outColor = HtmlColor::Yellow;
		}
		else
		
		// write -> write
		if ( EnumEq( src_access, EAccessType::Write ) and
			 EnumEq( dst_access, EAccessType::Write ) )
		{
			outLabel = "w\\n--\\nW\\n";
			outColor = HtmlColor::DodgerBlue;
		}
		else

		// read -> write
		if ( EnumEq( src_access, EAccessType::Read ) and
			 EnumEq( dst_access, EAccessType::Write ) )
		{
			outLabel = "R\\n--\\nW\\n";
			outColor = HtmlColor::LimeGreen;
		}
		else

		// write -> read
		if ( EnumEq( src_access, EAccessType::Write ) and
			 EnumEq( dst_access, EAccessType::Read ) )
		{
			outLabel = "W\\n--\\nR\\n";
			outColor = HtmlColor::Red;
		}
		else

		// unknown
		{
			outLabel = "|\\n|\\n|\\n";
			outColor = HtmlColor::Pink;
		}
	}
	
/*
=================================================
	_GetBufferBarrier
=================================================
*/
	void VFrameGraphDebugger::_GetBufferBarrier (const VBuffer *buffer, TaskPtr task, INOUT String &style, INOUT String &deps,
												 INOUT HashSet<String> &existingBarriers) const
	{
		auto iter = _buffers.find( buffer );
		if ( iter == _buffers.end() )
			return;
		
		for (auto& bar : iter->second.barriers)
		{
			if ( bar.dstIndex != task->ExecutionOrder() )
				continue;

			ASSERT( bar.info.buffer == buffer->Handle() );
			
			_GetResourceBarrier( buffer, bar, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED,
								 INOUT style, INOUT deps, INOUT existingBarriers );
		}
	}

/*
=================================================
	_GetImageBarrier
=================================================
*/
	void VFrameGraphDebugger::_GetImageBarrier (const VImage *image, TaskPtr task, INOUT String &style, INOUT String &deps,
												INOUT HashSet<String> &existingBarriers) const
	{
		auto iter = _images.find( image );
		if ( iter == _images.end() )
			return;

		for (auto& bar : iter->second.barriers)
		{
			if ( bar.dstIndex != task->ExecutionOrder() )
				continue;

			ASSERT( bar.info.image == image->Handle() );

			_GetResourceBarrier( image, bar, bar.info.oldLayout, bar.info.newLayout,
								 INOUT style, INOUT deps, INOUT existingBarriers );
		}
	}
	
/*
=================================================
	_GetImageBarrier
=================================================
*/
	template <typename T, typename B>
	inline void VFrameGraphDebugger::_GetResourceBarrier (const T *res, const Barrier<B> &bar, VkImageLayout oldLayout, VkImageLayout newLayout,
														  INOUT String &style, INOUT String &deps, INOUT HashSet<String> &existingBarriers) const
	{
		const NodeUniqueNames	node_name;
		const ColorScheme		color_scheme;

		const String	barrier_name = node_name( res, bar.srcIndex, bar.dstIndex );
		const TaskPtr	src_task	 = _GetTask( bar.srcIndex );
		const TaskPtr	dst_task	 = _GetTask( bar.dstIndex );
		const String	edge_color1	 = color_scheme.ResourceToResourceEdge( src_task );
		const String	edge_color2	 = color_scheme.ResourceToResourceEdge( dst_task );

		// add barrier style
		if ( not existingBarriers.count( barrier_name ) )
		{
			ASSERT( bar.info.srcQueueFamilyIndex == bar.info.dstQueueFamilyIndex );

			String		label;
			RGBA8u		color;
			GetBarrierType( bar.srcStageMask, bar.dstStageMask, bar.dependencyFlags,
							bar.info.srcAccessMask, bar.info.dstAccessMask,
							oldLayout, newLayout,
							OUT label, OUT color );

			style << "\t\t" << barrier_name << " [label=\"" << label
					<< "\", width=.1, fontsize=12, fillcolor=\"#" << ColToStr( color ) << "\"];\n";

			existingBarriers.insert( barrier_name );
		}

		// add barrier dependency
		if ( EnumEq( _flags, ECompilationDebugFlags::VisBarrierLabels ) )
		{
			String	src_stage = VkPipelineStage_ToString( bar.srcStageMask );
			String	dst_stage = VkPipelineStage_ToString( bar.dstStageMask );

			FindAndReplace( INOUT src_stage, " | ", "\\n" );
			FindAndReplace( INOUT dst_stage, " | ", "\\n" );

			deps << "\t\t" << node_name( res, bar.srcIndex ) << " -> " << barrier_name
					<< " [color=\"#" << edge_color1 << "\", label=\"" << src_stage << "\", minlen=2, labelfloat=true, decorate=false];\n";

			deps << "\t\t" << barrier_name << " -> " << node_name( res, bar.dstIndex )
					<< " [color=\"#" << edge_color2 << "\", label=\"" << dst_stage << "\", minlen=2, labelfloat=true, decorate=false];\n";
		}
		else
		{
			deps << "\t\t" << node_name( res, bar.srcIndex ) << " -> " << barrier_name << " [color=\"#" << edge_color1 << "\"];\n";
			deps << "\t\t" << barrier_name << " -> " << node_name( res, bar.dstIndex ) << " [color=\"#" << edge_color2 << "\"];\n";
		}
	}


}	// FG

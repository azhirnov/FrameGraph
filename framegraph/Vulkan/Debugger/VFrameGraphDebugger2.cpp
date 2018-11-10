// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VFrameGraphDebugger.h"
#include "VEnumToString.h"
#include "Shared/EnumToString.h"
#include "VTaskGraph.h"
#include "VDebugger.h"

namespace FG
{
namespace {
	static constexpr char	indent[] = "\t\t";
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
			return ColToStr( task ? Lerp( HtmlColor::Black, task->DebugColor(), 0.5f ) : HtmlColor::Pink );
		}
	};

/*
=================================================
	BufferStateToString
=================================================
*
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
*
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
	void VFrameGraphDebugger::_DumpGraph (const CommandBatchID &batchId, uint indexInBatch, OUT String &str) const
	{
		str.clear();
		
		const NodeUniqueNames	node_name;
		const ColorScheme		color_scheme;
		HashSet<String>			existing_barriers;

		String	deps;
		String	subgraphs;

		str << indent << "subgraph cluster_SubBatch" << VDebugger::BuildSubBatchName( batchId, indexInBatch ) << " {\n"
			<< indent << "	style=filled;\n"
			<< indent << "	color=\"#282828\";\n"
			<< indent << "	fontcolor=\"#dcdcdc\";\n"
			<< indent << "	label=\"" << batchId.GetName() << " / " << ToString(indexInBatch) << "\";\n";
		
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
					deps << indent << '\t' << node_name( info.task ) << ":e -> { ";
				
					for (auto& out_node : info.task->Outputs())
					{
						deps << node_name( out_node ) << ":w; ";
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
					<< ToString( image->second.state ) << "\\n"
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
					<< ToString( buffer->second.state ) << "\\n"
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

			deps << "\t\t" << node_name( res, bar.srcIndex ) << ":e -> " << barrier_name
				 << ":w [color=\"#" << edge_color1 << "\", label=\"" << src_stage << "\", minlen=2, labelfloat=true, decorate=false];\n";

			deps << "\t\t" << barrier_name << ":e -> " << node_name( res, bar.dstIndex )
				 << ":w [color=\"#" << edge_color2 << "\", label=\"" << dst_stage << "\", minlen=2, labelfloat=true, decorate=false];\n";
		}
		else
		{
			deps << "\t\t" << node_name( res, bar.srcIndex ) << ":e -> " << barrier_name << ":w [color=\"#" << edge_color1 << "\"];\n";
			deps << "\t\t" << barrier_name << ":e -> " << node_name( res, bar.dstIndex ) << ":w [color=\"#" << edge_color2 << "\"];\n";
		}
	}

}	// FG

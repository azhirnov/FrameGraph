// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	docs: https://graphviz.gitlab.io/_pages/pdf/dotguide.pdf
*/

#include "VLocalDebugger.h"
#include "VEnumToString.h"
#include "Shared/EnumToString.h"
#include "VTaskGraph.h"
#include "stl/Containers/Iterators.h"

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
		uint	val = (uint(col.r) << 16) | (uint(col.g) << 8) | uint(col.b);
		String	str = ToString<16>( val );

		for (; str.length() < 6;) {
			str.insert( str.begin(), '0' );
		}
		return str;
	}
	
/*
=================================================
	_Vis***
=================================================
*/
	String  VLocalDebugger::_VisTaskName (VTask task) const
	{
		return "n"s << ToString<16>( uint(task ? task->ExecutionOrder() : ExeOrderIndex::Initial) ) << '_' << _subBatchUID;
	}
	
	String  VLocalDebugger::_VisBarrierGroupName (VTask task) const
	{
		return _VisBarrierGroupName( task ? task->ExecutionOrder() : ExeOrderIndex::Initial );
	}
	
	String  VLocalDebugger::_VisBarrierGroupName (ExeOrderIndex index) const
	{
		return "barGr"s << ToString<16>( uint(index) ) << '_' << _subBatchUID;
	}

	String  VLocalDebugger::_VisResourceName (const VBuffer *buffer, VTask task) const
	{
		return _VisResourceName( buffer, task ? task->ExecutionOrder() : ExeOrderIndex::Initial );
	}

	String  VLocalDebugger::_VisResourceName (const VBuffer *buffer, ExeOrderIndex index) const
	{
		return "buf"s << ToString<16>( uint64_t(buffer->Handle()) ) << '_' << ToString<16>( uint(index) ) << '_' << _subBatchUID;
	}
	
	String  VLocalDebugger::_VisResourceName (const VImage *image, VTask task) const
	{
		return _VisResourceName( image, task ? task->ExecutionOrder() : ExeOrderIndex::Initial );
	}

	String  VLocalDebugger::_VisResourceName (const VImage *image, ExeOrderIndex index) const
	{
		return "img"s << ToString<16>( uint64_t(image->Handle()) ) << '_' << ToString<16>( uint(index) ) << '_' << _subBatchUID;
	}
	
	String  VLocalDebugger::_VisBarrierName (const VBuffer *buffer, ExeOrderIndex srcIndex, ExeOrderIndex dstIndex) const
	{
			return "bufBar"s << ToString<16>( uint64_t(buffer->Handle()) )
							<< '_' << ToString<16>( uint(srcIndex) )
							<< '_' << ToString<16>( uint(dstIndex) )
							<< '_' << _subBatchUID;
	}

	String  VLocalDebugger::_VisBarrierName (const VImage *image, ExeOrderIndex srcIndex, ExeOrderIndex dstIndex) const
	{
			return "imgBuf"s << ToString<16>( uint64_t(image->Handle()) )
							<< '_' << ToString<16>( uint(srcIndex) )
							<< '_' << ToString<16>( uint(dstIndex) )
							<< '_' << _subBatchUID;
	}

/*
=================================================
	ColorScheme
=================================================
*/
	String  VLocalDebugger::_SubBatchBG () const
	{
		return ColToStr( ColorScheme::CmdSubBatchBackground );
	}

	String  VLocalDebugger::_TaskLabelColor (RGBA8u) const
	{
		return ColToStr( HtmlColor::White );
	}

	/*String  VLocalDebugger::_DrawTaskBG () const
	{
		return ColToStr( HtmlColor::Bisque );
	}
	
	String  VLocalDebugger::_DrawTaskLabelColor () const
	{
		return ColToStr( HtmlColor::Black );
	}*/
	
	String  VLocalDebugger::_ResourceBG (const VBuffer *) const
	{
		return ColToStr( ColorScheme::ResourceBackground );
	}

	String  VLocalDebugger::_ResourceBG (const VImage *) const
	{
		return ColToStr( ColorScheme::ResourceBackground );
	}
	
	String  VLocalDebugger::_ResourceToResourceEdgeColor (VTask task) const
	{
		return ColToStr( task ? task->DebugColor() : ColorScheme::ResourceBackground );
	}
	
	String  VLocalDebugger::_ResourceGroupBG (VTask task) const
	{
		return ColToStr( task ? Lerp( HtmlColor::Black, task->DebugColor(), 0.5f ) : ColorScheme::Debug );
	}
	
	String  VLocalDebugger::_BarrierGroupBorderColor () const
	{
		return ColToStr( ColorScheme::BarrierGroupBorder );
	}
	
	String  VLocalDebugger::_GroupBorderColor () const
	{
		return ColToStr( ColorScheme::GroupBorder );
	}

/*
=================================================
	GetBufferName
=================================================
*/
	ND_ static StringView  GetBufferName (const VBuffer *buffer)
	{
		return not buffer->GetDebugName().empty() ? buffer->GetDebugName() : "Buffer";
	}

/*
=================================================
	GetImageName
=================================================
*/
	ND_ static StringView  GetImageName (const VImage *image)
	{
		return not image->GetDebugName().empty() ? image->GetDebugName() : "Image";
	}

/*
=================================================
	_DumpGraph
=================================================
*/
	void VLocalDebugger::_DumpGraph (OUT BatchGraph &result) const
	{
		String&	str = result.body;

		str.clear();
		_existingNodes.clear();

		String	deps;
		String	subgraphs;

		str << indent << "subgraph cluster_SubBatch" << _subBatchUID << " {\n"
			<< indent << "	style = filled;\n"
			<< indent << "	color = \"#" << _SubBatchBG() << "\";\n"
			<< indent << "	fontcolor = \"#" << ColToStr( ColorScheme::CmdSubBatchLavel ) << "\";\n";
			//<< indent << "	label = \"" << batchId.GetName() << " / " << ToString(indexInBatch) << "\";\n";
		
		_AddInitialStates( INOUT subgraphs, OUT result.firstNode );

		for (auto& info : _tasks)
		{
			if ( not info.task )
				continue;

			// add task with resource usage
			if ( EnumEq( _flags, EDebugFlags::VisResources ) )
			{
				String	res_style;
				String	bar_style;
				_GetResourceUsage( info, OUT res_style, OUT bar_style, INOUT deps );

				if ( res_style.empty() )
				{
					info.anyNode = _VisTaskName( info.task );
					res_style << indent << "\t	" << info.anyNode << " [shape=point, style=invis];\n";
				}

				subgraphs
					<< indent << "\tsubgraph cluster_" << _VisTaskName( info.task ) << " {\n"
					<< indent << "\t	style = filled;\n"
					<< indent << "\t	rankdir = TB;\n"
					<< indent << "\t	color = \"#" << _ResourceGroupBG( info.task ) << "\";\n"
					<< indent << "\t	fontcolor = \"#" << _TaskLabelColor( info.task->DebugColor() ) << "\";\n"
					<< indent << "\t	label = \"" << info.task->Name() << "\";\n"
					<< res_style
					<< indent << "\t}\n\n";
				
				// add dependencies
				if ( EnumEq( _flags, EDebugFlags::VisTaskDependencies ) )
				{
					for (auto& in_node : info.task->Inputs())
					{
						auto&	in_info = _tasks[ uint(in_node->ExecutionOrder()) ];
						ASSERT( in_info.task == in_node );
						ASSERT( not in_info.anyNode.empty() );

						deps << indent << '\t' << in_info.anyNode << ":ne -> " << info.anyNode << ":nw"
								<< " [ltail=cluster_" << _VisTaskName( in_node ) << ", lhead=cluster_" << _VisTaskName( info.task )
								<< ", color=\"#" << ColToStr( ColorScheme::TaskDependencyEdge ) << "\", style=dotted, weight=1];\n";
					}
				}

				// group of resource barriers
				if ( bar_style.size() )
				{
					subgraphs
						<< indent << "\tsubgraph cluster_" << _VisBarrierGroupName( info.task ) << " {\n"
						<< indent << "\t	style = filled;\n"
						<< indent << "\t	pencolor = \"#" << _BarrierGroupBorderColor() << "\";\n"
						<< indent << "\t	penwidth = 1.0;\n"
						<< indent << "\t	color = \"#" << _SubBatchBG() << "\";\n"
						<< indent << "\t	label = \"\";\n"
						<< bar_style
						<< indent << "\t}\n\n";
				}
			}
			else
			// add task
			{
				const String	color = ColToStr( info.task->DebugColor() );

				// add style
				str << indent << '\t' << _VisTaskName( info.task ) << " [label=\"" << info.task->Name() << "\", fontsize=24, fillcolor=\"#" << color
					<< "\", fontcolor=\"#" << _TaskLabelColor( info.task->DebugColor() ) << "\"];\n";

				// add dependencies
				if ( EnumEq( _flags, EDebugFlags::VisTaskDependencies ) and
					 not info.task->Outputs().empty() )
				{
					deps << indent << '\t' << _VisTaskName( info.task ) << ":e -> { ";
				
					for (auto& out_node : info.task->Outputs())
					{
						deps << _VisTaskName( out_node ) << ":w; ";
					}
					deps << "} [color=\"#" << color << "\", style=bold];\n";
				}
			}
		}

		_AddFinalStates( INOUT subgraphs, INOUT deps, OUT result.lastNode );
		
		str << '\n'
			<< subgraphs
			<< deps
			<< indent << "}\n\n";
	}
	
/*
=================================================
	_AddInitialStates
=================================================
*/
	void VLocalDebugger::_AddInitialStates (INOUT String &str, OUT String &firstNode) const
	{
		firstNode.clear();

		str << indent << "\tsubgraph cluster_Initial_" << _subBatchUID << " {\n"
			<< indent << "\t	style = filled;\n"
			<< indent << "\t	pencolor = \"#" << _GroupBorderColor() << "\";\n"
			<< indent << "\t	penwidth = 1.0;\n"
			<< indent << "\t	color = \"#" << _SubBatchBG() << "\";\n"
			<< indent << "\t	label = \"Initial\";\n";

		for (auto& image : _images)
		{
			str << indent << "\t\t" << _VisResourceName( image.first, ExeOrderIndex::Initial ) << " [label=\"" << GetImageName( image.first ) << "\\n";

			if ( firstNode.empty() )
				firstNode = _VisResourceName( image.first, ExeOrderIndex::Initial );

			for (auto& bar : image.second.barriers) {
				str << VkImageLayout_ToString( bar.info.oldLayout );
				break;
			}
			str << "\", fontsize=10, fillcolor=\"#" << _ResourceBG( image.first ) << "\"];\n";
		}

		for (auto& buffer : _buffers)
		{
			str << indent << "\t\t" << _VisResourceName( buffer.first, ExeOrderIndex::Initial ) << " [label=\"" << GetBufferName( buffer.first );
			
			if ( firstNode.empty() )
				firstNode = _VisResourceName( buffer.first, ExeOrderIndex::Initial );

			for (auto& bar : buffer.second.barriers)
			{
				if ( bar.srcIndex > ExeOrderIndex::Initial )
					break;

				str << "\\n[" << ToString(BytesU{bar.info.offset}) << ", " << (bar.info.size == VK_WHOLE_SIZE ? "whole" : ToString(BytesU{bar.info.size})) << ")";
			}
			str << "\", fontsize=10, fillcolor=\"#" << _ResourceBG( buffer.first ) << "\"];\n";
		}

		str << indent << "\t}\n\n";
	}
	
/*
=================================================
	_AddFinalStates
=================================================
*/
	void VLocalDebugger::_AddFinalStates (INOUT String &str, INOUT String &deps, OUT String &lastNode) const
	{
		lastNode.clear();

		str << indent << "\tsubgraph cluster_Final_" << _subBatchUID << " {\n"
			<< indent << "\t	style = filled;\n"
			<< indent << "\t	pencolor = \"#" << _GroupBorderColor() << "\";\n"
			<< indent << "\t	penwidth = 1.0;\n"
			<< indent << "\t	color = \"#" << _SubBatchBG() << "\";\n"
			<< indent << "\t	label = \"Final\";\n";
		
		String	bar_style;

		for (auto& image : _images)
		{
			for (auto& bar : Reverse(image.second.barriers))
			{
				if ( bar.dstIndex < ExeOrderIndex::Final )
					break;

				//auto&	range = bar.info.subresourceRange;

				str << indent << "\t\t" << _VisResourceName( image.first, ExeOrderIndex::Final ) << " [label=\"" << GetImageName( image.first ) << "\\n"
					<< VkImageLayout_ToString( bar.info.newLayout ) << "\\n"
					//<< "layer: " << ToString(range.baseArrayLayer) << (range.layerCount == VK_REMAINING_ARRAY_LAYERS ? "..whole" : range.layerCount > 1 ? ".."s << ToString(range.layerCount) : "") << "\\n"
					//<< "mipmap: " << ToString(range.baseMipLevel) << (range.levelCount == VK_REMAINING_MIP_LEVELS ? "..whole" : range.levelCount > 1 ? ".."s << ToString(range.levelCount) : "")
					<< "\", fontsize=10, fillcolor=\"#" << _ResourceBG( image.first ) << "\"];\n";
				
				if ( lastNode.empty() )
					lastNode = _VisResourceName( image.first, ExeOrderIndex::Final );

				_GetResourceBarrier( image.first, bar, bar.info.oldLayout, bar.info.newLayout, INOUT bar_style, INOUT deps );
			}
		}

		for (auto& buffer : _buffers)
		{
			for (auto& bar : Reverse(buffer.second.barriers))
			{
				if ( bar.dstIndex < ExeOrderIndex::Final )
					break;

				str << indent << "\t\t" << _VisResourceName( buffer.first, ExeOrderIndex::Final ) << " [label=\"" << GetBufferName( buffer.first ) << "\\n"
					//<< "[" << ToString(BytesU{bar.info.offset}) << ", " << (bar.info.size == VK_WHOLE_SIZE ? "whole" : ToString(BytesU{bar.info.size})) << ")"
					<< "\", fontsize=10, fillcolor=\"#" << _ResourceBG( buffer.first ) << "\"];\n";
				
				if ( lastNode.empty() )
					lastNode = _VisResourceName( buffer.first, ExeOrderIndex::Final );

				_GetResourceBarrier( buffer.first, bar, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED, INOUT bar_style, INOUT deps );
			}
		}

		str << indent << "\t}\n\n";
		
		if ( bar_style.size() )
		{
			str	<< indent << "\tsubgraph cluster_" << _VisBarrierGroupName( ExeOrderIndex::Final ) << " {\n"
				<< indent << "\t	style = filled;\n"
				<< indent << "\t	pencolor = \"#" << _BarrierGroupBorderColor() << "\";\n"
				<< indent << "\t	penwidth = 1.0;\n"
				<< indent << "\t	color = \"#" << _SubBatchBG() << "\";\n"
				<< indent << "\t	label = \"\";\n"
				<< bar_style
				<< indent << "\t}\n\n";
		}
	}

/*
=================================================
	_GetResourceUsage
=================================================
*/
	void VLocalDebugger::_GetResourceUsage (const TaskInfo &info, OUT String &resStyle, OUT String &barStyle, INOUT String &deps) const
	{
		for (auto& res : info.resources)
		{
			Visit( res,
					[&] (const ImageUsage_t &image)
					{
						auto&	range = image.second.range;
						String	name  = _VisResourceName( image.first, image.second.task );

						// add image style
						resStyle
							<< indent << "\t\t" << name << " [label=\"" << GetImageName( image.first ) << "\\n"
							<< ToString( image.second.state ) << "\\n"
							//<< VkImageLayout_ToString( image.second.layout ) << "\\n"
							<< "layer: " << ToString(range.Layers().begin) << (range.IsWholeLayers() ? "..whole" : range.Layers().Count() > 1 ? ".."s << ToString(range.Layers().end) : "") << ", "
							<< "mip: " << ToString(range.Mipmaps().begin) << (range.IsWholeMipmaps() ? "..whole" : range.Mipmaps().Count() > 1 ? ".."s << ToString(range.Mipmaps().end) : "")
							<< "\", fontsize=10, fillcolor=\"#" << _ResourceBG( image.first ) << "\"];\n";

						if ( info.anyNode.empty() )
							info.anyNode = std::move(name);

						_GetImageBarrier( image.first, image.second.task, INOUT barStyle, INOUT deps );
					},

					[&] (const BufferUsage_t &buffer)
					{
						auto&	range = buffer.second.range;
						String	name  = _VisResourceName( buffer.first, buffer.second.task );

						// add buffer style
						resStyle
							<< indent << "\t\t" << name << " [label=\"" << GetBufferName( buffer.first ) << "\\n"
							<< ToString( buffer.second.state ) << "\\n"
							<< "[" << ToString(BytesU{range.begin}) << ", " << (range.IsWhole() ? "whole" : ToString(BytesU{range.end})) << ")"
							<< "\", fontsize=10, fillcolor=\"#" << _ResourceBG( buffer.first ) << "\"];\n";
				
						if ( info.anyNode.empty() )
							info.anyNode = std::move(name);

						_GetBufferBarrier( buffer.first, buffer.second.task, INOUT barStyle, INOUT deps );
					},

					[] (const RTGeometryUsage_t &geometry)
					{
					},
					[] (const RTSceneUsage_t &scene)
					{
					},
					[] (const NullUnion &) { ASSERT(false); }
				);
		}
	}
	
/*
=================================================
	_GetBufferBarrier
=================================================
*/
	void VLocalDebugger::_GetBufferBarrier (const VBuffer *buffer, VTask task, INOUT String &barStyle, INOUT String &deps) const
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
								 INOUT barStyle, INOUT deps );
		}
	}

/*
=================================================
	_GetImageBarrier
=================================================
*/
	void VLocalDebugger::_GetImageBarrier (const VImage *image, VTask task, INOUT String &barStyle, INOUT String &deps) const
	{
		auto iter = _images.find( image );
		if ( iter == _images.end() )
			return;

		for (auto& bar : iter->second.barriers)
		{
			if ( bar.dstIndex != task->ExecutionOrder() )
				continue;

			ASSERT( bar.info.image == image->Handle() );

			_GetResourceBarrier( image, bar, bar.info.oldLayout, bar.info.newLayout, INOUT barStyle, INOUT deps );
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

			BEGIN_ENUM_CHECKS();
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
				case VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV :			result |= EAccessType::Read;	break;
				case VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV :		result |= EAccessType::Write;	break;
				case VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT :			result |= EAccessType::Read;	break;
				case VK_ACCESS_FLAG_BITS_MAX_ENUM :
				default :													ASSERT(false);  break;
			}
			END_ENUM_CHECKS();
		}
		return result;
	}
	
/*
=================================================
	AddPipelineStages
----
	make pipeline barrier visualization like in
	image https://32ipi028l5q82yhj72224m8j-wpengine.netdna-ssl.com/wp-content/uploads/2016/10/vulkan-good-barrier-1024x771.png
	from https://gpuopen.com/vulkan-barriers-explained/
	or
	image https://i1.wp.com/cpp-rendering.io/wp-content/uploads/2016/11/Barrier-3.png
	from http://cpp-rendering.io/barriers-vulkan-not-difficult/

	TODO:
	- graphics, compute, transfer, raytracing stages are not intersects, so ... ?
=================================================
*/
	static void  AddPipelineStages (VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
									StringView barrierStyle, INOUT String &style)
	{
		static constexpr VkPipelineStageFlags	all_graphics =
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV |
			VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
			VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
			VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT | VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT | VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV;

		static const VkPipelineStageFlagBits	all_stages[] = {
			// all stages
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			// graphics only
			VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,
			VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,
			VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV,
			VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV,
			VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			// compute only
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			// transfer only
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			// ray tracing & AS update
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
			// all stages
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			//VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT,
			//VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT,
			//VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX,
		};

		auto	TestStage = [] (VkPipelineStageFlags stages, size_t i)
							{
								return	EnumEq( stages, all_stages[i] ) or
										(EnumEq( stages, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT ) and EnumEq( all_graphics, all_stages[i] )) or
										EnumEq( stages, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT );
							};

		// colors for empty, exists and skip cells
		const String	cell_color[] = { ColToStr( HtmlColor::Gray ), ColToStr( HtmlColor::Red ), ColToStr( HtmlColor::LimeGreen ) };
		const char		space[]		 = "  ";

		bool	src_bits [CountOf(all_stages)] = {};
		bool	dst_bits [CountOf(all_stages)] = {};

		size_t	last_src_cell	= 0;
		size_t	first_dst_cell	= UMax;

		for (size_t i = 0; i < CountOf(all_stages); ++i)
		{
			src_bits[i] = TestStage( srcStageMask, i );
			dst_bits[i] = TestStage( dstStageMask, i );

			if ( src_bits[i] )
				last_src_cell  = Max( last_src_cell,  i );

			if ( dst_bits[i] )
				first_dst_cell = Min( first_dst_cell, i );
		}
		
		for (size_t i = 0; i < CountOf(all_stages); ++i)
		{
			uint	src = src_bits[i] ? 1 : (i < last_src_cell ? 0 : 2);
			uint	dst = dst_bits[i] ? 1 : (i < first_dst_cell ? 2 : 0);

			style << "<TR><TD bgcolor=\"#" << cell_color[src] << "\">" << space << "</TD>";

			if ( i == 0 ) {
				style << "<TD ROWSPAN=\"" << ToString( CountOf(all_stages) )
					  << "\" bgcolor=\"#000000\">"
					  << barrierStyle << "</TD>";
			}
			style << "<TD bgcolor=\"#" << cell_color[dst] << "\">" << space << "</TD></TR>";
		}
	}

/*
=================================================
	BuildBarrierStyle
=================================================
*/
	static void BuildBarrierStyle (VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
								   VkDependencyFlags /*dependencyFlags*/,
								   VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
								   VkImageLayout oldLayout, VkImageLayout newLayout,
								   INOUT String &style)
	{
		const EAccessType	src_access = GetAccessType( srcAccessMask );
		const EAccessType	dst_access = GetAccessType( dstAccessMask );

		// new style
		if constexpr ( not ColorScheme::EnabledOldStyleBarriers )
		{
			String	label = "<FONT COLOR=\"#fefeee\" POINT-SIZE=\"8\">";

			if ( oldLayout != newLayout )
			{
				label << " L <BR/> A <BR/> Y ";
			}
			else
		
			// write -> write
			if ( EnumEq( src_access, EAccessType::Write ) and
				 EnumEq( dst_access, EAccessType::Write ) )
			{
				label << " W <BR/>--<BR/> W ";
			}
			else

			// read -> write
			if ( EnumEq( dst_access, EAccessType::Write ) )
			{
				label << " R <BR/>--<BR/> W ";
			}
			else

			// write -> read
			if ( EnumEq( src_access, EAccessType::Write ) )
			{
				label << " W <BR/>--<BR/> R ";
			}
			else

			// unknown
			{
				label << " ? <BR/> ? <BR/> ? ";
			}

			label << "</FONT>";
			style << " [shape=none, width=.1, margin=0, fontsize=2, label=<<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"0\">\n";
	
			AddPipelineStages( srcStageMask, dstStageMask, label, INOUT style );
			
			style << "</TABLE>>];\n";

		}
		// old style
		else
		{
			String		label;
			RGBA8u		color;

			if ( oldLayout != newLayout )
			{
				label = "L\\nA\\nY\\n";
				color = ColorScheme::LayoutBarrier;
			}
			else
		
			// write -> write
			if ( EnumEq( src_access, EAccessType::Write ) and
				 EnumEq( dst_access, EAccessType::Write ) )
			{
				label = "w\\n--\\nW\\n";
				color = ColorScheme::WriteAfterWriteBarrier;
			}
			else

			// read -> write
			if ( EnumEq( src_access, EAccessType::Read ) and
				 EnumEq( dst_access, EAccessType::Write ) )
			{
				label = "R\\n--\\nW\\n";
				color = ColorScheme::WriteAfterReadBarrier;
			}
			else

			// write -> read
			if ( EnumEq( src_access, EAccessType::Write ) and
				 EnumEq( dst_access, EAccessType::Read ) )
			{
				label = "W\\n--\\nR\\n";
				color = ColorScheme::ReadAfterWriteBarrier;
			}
			else

			// unknown
			{
				label = "|\\n|\\n|\\n";
				color = ColorScheme::Debug;
			}

			style << " [label=\"" << label << "\", width=.1, fontsize=12, fillcolor=\"#" << ColToStr( color ) << "\"];\n";
		}
	}

/*
=================================================
	_GetResourceBarrier
=================================================
*/
	template <typename T, typename B>
	inline void VLocalDebugger::_GetResourceBarrier (const T *res, const Barrier<B> &bar, VkImageLayout oldLayout, VkImageLayout newLayout,
														  INOUT String &style, INOUT String &deps) const
	{
		const String	barrier_name = _VisBarrierName( res, bar.srcIndex, bar.dstIndex );
		const VTask		src_task	 = _GetTask( bar.srcIndex );
		const VTask		dst_task	 = _GetTask( bar.dstIndex );
		const String	edge_color1	 = _ResourceToResourceEdgeColor( src_task );
		const String	edge_color2	 = _ResourceToResourceEdgeColor( dst_task );

		// add barrier style
		if ( not _existingNodes.count( barrier_name ) )
		{
			ASSERT( bar.info.srcQueueFamilyIndex == bar.info.dstQueueFamilyIndex );	// not supported yet

			style << indent << '\t' << barrier_name;

			BuildBarrierStyle( bar.srcStageMask, bar.dstStageMask, bar.dependencyFlags,
							   bar.info.srcAccessMask, bar.info.dstAccessMask,
							   oldLayout, newLayout,
							   INOUT style );

			_existingNodes.insert( barrier_name );
		}

		// add barrier dependency
		if ( EnumEq( _flags, EDebugFlags::VisBarrierLabels ) )
		{
			String	src_stage = VkPipelineStage_ToString( bar.srcStageMask );
			String	dst_stage = VkPipelineStage_ToString( bar.dstStageMask );

			FindAndReplace( INOUT src_stage, " | ", "\\n" );
			FindAndReplace( INOUT dst_stage, " | ", "\\n" );

			deps << indent << '\t' << _VisResourceName( res, bar.srcIndex ) << ":e -> " << barrier_name
				 << ":w [color=\"#" << edge_color1 << "\", label=\"" << src_stage << "\", minlen=2, labelfloat=true, decorate=false, weight=2];\n";

			deps << indent << '\t' << barrier_name << ":e -> " << _VisResourceName( res, bar.dstIndex )
				 << ":w [color=\"#" << edge_color2 << "\", label=\"" << dst_stage << "\", minlen=2, labelfloat=true, decorate=false, weight=2];\n";
		}
		else
		{
			deps << indent << '\t' << _VisResourceName( res, bar.srcIndex ) << ":e -> " << barrier_name << ":w [color=\"#" << edge_color1 << "\", weight=2];\n";
			deps << indent << '\t' << barrier_name << ":e -> " << _VisResourceName( res, bar.dstIndex ) << ":w [color=\"#" << edge_color2 << "\", weight=2];\n";
		}
	}

}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VDebugger.h"
#include "stl/Algorithms/StringUtils.h"
#include "Public/ColorScheme.h"

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	VDebugger::VDebugger ()
	{
	}
	
/*
=================================================
	AddBatchDump
=================================================
*/
	void VDebugger::AddBatchDump (StringView value)
	{
		_fullDump.append( value );
	}

/*
=================================================
	GetFrameDump
=================================================
*/
	void VDebugger::GetFrameDump (OUT String &str) const
	{
		std::swap( str, _fullDump );
	}
	
/*
=================================================
	ColToStr
=================================================
*
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
	GetGraphDump
=================================================
*/
	void VDebugger::GetGraphDump (OUT String &str) const
	{
		/*str.clear();
		str	<< "digraph FrameGraph {\n"
			<< "	rankdir = LR;\n"
			<< "	bgcolor = black;\n"
			<< "	labelloc = top;\n"
			//<< "	concentrate = true;\n"
			<< "	compound = true;\n"
			<< "	node [shape=rectangle, margin=\"0.1,0.1\" fontname=\"helvetica\", style=filled, layer=all, penwidth=0.0];\n"
			<< "	edge [fontname=\"helvetica\", fontsize=8, fontcolor=white, layer=all];\n\n";
		
		// TODO: sort?
		for (auto& batch : _batches)
		{
			str << "	subgraph cluster_Batch" << ToString<16>( size_t(&batch) ) << " {\n"
				<< "		style = filled;\n"
				<< "		color = \"#" << ColToStr( ColorScheme::CmdBatchBackground ) << "\";\n"
				<< "		fontcolor = \"#" << ColToStr( ColorScheme::CmdBatchLabel ) << "\";\n"
				<< "		label = \"" << batch.first.GetName() << "\";\n";

			for (uint i = 0; i < batch.second.threadCount; ++i)
			{
				auto&	sub_batch = batch.second.subBatches[i];
				EXLOCK( sub_batch.rcCheck );

				str << sub_batch.graph.body;
			}

			str << "	}\n\n";
		}
		str << "}\n";*/
	}

}	// FG

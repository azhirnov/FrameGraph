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
		_fullDump.reserve( 8 );
		_graphs.reserve( 8 );
	}
	
/*
=================================================
	AddBatchDump
=================================================
*/
	void VDebugger::AddBatchDump (String &&value)
	{
		_fullDump.push_back( std::move(value) );
	}

/*
=================================================
	GetFrameDump
=================================================
*/
	void VDebugger::GetFrameDump (OUT String &str) const
	{
		for (auto& item : _fullDump) {
			str << item;
		}
		_fullDump.clear();
	}
	
/*
=================================================
	AddBatchGraph
=================================================
*/
	void VDebugger::AddBatchGraph (BatchGraph &&value)
	{
		_graphs.push_back( std::move(value) );
	}

/*
=================================================
	GetGraphDump
=================================================
*/
	void VDebugger::GetGraphDump (OUT String &str) const
	{
		str.clear();
		str	<< "digraph FrameGraph {\n"
			<< "	rankdir = LR;\n"
			<< "	bgcolor = black;\n"
			<< "	labelloc = top;\n"
			//<< "	concentrate = true;\n"
			<< "	compound = true;\n"
			<< "	node [shape=rectangle, margin=\"0.1,0.1\" fontname=\"helvetica\", style=filled, layer=all, penwidth=0.0];\n"
			<< "	edge [fontname=\"helvetica\", fontsize=8, fontcolor=white, layer=all];\n\n";
		
		for (auto& item : _graphs)
		{
			str << item.body;
		}
		str << "}\n";
	}

}	// FG

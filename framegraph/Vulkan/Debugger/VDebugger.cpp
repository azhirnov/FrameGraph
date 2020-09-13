// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

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
		EXLOCK( _drCheck );

		_fullDump.reserve( 8 );
		_graphs.reserve( 8 );
	}
	
/*
=================================================
	AddBatchDump
=================================================
*/
	void VDebugger::AddBatchDump (StringView name, String &&value)
	{
		EXLOCK( _drCheck );

		if ( value.size() )
			_fullDump.emplace_back( name, std::move(value) );
	}

/*
=================================================
	GetFrameDump
=================================================
*/
	void VDebugger::GetFrameDump (OUT String &str) const
	{
		EXLOCK( _drCheck );

		std::sort( _fullDump.begin(), _fullDump.end(), [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });

		for (auto& [name, item] : _fullDump) {
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
		EXLOCK( _drCheck );

		if ( value.body.size() )
			_graphs.push_back( std::move(value) );
	}

/*
=================================================
	GetGraphDump
=================================================
*/
	void VDebugger::GetGraphDump (OUT String &str) const
	{
		EXLOCK( _drCheck );

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

		_graphs.clear();
	}

}	// FG

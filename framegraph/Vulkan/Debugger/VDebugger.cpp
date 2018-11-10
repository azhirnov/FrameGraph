// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "VDebugger.h"
#include "stl/Algorithms/StringUtils.h"

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
	OnBeginFrame
=================================================
*/
	void VDebugger::OnBeginFrame (const SubmissionGraph &graph)
	{
		_batches.clear();
		
		// build graph
		for (auto& src : graph.Batches())
		{
			auto	inserted = _batches.insert({ src.first, Batch{} });
			CHECK_ERR( inserted.second, void());
			ASSERT( src.second.threadCount <= MaxSubBatches );

			Batch&	dst = inserted.first->second;
			dst.threadCount = src.second.threadCount;

			for (auto& dep : src.second.dependsOn)
			{
				auto	iter = _batches.find( dep );
				CHECK_ERR( iter != _batches.end(), void());

				dst.input.push_back( &iter->second );
				iter->second.output.push_back( &dst );
			}
		}
	}
	
/*
=================================================
	OnEndFrame
=================================================
*/
	void VDebugger::OnEndFrame ()
	{
	}
	
/*
=================================================
	AddFrameDump
=================================================
*/
	void VDebugger::AddFrameDump (const CommandBatchID &batchId, uint indexInBatch, INOUT String &str) const
	{
		auto	iter = _batches.find( batchId );
		ASSERT( iter != _batches.end() );

		if ( iter != _batches.end() )
		{
			ASSERT( iter->second.dumps[indexInBatch].empty() );
			std::swap( iter->second.dumps[indexInBatch], str );
		}
	}

/*
=================================================
	GetFrameDump
=================================================
*/
	void VDebugger::GetFrameDump (OUT String &str) const
	{
		str.clear();

		// TODO: sort?
		for (auto& batch : _batches)
		{
			for (uint i = 0; i < batch.second.threadCount; ++i)
			{
				ASSERT( not batch.second.dumps[i].empty() );

				str << batch.second.dumps[i];
			}
		}
	}
	
/*
=================================================
	AddGraphDump
=================================================
*/
	void VDebugger::AddGraphDump (const CommandBatchID &batchId, uint indexInBatch, INOUT String &str) const
	{
		auto	iter = _batches.find( batchId );
		ASSERT( iter != _batches.end() );

		if ( iter != _batches.end() )
		{
			ASSERT( iter->second.graphs[indexInBatch].empty() );
			std::swap( iter->second.graphs[indexInBatch], str );
		}
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
			<< "	node [shape=rectangle, fontname=\"helvetica\", style=filled, layer=all, penwidth=0.0];\n"
			<< "	edge [fontname=\"helvetica\", fontsize=8, fontcolor=white, layer=all];\n"
			//<< "	concentrate=true;\n"
			<< "	compound=true;\n\n";
		
		// TODO: sort?
		for (auto& batch : _batches)
		{
			str << "	subgraph cluster_Batch" << ToString<16>( size_t(&batch) ) << " {\n"
				<< "		style=filled;\n"
				<< "		color=\"#181818\";\n"
				<< "		fontcolor=\"#dcdcdc\";\n"
				<< "		label=\"" << batch.first.GetName() << "\";\n";

			for (uint i = 0; i < batch.second.threadCount; ++i)
			{
				ASSERT( not batch.second.graphs[i].empty() );

				str << batch.second.graphs[i];
			}

			str << "	}\n";
		}
		str << "}\n";
	}
	
/*
=================================================
	BuildSubBatchName
=================================================
*/
	String  VDebugger::BuildSubBatchName (const CommandBatchID &batchId, uint indexInBatch)
	{
		String	result {batchId.GetName()};

		FindAndReplace( INOUT result, " ", "_" );
		result << "_Index" << ToString( indexInBatch );

		return result;
	}

}	// FG

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
		
		uint	uid = 0;

		// build graph
		for (auto& src : graph.Batches())
		{
			auto	inserted = _batches.insert({ src.first, Batch{} });
			CHECK_ERR( inserted.second, void());
			ASSERT( src.second.threadCount <= MaxSubBatches );

			Batch&	dst = inserted.first->second;
			dst.threadCount = src.second.threadCount;
		
			for (size_t i = 0; i < dst.threadCount; ++i, ++uid)
			{
				dst.subBatches[i].name = StringView{"sb"s << ToString(uid)};
			}

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
	GetSubBatchInfo
=================================================
*/
	void VDebugger::GetSubBatchInfo (const CommandBatchID &batchId, uint indexInBatch,
									 OUT StringView &uid, OUT String* &dump, OUT String* &graph) const
	{
		auto	iter = _batches.find( batchId );
		ASSERT( iter != _batches.end() );

		if ( iter != _batches.end() )
		{
			auto&	sub_batch = iter->second.subBatches[indexInBatch];
			SCOPELOCK( sub_batch.rcCheck );

			ASSERT( sub_batch.dump.empty() );
			ASSERT( sub_batch.graph.empty() );

			uid   = sub_batch.name;
			dump  = &sub_batch.dump;
			graph = &sub_batch.graph;
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
				auto&	sub_batch = batch.second.subBatches[i];
				SCOPELOCK( sub_batch.rcCheck );

				ASSERT( not sub_batch.dump.empty() );
				str << sub_batch.dump;
			}
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
				auto&	sub_batch = batch.second.subBatches[i];
				SCOPELOCK( sub_batch.rcCheck );

				ASSERT( not sub_batch.graph.empty() );
				str << sub_batch.graph;
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

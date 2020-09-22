// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VLocalDebugger.h"

namespace FG
{

	//
	// Debugger
	//

	class VDebugger
	{
	// types
	private:
		using BatchGraph	= VLocalDebugger::BatchGraph;


	// variables
	private:
		mutable Array<Pair< String, String >>	_fullDump;
		mutable Array<BatchGraph>				_graphs;
		
		DataRaceCheck							_drCheck;


	// methods
	public:
		VDebugger ();

		void AddBatchDump (StringView name, String &&value);
		void GetFrameDump (OUT String &) const;

		void AddBatchGraph (BatchGraph &&);
		void GetGraphDump (OUT String &) const;
	};


}	// FG

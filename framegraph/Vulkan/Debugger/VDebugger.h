// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
		mutable Array<String>		_fullDump;
		mutable Array<BatchGraph>	_graphs;


	// methods
	public:
		VDebugger ();

		void AddBatchDump (String &&);
		void GetFrameDump (OUT String &) const;

		void AddBatchGraph (BatchGraph &&);
		void GetGraphDump (OUT String &) const;
	};


}	// FG

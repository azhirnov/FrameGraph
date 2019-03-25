// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "VCommon.h"

namespace FG
{

	//
	// Debugger
	//

	class VDebugger
	{
	// variables
	private:
		mutable String		_fullDump;


	// methods
	public:
		VDebugger ();

		void AddBatchDump (StringView);
		void GetFrameDump (OUT String &) const;

		void GetGraphDump (OUT String &) const;
	};


}	// FG

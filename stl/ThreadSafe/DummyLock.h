// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"
#include <mutex>	// for lock_guard

namespace FG
{

	//
	// Dummy Lock
	//

	struct DummyLock
	{
		void lock ()	{}
		void unlock ()	{}
	};


}	// FG

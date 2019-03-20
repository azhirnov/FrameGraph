// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"
#include <mutex>	// for lock_guard

namespace FGC
{

	//
	// Dummy Lock
	//

	struct DummyLock
	{
		void lock ()	{}
		void unlock ()	{}
	};



	//
	// Dummy Shared Lock
	//

	struct DummySharedLock
	{
		void lock ()			{}
		void unlock ()			{}

		void lock_shared ()		{}
		void unlock_shared () 	{}
	};


}	// FGC

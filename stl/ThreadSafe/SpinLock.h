// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"
#include <atomic>
#include <mutex>
#include <thread>

namespace FGC
{


	//
	// Spin Lock
	//

	struct SpinLock
	{
	// variables
	private:
		alignas(FG_CACHE_LINE) std::atomic_flag		_flag;


	// methods
	public:
		SpinLock () : _flag{ATOMIC_FLAG_INIT}
		{}

		ND_ forceinline bool try_lock ()
		{
			return not _flag.test_and_set( memory_order_acquire );
		}


		// for std::lock_guard
		forceinline void lock ()
		{
			for (uint i = 0; _flag.test_and_set( memory_order_acquire ); ++i)
			{
				if ( i > 100 ) {
					i = 0;
					std::this_thread::yield();
				}
			}
		}

		forceinline void unlock ()
		{
			_flag.clear( memory_order_release );
		}
	};


}	// FGC

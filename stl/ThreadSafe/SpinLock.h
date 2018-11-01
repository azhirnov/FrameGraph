// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"
#include <atomic>
#include <mutex>
#include <thread>

namespace FG
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


		// for std::lock_guard
		forceinline void lock ()
		{
			for (uint i = 0; _flag.test_and_set( std::memory_order_acquire ); ++i)
			{
				if ( i > 100 ) {
					i = 0;
					std::this_thread::yield();
				}
			}
		}

		forceinline void unlock ()
		{
			_flag.clear( std::memory_order_release );
		}
	};


}	// FG

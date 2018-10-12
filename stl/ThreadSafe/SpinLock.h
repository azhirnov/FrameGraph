// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"
#include <atomic>
#include <mutex>

namespace FG
{


	//
	// Spin Lock
	//

	struct SpinLock
	{
	// variables
	private:
		mutable std::atomic_flag	_flag;


	// methods
	public:
		SpinLock () : _flag{ATOMIC_FLAG_INIT}
		{}


		// for std::lock_guard
		void lock () const
		{
			while ( _flag.test_and_set( std::memory_order_acquire ) )
			{}
		}

		void unlock () const
		{
			_flag.clear( std::memory_order_release );
		}
	};


}	// FG

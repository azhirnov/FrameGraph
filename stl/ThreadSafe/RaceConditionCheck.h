// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"
#include <atomic>
#include <mutex>

namespace FG
{
#ifdef FG_DEBUG


	//
	// Race Condition Check
	//

	struct RaceConditionCheck
	{
	// variables
	private:
		mutable std::atomic_flag		_flag;


	// methods
	public:
		RaceConditionCheck () : _flag{ATOMIC_FLAG_INIT}
		{}


		// for std::lock_guard
		void lock () const
		{
			CHECK( not _flag.test_and_set() );
		}

		void unlock () const
		{
			_flag.clear();
		}
	};

#else


	//
	// Race Condition Check
	//

	struct RaceConditionCheck
	{
		// for std::lock_guard
		void lock ()	{}
		void unlock ()	{}
	};

#endif	// FG_DEBUG

}	// FG

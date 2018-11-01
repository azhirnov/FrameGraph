// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"
#include <atomic>
#include <mutex>
#include <thread>

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
		mutable std::atomic<size_t>		_state { 0 };
		mutable bool					_locked {false};


	// methods
	public:
		RaceConditionCheck () {}

		// for std::lock_guard
		void lock () const
		{
			const size_t	id		= size_t(HashOf( std::this_thread::get_id() ));
			size_t			curr	= _state.load();
			
			if ( curr == id )
				return;	// recursive lock

			curr	= 0;
			_locked	= _state.compare_exchange_strong( INOUT curr, id );
			CHECK( curr == 0 );		// locked by other thread - race condition detected!
			CHECK( _locked );
		}

		void unlock () const
		{
			if ( _locked )
			{
				_state.store( 0 );
				_locked = false;
			}
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

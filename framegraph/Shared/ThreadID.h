// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include <atomic>
#include <thread>
#include "framegraph/Public/Types.h"

namespace FG
{

	//
	// Thread ID
	//

	struct ThreadID
	{
	// variables
	private:
		std::atomic<size_t>		_value;

	// methods
	public:
		ThreadID ()
		{
			SetCurrent();
		}

		ThreadID (const ThreadID &) = delete;
		ThreadID (ThreadID &&) = delete;

		ThreadID& operator = (const ThreadID &) = delete;
		ThreadID& operator = (ThreadID &&) = delete;


		ND_ bool operator == (const ThreadID &rhs) const
		{
			return _value.load( memory_order_acquire ) == rhs._value.load( memory_order_acquire );
		}

		ND_ bool IsCurrent () const
		{
			return _value.load( memory_order_acquire ) == _GetThreadId();
		}

		void SetCurrent ()
		{
			_value.store( _GetThreadId(), memory_order_release );
		}


	private:
		ND_ static size_t  _GetThreadId ()
		{
			return std::hash<std::thread::id>()( std::this_thread::get_id() );
		}
	};


}	// FG

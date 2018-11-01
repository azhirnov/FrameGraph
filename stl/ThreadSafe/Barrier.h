// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	FG_BARRIER_MODE:
		0 - WinAPI native barrier implementation, requires Windows 8 desctop.
		1 - implementation based only on atomics, requires another barrier between 'wait()'.
		2 - implementation based on boost::fibers::barrier, shows same performance as native WinAPI barrier.
		3 - wrapper around std::barrier, requires C++20.
*/

#pragma once

#include "stl/Common.h"

#ifdef PLATFORM_WINDOWS
#	define FG_BARRIER_MODE	0
#else
#	define FG_BARRIER_MODE	2
#endif


#if (FG_BARRIER_MODE == 0)

namespace FG
{

	//
	// Barrier (requires Windows 8)
	//

	struct Barrier
	{
	// types
	private:
		static constexpr uint	StorageSize = 32;


	// variables
	private:
		alignas(uint64_t) uint8_t	_data [StorageSize];


	// methods
	public:
		explicit Barrier (uint numThreads);
		~Barrier ();

		Barrier (Barrier &&) = delete;
		Barrier (const Barrier &) = delete;

		Barrier& operator = (const Barrier &) = delete;
		Barrier& operator = (Barrier &&) = delete;

		void wait ();
	};

}	// FG


#elif (FG_BARRIER_MODE == 1)

#include <atomic>
#include <thread>

namespace FG
{

	//
	// Barrier
	//

	struct Barrier
	{
		STATIC_ASSERT( std::atomic<uint>::is_always_lock_free );

	// variables
	private:
		alignas(FG_CACHE_LINE) std::atomic<uint>	_counter;
		const uint									_numThreads;


	// methods
	public:
		explicit Barrier (uint numThreads) :
			_counter{0}, _numThreads{numThreads}
		{
			ASSERT( numThreads > 0 );
		}

		~Barrier ()
		{
			ASSERT( _counter.load() == 0 );
		}

		Barrier (Barrier &&) = delete;
		Barrier (const Barrier &) = delete;

		Barrier& operator = (const Barrier &) = delete;
		Barrier& operator = (Barrier &&) = delete;

		void wait ()
		{
			_counter.fetch_add( 1u, std::memory_order_relaxed );

			const uint	max_count	= _numThreads;
			uint		expected	= max_count;

			for (uint i = 0;
				 not _counter.compare_exchange_weak( INOUT expected, 0, std::memory_order_release, std::memory_order_relaxed ) and (expected != 0);
				 ++i)
			{
				expected = max_count;

				if ( i > 100 ) {
					i = 0;
					std::this_thread::yield();
				}
			}
		}
	};

}	// FG


#elif (FG_BARRIER_MODE == 2)

#include <mutex>
#include <condition_variable>

namespace FG
{

	//
	// Barrier (based on boost::fibers::barrier)
	//

	struct Barrier
	{
	// variables
	private:
		uint					_value;
		uint					_cycle;
		const uint				_numThreads;
		std::mutex				_mutex;
		std::condition_variable	_cv;


	// methods
	public:
		explicit Barrier (uint numThreads) :
			_value{numThreads}, _cycle{0}, _numThreads{numThreads}
		{
			ASSERT( numThreads > 0 );
		}

		~Barrier ()
		{}

		Barrier (Barrier &&) = delete;
		Barrier (const Barrier &) = delete;

		Barrier& operator = (const Barrier &) = delete;
		Barrier& operator = (Barrier &&) = delete;

		void wait ()
		{
			std::unique_lock	lock{ _mutex };

			if ( (--_value) == 0 )
			{
				++_cycle;
				_value = _numThreads;

				lock.unlock();
				_cv.notify_all();
				return;
			}

			_cv.wait( lock, [this, cycle = _cycle] () { return cycle != _cycle; });
		}
	};

}	// FG


#elif (FG_BARRIER_MODE == 3)

// TODO
//#include <barrier>
//#include <experimental/barrier>

namespace FG
{

	//
	// Barrier (wraps std::barrier or std::experimental::barrier)
	//

	struct Barrier
	{
	// variables
	private:
		std::barrier	_barrier;


	// methods
	public:
		explicit Barrier (uint numThreads) : _barrier{numThreads}
		{}

		Barrier (Barrier &&) = delete;
		Barrier (const Barrier &) = delete;

		Barrier& operator = (const Barrier &) = delete;
		Barrier& operator = (Barrier &&) = delete;

		void wait ()
		{
			_barrier.arrive_and_wait();
		}
	};

}	// FG

#else
#	error not supported!

#endif	// FG_BARRIER_MODE

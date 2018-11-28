// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Containers/IndexPools.h"
#include "stl/ThreadSafe/SpinLock.h"
#include "PerfTestCommon.h"
#include <shared_mutex>


template <typename LockType, uint NumThreads, typename ResourcesT>
static void IndexedPoolAsyncPerfTest_Impl (ResourcesT &resources, const uint count, StringView msg)
{
	using Index_t = typename ResourcesT::value_type::Index_t;

	StaticArray< Pair<std::thread, Duration_t>, NumThreads >	threads;
	
	for (uint i = 0; i < threads.size(); ++i)
	{
		threads[i].first = std::thread([&resources, count, uid = i, dt = &threads[i].second] ()
		{
			auto	t_start = TimePoint_t::clock::now();

			for (uint j = 0; j < count; ++j)
			{
				uint		idx = (j + uid) % resources.size();
				Index_t		index;

				TEST( resources[idx].Assign( OUT index ));
			}

			*dt = TimePoint_t::clock::now() - t_start;
		});
	}
	
	Duration_t	dt_sum {0};

	for (auto& thread : threads)
	{
		thread.first.join();
		dt_sum += thread.second;
	}

	FG_LOGI( "dt: "s << ToString( dt_sum / NumThreads ) << ", mode: " << msg );
}


template <typename LockType, uint NumThreads, typename ResourcesT>
static void IndexedPoolAsyncPerfTest2_Impl (ResourcesT &resources, const uint count, StringView msg)
{
	using Index_t = typename ResourcesT::value_type::Index_t;

	StaticArray< Pair<std::thread, Duration_t>, NumThreads >	threads;
	
	for (uint i = 0; i < threads.size(); ++i)
	{
		threads[i].first = std::thread([&resources, count, uid = i, dt = &threads[i].second] ()
		{
			auto	t_start = TimePoint_t::clock::now();

			for (uint j = 0; j < count; ++j)
			{
				Index_t		index;
				TEST( resources[uid].Assign( OUT index ));
			}

			*dt = TimePoint_t::clock::now() - t_start;
		});
	}
	
	Duration_t	dt_sum {0};

	for (auto& thread : threads)
	{
		thread.first.join();
		dt_sum += thread.second;
	}

	FG_LOGI( "dt: "s << ToString( dt_sum / NumThreads ) << ", mode: " << msg );
}


template <uint Count, uint Mode, typename LockType, uint NumThreads>
static void IndexedPoolAsyncPerfTest (StringView msg)
{
	if constexpr ( Mode == 0 )
	{
		StaticArray< ChunkedIndexedPool1< StaticString<64>, uint64_t, UntypedAlignedAllocator, LockType >, 8 >	resources;
		for (auto& res : resources) {
			std::remove_reference_t<decltype(res)>  new_res{ 1u<<14 };
			res.Swap( new_res );
		}
		IndexedPoolAsyncPerfTest_Impl< LockType, NumThreads >( resources, Count, msg );
	}

	if constexpr ( Mode == 1 )
	{
		StaticArray< ChunkedIndexedPool2< StaticString<64>, uint64_t, (1u << 14), UntypedAlignedAllocator, LockType >, 8 >	resources;

		IndexedPoolAsyncPerfTest_Impl< LockType, NumThreads >( resources, Count, msg );
	}

	if constexpr ( Mode == 2 )
	{
		StaticArray< ChunkedIndexedPool3< StaticString<64>, uint64_t, Count/4, 16, UntypedAlignedAllocator, LockType, AtomicPtr >, 8 >	resources;

		IndexedPoolAsyncPerfTest_Impl< LockType, NumThreads >( resources, Count, msg );
	}
}


template <uint Count, uint Mode, typename LockType, uint NumThreads>
static void IndexedPoolAsyncPerfTest2 (StringView msg)
{
	if constexpr ( Mode == 0 )
	{
		StaticArray< ChunkedIndexedPool1< StaticString<64>, uint64_t, UntypedAlignedAllocator, LockType >, NumThreads >	resources;
		for (auto& res : resources) {
			std::remove_reference_t<decltype(res)>  new_res{ 1u<<14 };
			res.Swap( new_res );
		}
		IndexedPoolAsyncPerfTest2_Impl< LockType, NumThreads >( resources, Count, msg );
	}
	
	if constexpr ( Mode == 1 )
	{
		StaticArray< ChunkedIndexedPool2< StaticString<64>, uint64_t, (1u << 14), UntypedAlignedAllocator, LockType >, NumThreads >		resources;

		IndexedPoolAsyncPerfTest2_Impl< LockType, NumThreads >( resources, Count, msg );
	}
	
	if constexpr ( Mode == 2 )
	{
		StaticArray< ChunkedIndexedPool3< StaticString<64>, uint64_t, Count/4, 16, UntypedAlignedAllocator, LockType, AtomicPtr >, NumThreads >		resources;

		IndexedPoolAsyncPerfTest2_Impl< LockType, NumThreads >( resources, Count, msg );
	}
}


extern void PerformanceTest_IndexPoolMT ()
{
#ifdef FG_DEBUG
	const uint	count = 100'000;
#else
	const uint	count = 2'000'000;
#endif

	IndexedPoolAsyncPerfTest< count, 0, SpinLock, 12 >( "1 - SpinLock - IndexPool_1" );
	IndexedPoolAsyncPerfTest< count, 0, std::shared_mutex, 12 >( "1 - shared_mutex - IndexPool_1" );
	IndexedPoolAsyncPerfTest< count, 0, std::mutex, 12 >( "1 - mutex - IndexPool_1" );
	IndexedPoolAsyncPerfTest< count, 0, std::recursive_mutex, 12 >( "1 - recursive_mutex - IndexPool_1" );
	
	IndexedPoolAsyncPerfTest< count, 1, SpinLock, 12 >( "1 - SpinLock - IndexPool_2" );
	IndexedPoolAsyncPerfTest< count, 1, std::shared_mutex, 12 >( "1 - shared_mutex - IndexPool_2" );
	IndexedPoolAsyncPerfTest< count, 1, std::mutex, 12 >( "1 - mutex - IndexPool_2" );
	IndexedPoolAsyncPerfTest< count, 1, std::recursive_mutex, 12 >( "1 - recursive_mutex - IndexPool_2" );
	
	IndexedPoolAsyncPerfTest< count, 2, SpinLock, 12 >( "1 - SpinLock - IndexPool_3" );
	IndexedPoolAsyncPerfTest< count, 2, std::shared_mutex, 12 >( "1 - shared_mutex - IndexPool_3" );
	IndexedPoolAsyncPerfTest< count, 2, std::mutex, 12 >( "1 - mutex - IndexPool_3" );
	IndexedPoolAsyncPerfTest< count, 2, std::recursive_mutex, 12 >( "1 - recursive_mutex - IndexPool_3" );

	IndexedPoolAsyncPerfTest2< count, 0, SpinLock, 12 >( "2 - SpinLock - IndexPool_1" );
	IndexedPoolAsyncPerfTest2< count, 0, std::shared_mutex, 12 >( "2 - shared_mutex - IndexPool_1" );
	IndexedPoolAsyncPerfTest2< count, 0, std::mutex, 12 >( "2 - mutex - IndexPool_1" );
	IndexedPoolAsyncPerfTest2< count, 0, std::recursive_mutex, 12 >( "2 - recursive_mutex - IndexPool_1" );
	
	IndexedPoolAsyncPerfTest2< count, 1, SpinLock, 12 >( "2 - SpinLock - IndexPool_2" );
	IndexedPoolAsyncPerfTest2< count, 1, std::shared_mutex, 12 >( "2 - shared_mutex - IndexPool_2" );
	IndexedPoolAsyncPerfTest2< count, 1, std::mutex, 12 >( "2 - mutex - IndexPool_2" );
	IndexedPoolAsyncPerfTest2< count, 1, std::recursive_mutex, 12 >( "2 - recursive_mutex - IndexPool_2" );
	
	IndexedPoolAsyncPerfTest2< count, 2, SpinLock, 12 >( "2 - SpinLock - IndexPool_3" );
	IndexedPoolAsyncPerfTest2< count, 2, std::shared_mutex, 12 >( "2 - shared_mutex - IndexPool_3" );
	IndexedPoolAsyncPerfTest2< count, 2, std::mutex, 12 >( "2 - mutex - IndexPool_3" );
	IndexedPoolAsyncPerfTest2< count, 2, std::recursive_mutex, 12 >( "2 - recursive_mutex - IndexPool_3" );

	FG_LOGI( "PerformanceTest_IndexPoolMT - finished" );
}

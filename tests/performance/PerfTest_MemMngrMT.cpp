// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Containers/IndexPools.h"
#include "stl/ThreadSafe/SpinLock.h"
#include "PerfTestCommon.h"
#include <shared_mutex>


struct Resource
{
	StaticString<256>	data;
};


template <typename LockType, uint NumThreads>
static void MemManagerPerfTest1 (const uint count, const uint frames, StringView msg)
{
	StaticArray< ChunkedIndexedPool2< Resource, uint64_t, (1u << 14) >, 8 >					resources;
	StaticArray< std::tuple<std::thread, Duration_t, Duration_t, HashVal>, NumThreads >		threads;
	
	for (uint i = 0; i < 100 * count; ++i)
	{
		auto&	pool = resources[i % resources.size()];
		
		uint64_t	index;
		TEST( pool.Assign( OUT index ));

		auto&	res	= pool[index];
		
		RandomString( OUT res.data );
	}

	LockType		memmngr_lock;

	for (uint i = 0; i < threads.size(); ++i)
	{
		std::get<0>(threads[i]) = std::thread([&resources, uid = i,&memmngr_lock, count, frames,
											   dt1 = &std::get<1>(threads[i]), dt2 = &std::get<2>(threads[i]), hash = &std::get<3>(threads[i])] ()
		{
			*dt1 = *dt2 = Duration_t(0);
			auto	t2_start = TimePoint_t::clock::now();

			for (uint f = 0; f < frames; ++f)
			{
				for (uint k = 0; k < count; ++k)
				{
					const uint64_t	j	 = uint64_t(uid) + k + f;
					const uint		idx1 = uint(j % resources.size());
					const auto&		pool = resources[idx1];
					const size_t	idx0 = size_t(j % pool.size());
					const auto&		res  = pool[idx0];

					if ( (j & 0x1FF) == 0 )
					{
						auto	t1_start = TimePoint_t::clock::now();
						SCOPELOCK( memmngr_lock );
						*dt1 += TimePoint_t::clock::now() - t1_start;

						BytesU	size = res.data.length() * 333_b;
						void*	ptr  = UntypedAllocator::Allocate( size );
						if ( ptr )
						{
							for (BytesU written; written < size;)
							{
								BytesU	len = Min( size - written, BytesU(res.data.length()) );
								memcpy( ptr + written, res.data.data(), size_t(len) );
								written += len;
							}

							*hash << HashOf( ptr, size_t(size) );

							UntypedAllocator::Deallocate( ptr );
						}
					}
					else
					{
						*hash << HashOf( StringView(res.data) )
							  << HashOf( StringView(res.data).substr(1) )
							  << HashOf( StringView(res.data).substr(2) )
							  << HashOf( StringView(res.data).substr(3) );
					}
				}
			}
			*dt2 = TimePoint_t::clock::now() - t2_start;
		});
	}


	Duration_t	dt1_sum {0};
	Duration_t	dt2_sum {0};
	HashVal		h_sum;

	for (auto& thread : threads)
	{
		std::get<0>(thread).join();

		dt1_sum += std::get<1>(thread);
		dt2_sum += std::get<2>(thread);
		h_sum << std::get<3>(thread);
	}

	FG_LOGI( "blocked: "s << ToString( dt1_sum / NumThreads ) << ", time: "s << ToString( dt2_sum / NumThreads )
			 << ", mode: " << msg << ", hash: " << ToString(size_t(h_sum)) );
}


extern void PerformanceTest_MemMngrMT ()
{
#ifdef FG_DEBUG
	const uint	count = 10'000;
#else
	const uint	count = 50'000;
#endif
	const uint	frames = 30;

	MemManagerPerfTest1< std::mutex, 12 >( count, frames, "mutex" );

	FG_LOGI( "PerformanceTest_MemMngrMT - finished" );
}

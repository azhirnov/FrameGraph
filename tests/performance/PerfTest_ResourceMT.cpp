// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Containers/IndexPools.h"
#include "stl/ThreadSafe/SpinLock.h"
#include "stl/ThreadSafe/RaceConditionCheck.h"
#include "stl/ThreadSafe/Barrier.h"
#include "PerfTestCommon.h"
#include <shared_mutex>


struct Resource
{
	StaticString<256>	data;
	//RaceConditionCheck	rcCheck;
};

struct ResIndex
{
	uint		resIndex;
	uint64_t	itemIndex;
};


// one lock per resource type
template <typename LockType, uint NumThreads>
static void ResourceRWPerfTest1 (const uint count, const uint frames, const uint readCount, StringView msg)
{
#if 0
	StaticArray< ChunkedIndexedPool1< Resource, uint64_t, UntypedAlignedAllocator, LockType >, 8 >	resources;
	for (auto& res : resources) {
		std::remove_reference_t<decltype(res)>  new_res{ 1u<<14 };
		res.Swap( new_res );
	}
#else

	StaticArray< ChunkedIndexedPool2< Resource, uint64_t, (1u << 14), UntypedAlignedAllocator, LockType >, 8 >	resources;
#endif
	
	StaticArray< std::tuple<std::thread, Duration_t, Duration_t, HashVal>, NumThreads >		threads;

	Barrier				frame_begin_lock{ NumThreads };
	Barrier				frame_end_lock{ NumThreads };
	LockType			shared_indices_lock;
	Array< ResIndex >	shared_indices;
	

	for (uint i = 0; i < threads.size(); ++i)
	{
		std::get<0>(threads[i]) = std::thread([&resources, &shared_indices, &frame_begin_lock, &frame_end_lock, &shared_indices_lock,
											   uid = i, dt1 = &std::get<1>(threads[i]), dt2 = &std::get<2>(threads[i]), hash = &std::get<3>(threads[i]),
											   count, frames, readCount] ()
		{
			*dt2 = Duration_t(0);
			Array< ResIndex >		indices;

			auto	t_start = TimePoint_t::clock::now();
			for (uint f = 0; f < frames; ++f)
			{
				auto	t2_start = TimePoint_t::clock::now();
				frame_begin_lock.wait();
				*dt2 += TimePoint_t::clock::now() - t2_start;

				for (uint k = 0; k < count; ++k)
				{
					const uint64_t	j	= uint64_t(uid) + k + f;

					if ( (j & 1) or shared_indices.empty() )
					{
						uint		idx = uint(j % resources.size());
						uint64_t	index;
					
						TEST( resources[idx].Assign( OUT index ));
						indices.push_back({ idx, index });
					
						// initialize
						{
							auto&	res = resources[idx][index];

							RandomString( OUT res.data );
						}
					}
					else
					{
						for (uint c = 0; c < readCount; ++c)
						{
							const auto&	idx = shared_indices[ (j + c) % shared_indices.size() ];
							const auto&	res = resources[ idx.resIndex ][ idx.itemIndex ];

							ASSERT( not res.data.empty() );
							*hash << HashVal(j) << HashOf( res.data );
						}
					}
				}

				auto	t3_start = TimePoint_t::clock::now();
				frame_end_lock.wait();
				*dt2 += TimePoint_t::clock::now() - t3_start;
				
				{
					SCOPELOCK( shared_indices_lock );

					if ( uid == 0 )
					{
						// destroy
						const size_t	size		= shared_indices.size();
						const size_t	del_size	= size/3;

						//FG_LOGI( "del_size: "s << ToString(del_size) );

						for (size_t j = 0; j < del_size; ++j)
						{
							auto&	idx		= shared_indices[j];
							auto&	pool	= resources[ idx.resIndex ];
							auto&	res		= pool[ idx.itemIndex ];
						
							ASSERT( not res.data.empty() );
							res.data.clear();
							TEST( pool.Unassign( idx.itemIndex ));
						}
						shared_indices.erase( shared_indices.begin(), shared_indices.begin()+del_size );
					}
					shared_indices.insert( shared_indices.end(), indices.begin(), indices.end() );
				}

				indices.clear();
			}
			*dt1 = TimePoint_t::clock::now() - t_start;
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

	FG_LOGI( "time: "s << ToString( dt1_sum / NumThreads ) << ", blocked: "s << ToString( dt2_sum / NumThreads )
			  << ", mode: " << msg << ", hash: " << ToString(size_t(h_sum)) );
}


// one rw-lock per resource type
template <typename LockType, uint NumThreads>
static void ResourceRWPerfTest2 (const uint count, const uint frames, const uint readCount, StringView msg)
{
#if 0
	StaticArray< ChunkedIndexedPool1< Resource, uint64_t, UntypedAlignedAllocator >, 8 >	resources;
	for (auto& res : resources) {
		std::remove_reference_t<decltype(res)>  new_res{ 1u<<14 };
		res.Swap( new_res );
	}
#else

	StaticArray< ChunkedIndexedPool2< Resource, uint64_t, (1u << 14), UntypedAlignedAllocator >, 8 >	resources;
#endif
	
	StaticArray< std::tuple<std::thread, Duration_t, HashVal>, NumThreads >		threads;

	LockType				global_lock;
	Array< ResIndex >		indices;
	Barrier					frame_div{ NumThreads };
	
	for (uint i = 0; i < threads.size(); ++i)
	{
		std::get<0>(threads[i]) = std::thread([&resources, &indices, &global_lock, &frame_div,
											   uid = i, count, frames, readCount,
											   dt = &std::get<1>(threads[i]), hash = &std::get<2>(threads[i])] ()
		{
			auto	t_start = TimePoint_t::clock::now();
			for (uint f = 0; f < frames; ++f)
			{
				frame_div.wait();

				size_t	idx_size = 0;
				if ( uid == 0 ) {
					SCOPELOCK( global_lock );
					idx_size = indices.size();
				}

				for (uint k = 0; k < count; ++k)
				{
					const uint64_t	j			= uint64_t(uid) + k + f;
					bool			idx_empty	= false;
					
					{
						SCOPELOCK( global_lock );
						idx_empty = indices.empty();
					}

					if ( (j & 1) or idx_empty )
					{
						SCOPELOCK( global_lock );
						uint		idx = uint(j % resources.size());
						uint64_t	index;
					
						TEST( resources[idx].Assign( OUT index ));

						indices.push_back({ idx, index });
					
						// initialize
						{
							auto&	res = resources[idx][index];

							RandomString( OUT res.data );
						}
						continue;
					}

					for (uint c = 0; c < readCount; ++c)
					{
						SCOPELOCK( global_lock );
						if ( indices.empty() )
							continue;
						
						ResIndex	idx = indices[ (j + c) % indices.size() ];
						const auto&	res = resources[ idx.resIndex ][ idx.itemIndex ];

						ASSERT( not res.data.empty() );
						*hash << HashVal(j) << HashOf( res.data );
					}
				}

				if ( idx_size )
				{
					SCOPELOCK( global_lock );
					const size_t	del_size = idx_size/3;
					
					//FG_LOGI( "del_size: "s << ToString(del_size) );

					for (size_t c = 0; c < del_size; ++c)
					{
						const auto	idx	= indices[c];

						auto&	pool	= resources[ idx.resIndex ];
						auto&	res		= pool[ idx.itemIndex ];
						
						ASSERT( not res.data.empty() );
						res.data.clear();
						TEST( pool.Unassign( idx.itemIndex ));
					}

					indices.erase( indices.begin(), indices.begin()+del_size );
				}
			}
			*dt = TimePoint_t::clock::now() - t_start;
		});
	}

	
	Duration_t	dt_sum {0};
	HashVal		h_sum;

	for (auto& thread : threads)
	{
		std::get<0>(thread).join();

		dt_sum += std::get<1>(thread);
		h_sum << std::get<2>(thread);
	}

	FG_LOGI( "dt: "s << ToString( dt_sum / NumThreads ) << ", mode: " << msg << ", hash: " << ToString(size_t(h_sum)) );
}


template <typename LockType, uint NumThreads>
static void ResourceRWPerfTest3 (const uint count, const uint frames, const uint readCount, StringView msg)
{
#if 0
	StaticArray< ChunkedIndexedPool1< Resource, uint64_t, UntypedAlignedAllocator >, 8 >	resources;
	for (auto& res : resources) {
		std::remove_reference_t<decltype(res)>  new_res{ 1u<<14 };
		res.Swap( new_res );
	}
#else

	StaticArray< ChunkedIndexedPool2< Resource, uint64_t, (1u << 14), UntypedAlignedAllocator >, 8 >	resources;
#endif
	
	StaticArray< std::tuple<std::thread, Duration_t, HashVal>, NumThreads >		threads;
	
	LockType				res_lock;
	LockType				index_lock;
	Array< ResIndex >		indices;
	Barrier					frame_div{ NumThreads };
	
	for (uint i = 0; i < threads.size(); ++i)
	{
		std::get<0>(threads[i]) = std::thread([&resources, &indices, &res_lock, &index_lock, &frame_div,
											   uid = i, count, frames, readCount,
											   dt = &std::get<1>(threads[i]), hash = &std::get<2>(threads[i])] ()
		{
			auto	t_start = TimePoint_t::clock::now();
			for (uint f = 0; f < frames; ++f)
			{
				frame_div.wait();

				size_t	idx_size = 0;
				if ( uid == 0 ) {
					SCOPELOCK( index_lock );
					idx_size = indices.size();
				}

				for (uint k = 0; k < count; ++k)
				{
					const uint64_t	j			= uint64_t(uid) + k + f;
					bool			idx_empty	= false;
					
					{
						SCOPELOCK( index_lock );
						idx_empty = indices.empty();
					}

					if ( (j & 1) or idx_empty )
					{
						uint64_t	index;
						uint		idx;
						{
							SCOPELOCK( res_lock );
							idx = uint(j % resources.size());
					
							TEST( resources[idx].Assign( OUT index ));

							auto&	res = resources[idx][index];

							RandomString( OUT res.data );
						}

						SCOPELOCK( index_lock );
						indices.push_back({ idx, index });
						continue;
					}

					for (uint c = 0; c < readCount; ++c)
					{
						SCOPELOCK( index_lock );

						if ( indices.empty() )
							continue;

						ResIndex	idx = indices[ (j + c) % indices.size() ];

						SCOPELOCK( res_lock );
						const auto&	res = resources[ idx.resIndex ][ idx.itemIndex ];

						ASSERT( not res.data.empty() );
						*hash << HashVal(j) << HashOf( res.data );
					}
				}

				if ( idx_size )
				{
					const size_t	del_size = idx_size/3;
					
					//FG_LOGI( "del_size: "s << ToString(del_size) );

					for (size_t c = 0; c < del_size; ++c)
					{
						SCOPELOCK( index_lock );
						SCOPELOCK( res_lock );

						const auto	idx = indices[c];

						auto&	pool	= resources[ idx.resIndex ];
						auto&	res		= pool[ idx.itemIndex ];
						
						ASSERT( not res.data.empty() );
						res.data.clear();
						TEST( pool.Unassign( idx.itemIndex ));
					}

					{
						SCOPELOCK( index_lock );
						indices.erase( indices.begin(), indices.begin()+del_size );
					}
				}
			}
			*dt = TimePoint_t::clock::now() - t_start;
		});
	}

	
	Duration_t	dt_sum {0};
	HashVal		h_sum;

	for (auto& thread : threads)
	{
		std::get<0>(thread).join();

		dt_sum += std::get<1>(thread);
		h_sum << std::get<2>(thread);
	}

	FG_LOGI( "dt: "s << ToString( dt_sum / NumThreads ) << ", mode: " << msg << ", hash: " << ToString(size_t(h_sum)) );
}


template <typename LockType, uint NumThreads>
static void ResourceRWPerfTest4 (const uint count, const uint frames, const uint readCount, StringView msg)
{
#if 0
	StaticArray< ChunkedIndexedPool1< Resource, uint64_t, UntypedAlignedAllocator >, 8 >	resources;
	for (auto& res : resources) {
		std::remove_reference_t<decltype(res)>  new_res{ 1u<<14 };
		res.Swap( new_res );
	}
#else

	StaticArray< ChunkedIndexedPool2< Resource, uint64_t, (1u << 14), UntypedAlignedAllocator >, 8 >	resources;
#endif
	
	StaticArray< std::tuple<std::thread, Duration_t, HashVal>, NumThreads >		threads;
	
	LockType				res_lock;
	LockType				index_lock;
	Array< ResIndex >		indices;
	Barrier					frame_div{ NumThreads };
	
	for (uint i = 0; i < threads.size(); ++i)
	{
		std::get<0>(threads[i]) = std::thread([&resources, &indices, &res_lock, &index_lock, &frame_div,
											   uid = i, count, frames, readCount,
											   dt = &std::get<1>(threads[i]), hash = &std::get<2>(threads[i])] ()
		{
			auto	t_start = TimePoint_t::clock::now();
			for (uint f = 0; f < frames; ++f)
			{
				frame_div.wait();

				size_t	idx_size = 0;
				if ( uid == 0 ) {
					index_lock.lock_shared();
					idx_size = indices.size();
					index_lock.unlock_shared();
				}

				for (uint k = 0; k < count; ++k)
				{
					const uint64_t	j			= uint64_t(uid) + k + f;
					bool			idx_empty	= false;
					
					{
						index_lock.lock_shared();
						idx_empty = indices.empty();
						index_lock.unlock_shared();
					}

					if ( (j & 1) or idx_empty )
					{
						res_lock.lock();
						uint		idx = uint(j % resources.size());
						uint64_t	index;
					
						TEST( resources[idx].Assign( OUT index ));

						// initialize
						{
							auto&	res = resources[idx][index];

							RandomString( OUT res.data );
						}
						res_lock.unlock();

						index_lock.lock();
						indices.push_back({ idx, index });
						index_lock.unlock();
						continue;
					}

					for (uint c = 0; c < readCount; ++c)
					{
						index_lock.lock_shared();
						if ( not indices.empty() )
						{
							ResIndex	idx = indices[ (j + c) % indices.size() ];
							
							res_lock.lock_shared();
							const auto&	res = resources[ idx.resIndex ][ idx.itemIndex ];

							ASSERT( not res.data.empty() );
							*hash << HashVal(j) << HashOf( res.data );
							res_lock.unlock_shared();
						}
						index_lock.unlock_shared();
					}
				}

				if ( idx_size )
				{
					const size_t	del_size = idx_size/3;
					
					//FG_LOGI( "del_size: "s << ToString(del_size) );

					for (size_t c = 0; c < del_size; ++c)
					{
						index_lock.lock_shared();
						res_lock.lock_shared();

						const auto	idx = indices[c];

						auto&	pool	= resources[ idx.resIndex ];
						auto&	res		= pool[ idx.itemIndex ];
						
						ASSERT( not res.data.empty() );
						res.data.clear();
						TEST( pool.Unassign( idx.itemIndex ));

						res_lock.unlock_shared();
						index_lock.unlock_shared();
					}

					{
						index_lock.lock();
						indices.erase( indices.begin(), indices.begin()+del_size );
						index_lock.unlock();
					}
				}
			}
			*dt = TimePoint_t::clock::now() - t_start;
		});
	}

	
	Duration_t	dt_sum {0};
	HashVal		h_sum;

	for (auto& thread : threads)
	{
		std::get<0>(thread).join();

		dt_sum += std::get<1>(thread);
		h_sum << std::get<2>(thread);
	}

	FG_LOGI( "dt: "s << ToString( dt_sum / NumThreads ) << ", mode: " << msg << ", hash: " << ToString(size_t(h_sum)) );
}


template <typename LockType, uint NumThreads>
static void ResourceRWPerfTest5 (const uint count, const uint frames, const uint readCount, StringView msg)
{
	StaticArray< ChunkedIndexedPool3< Resource, uint64_t, (1u << 12), 16, UntypedAlignedAllocator, LockType, AtomicPtr >, 8 >	resources;
	using Index_t = typename decltype(resources)::value_type::Index_t;

	StaticArray< StaticArray<FixedArray<Index_t, 128>, 8>, NumThreads >		resource_cache;

	StaticArray< std::tuple<std::thread, Duration_t, Duration_t, HashVal>, NumThreads >		threads;

	Barrier				frame_begin_lock{ NumThreads };
	Barrier				frame_end_lock{ NumThreads };
	LockType			shared_indices_lock;
	Array< ResIndex >	shared_indices;
	

	for (uint i = 0; i < threads.size(); ++i)
	{
		std::get<0>(threads[i]) = std::thread([&resources, &shared_indices, &frame_begin_lock, &frame_end_lock, &shared_indices_lock, &resource_cache,
											   uid = i, dt1 = &std::get<1>(threads[i]), dt2 = &std::get<2>(threads[i]), hash = &std::get<3>(threads[i]),
											   count, frames, readCount] ()
		{
			*dt2 = Duration_t(0);
			Array< ResIndex >		indices;
			auto&					index_cache = resource_cache[uid];

			auto	t_start = TimePoint_t::clock::now();
			for (uint f = 0; f < frames; ++f)
			{
				auto	t2_start = TimePoint_t::clock::now();
				frame_begin_lock.wait();
				*dt2 += TimePoint_t::clock::now() - t2_start;

				for (uint k = 0; k < count; ++k)
				{
					const uint64_t	j	= uint64_t(uid) + k + f;

					if ( (j & 1) or shared_indices.empty() )
					{
						uint	idx = uint(j % resources.size());
					
						if ( index_cache[idx].size() == 0 )
						{
							TEST( resources[idx].Assign( index_cache[idx].capacity(), INOUT index_cache[idx] ) > 0 );
						}

						Index_t	index = index_cache[idx].back();
						index_cache[idx].pop_back();
						indices.push_back({ idx, index });
					
						// initialize
						{
							auto&	res = resources[idx][index];
							RandomString( OUT res.data );
						}
					}
					else
					{
						for (uint c = 0; c < readCount; ++c)
						{
							const auto&	idx = shared_indices[ (j + c) % shared_indices.size() ];
							const auto&	res = resources[ idx.resIndex ][ Index_t(idx.itemIndex) ];

							ASSERT( not res.data.empty() );
							*hash << HashVal(j) << HashOf( res.data );
						}
					}
				}

				auto	t3_start = TimePoint_t::clock::now();
				frame_end_lock.wait();
				*dt2 += TimePoint_t::clock::now() - t3_start;
				
				{
					SCOPELOCK( shared_indices_lock );

					if ( uid == 0 )
					{
						// destroy
						const size_t	size		= shared_indices.size();
						const size_t	del_size	= size/3;

						//FG_LOGI( "del_size: "s << ToString(del_size) );

						for (size_t c = 0; c < del_size; ++c)
						{
							auto&	idx		= shared_indices[c];
							auto&	pool	= resources[ idx.resIndex ];
							auto&	res		= pool[ Index_t(idx.itemIndex) ];
						
							ASSERT( not res.data.empty() );
							res.data.clear();
							pool.Unassign( Index_t(idx.itemIndex) );
						}
						shared_indices.erase( shared_indices.begin(), shared_indices.begin()+del_size );
					}
					shared_indices.insert( shared_indices.end(), indices.begin(), indices.end() );
				}

				indices.clear();
			}
			*dt1 = TimePoint_t::clock::now() - t_start;
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

	FG_LOGI( "time: "s << ToString( dt1_sum / NumThreads ) << ", blocked: "s << ToString( dt2_sum / NumThreads )
			  << ", mode: " << msg << ", hash: " << ToString(size_t(h_sum)) );
}


extern void PerformanceTest_ResourceMT ()
{
#ifdef FG_DEBUG
	const uint	count = 1'000;
#else
	const uint	count = 10'000;
#endif
	const uint	frames = 30;
	const uint	readCount = 100;
	
	//ResourceRWPerfTest1< SpinLock, 12 >( count, frames, readCount, "staged - SpinLock" );
	ResourceRWPerfTest1< std::shared_mutex, 12 >( count, frames, readCount, "staged - shared_mutex" );
	ResourceRWPerfTest1< std::mutex, 12 >( count, frames, readCount, "staged - mutex" );
	
	//ResourceRWPerfTest2< SpinLock, 12 >( count, frames, readCount, "global_lock - SpinLock" );
	//ResourceRWPerfTest2< std::mutex, 12 >( count, frames, readCount, "global_lock - mutex" );

	//ResourceRWPerfTest3< std::mutex, 12 >( count, frames, readCount, "index & resource locks - mutex" );

	//ResourceRWPerfTest4< std::shared_mutex, 12 >( count, frames, readCount, "RW - shared_mutex" );
	
	ResourceRWPerfTest5< SpinLock, 12 >( count, frames, readCount, "staged_2 - SpinLock" );
	ResourceRWPerfTest5< std::shared_mutex, 12 >( count, frames, readCount, "staged_2 - shared_mutex" );

	FG_LOGI( "PerformanceTest_ResourceMT - finished" );
}

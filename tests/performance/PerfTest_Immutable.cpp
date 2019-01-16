// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Containers/IndexPools.h"
#include "PerfTestCommon.h"
#include <thread>

namespace {
	struct Resource
	{
		StaticString<256>	data;
	};
}

template <uint NumThreads>
static void ImmutableStoragePerfTest1 (const uint count, const uint count2, StringView msg)
{
	struct LocalResource1
	{
		StaticString<256>	dataCopy;
	};

	ChunkedIndexedPool2< Resource, uint64_t, (1u<<14) >		resources;
	Array< uint64_t >										indices;

	for (size_t i = 0; i < count; ++i)
	{
		uint64_t	idx;
		TEST( resources.Assign( OUT idx ));

		RandomString( OUT resources[idx].data );
		ASSERT( not resources[idx].data.empty() );

		if ( (i & 2) or (i & 0x10) )
			indices.push_back( idx );
	}

	StaticArray< std::tuple<std::thread, Duration_t, HashVal>, NumThreads >	threads;

	for (uint i = 0; i < threads.size(); ++i)
	{
		std::get<0>(threads[i]) = std::thread([&resources, indices, uid = i, dt = &std::get<1>(threads[i]), hash = &std::get<2>(threads[i]), count2] () mutable
		{
			Array<uint64_t>		map_to_local;	map_to_local.resize( resources.size() );

			ChunkedIndexedPool2< LocalResource1, uint64_t, (1u<<14) >	local;
			uint64_t													def_idx;
			TEST( local.Assign( OUT def_idx ));
			TEST( def_idx == 0 );

			for (auto& idx : map_to_local) { idx = def_idx; }

			auto	t_start = TimePoint_t::clock::now();

			// remap
			for (auto& idx : indices)
			{
				auto&	l_idx = map_to_local[idx];
				if ( l_idx == def_idx )
				{
					TEST( local.Assign( OUT l_idx ));
					local[l_idx].dataCopy = resources[idx].data;
				}
				idx = l_idx;
				ASSERT( not local[idx].dataCopy.empty() );
			}

			for (uint j = 0; j < count2; ++j)
			{
				for (auto idx : indices)
				{
					*hash << HashVal(local[idx].dataCopy.length()) << HashVal(uid + j);
				}
			}

			*dt = TimePoint_t::clock::now() - t_start;
		});
	}
	
	Duration_t			dt_sum {0};
	Optional<HashVal>	ref_hash;

	for (auto& thread : threads)
	{
		std::get<0>(thread).join();

		auto	h = std::get<2>(thread);

		if ( ref_hash.has_value() )
			TEST( h == *ref_hash )
		else
			ref_hash = h;

		dt_sum += std::get<1>(thread);
	}

	FG_LOGI( "dt: "s << ToString( dt_sum / NumThreads ) << ", mode: " << msg );
}


template <uint NumThreads>
static void ImmutableStoragePerfTest2 (const uint count, const uint count2, StringView msg)
{
	struct LocalResource1
	{
		StaticString<256> const*	dataPtr = null;
	};

	ChunkedIndexedPool2< Resource, uint64_t, (1u<<14) >		resources;
	Array< uint64_t >										indices;

	for (size_t i = 0; i < count; ++i)
	{
		uint64_t	idx;
		TEST( resources.Assign( OUT idx ));

		RandomString( OUT resources[idx].data );
		ASSERT( not resources[idx].data.empty() );

		if ( (i & 2) or (i & 0x10) )
			indices.push_back( idx );
	}

	StaticArray< std::tuple<std::thread, Duration_t, HashVal>, NumThreads >	threads;

	for (uint i = 0; i < threads.size(); ++i)
	{
		std::get<0>(threads[i]) = std::thread([&resources, indices, uid = i, dt = &std::get<1>(threads[i]), hash = &std::get<2>(threads[i]), count2] () mutable
		{
			Array<uint64_t>		map_to_local;	map_to_local.resize( resources.size() );

			ChunkedIndexedPool2< LocalResource1, uint64_t, (1u<<14) >	local;
			uint64_t													def_idx;
			TEST( local.Assign( OUT def_idx ));
			TEST( def_idx == 0 );

			for (auto& idx : map_to_local) { idx = def_idx; }

			auto	t_start = TimePoint_t::clock::now();

			// remap
			for (auto& idx : indices)
			{
				auto&	l_idx = map_to_local[idx];
				if ( l_idx == def_idx )
				{
					TEST( local.Assign( OUT l_idx ));
					local[l_idx].dataPtr = &resources[idx].data;
				}
				idx = l_idx;
				ASSERT( local[idx].dataPtr != null );
			}

			for (uint j = 0; j < count2; ++j)
			{
				for (auto idx : indices)
				{
					*hash << HashVal(local[idx].dataPtr->length()) << HashVal(uid + j);
				}
			}

			*dt = TimePoint_t::clock::now() - t_start;
		});
	}
	
	Duration_t			dt_sum {0};
	Optional<HashVal>	ref_hash;

	for (auto& thread : threads)
	{
		std::get<0>(thread).join();

		auto	h = std::get<2>(thread);

		if ( ref_hash.has_value() )
			TEST( h == *ref_hash )
		else
			ref_hash = h;

		dt_sum += std::get<1>(thread);
	}

	FG_LOGI( "dt: "s << ToString( dt_sum / NumThreads ) << ", mode: " << msg );
}


template <uint NumThreads>
static void ImmutableStoragePerfTest3 (const uint count, const uint count2, StringView msg)
{
	HashMap< uint64_t, Resource >	resources;
	Array< uint64_t >				indices;

	for (size_t i = 0; i < count; ++i)
	{
		auto&	iter = resources.insert({ i, {} }).first->second;

		RandomString( OUT iter.data );
		ASSERT( not iter.data.empty() );

		if ( (i & 2) or (i & 0x10) )
			indices.push_back( i );
	}

	StaticArray< std::tuple<std::thread, Duration_t, HashVal>, NumThreads >	threads;

	for (uint i = 0; i < threads.size(); ++i)
	{
		std::get<0>(threads[i]) = std::thread([&resources, &indices, uid = i, dt = &std::get<1>(threads[i]), hash = &std::get<2>(threads[i]), count2] ()
		{
			auto	t_start = TimePoint_t::clock::now();

			for (uint j = 0; j < count2; ++j)
			{
				for (auto idx : indices)
				{
					auto	iter = resources.find( idx );
					ASSERT( iter != resources.end() );

					*hash << HashVal(iter->second.data.length()) << HashVal(uid + j);
				}
			}

			*dt = TimePoint_t::clock::now() - t_start;
		});
	}
	
	Duration_t			dt_sum {0};
	Optional<HashVal>	ref_hash;

	for (auto& thread : threads)
	{
		std::get<0>(thread).join();

		auto	h = std::get<2>(thread);

		if ( ref_hash.has_value() )
			TEST( h == *ref_hash )
		else
			ref_hash = h;

		dt_sum += std::get<1>(thread);
	}

	FG_LOGI( "dt: "s << ToString( dt_sum / NumThreads ) << ", mode: " << msg );
}



extern void PerformanceTest_Immutable ()
{
#ifdef FG_DEBUG
	const uint	count = 100'000;
#else
	const uint	count = 2'000'000;
#endif
	const uint	count2 = 10;

	ImmutableStoragePerfTest1< 12 >( count, count2, "LocalCopy" );
	ImmutableStoragePerfTest2< 12 >( count, count2, "LocalPtr" );
	ImmutableStoragePerfTest3< 12 >( count, count2, "Map" );

	FG_LOGI( "PerformanceTest_Immutable - finished" );
}

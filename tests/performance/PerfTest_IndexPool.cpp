// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Containers/IndexPools.h"
#include "stl/ThreadSafe/SpinLock.h"
#include "PerfTestCommon.h"
#include <shared_mutex>


static void IndexPool_Test1 (const uint count)
{
	ChunkedIndexedPool1< StaticString<64>, uint64_t >	pool{ 1u << 12 };
	
	auto	t_start = TimePoint_t::clock::now();

	for (uint i = 0; i < count; ++i)
	{
		uint64_t	index;
		TEST( pool.Assign( OUT index ));
	}

	auto	dt = TimePoint_t::clock::now() - t_start;
	FG_LOGI( "IndexPool_Test1: "s << ToString( dt ) );
}


static void IndexPool_Test2 (const uint count)
{
	ChunkedIndexedPool1< StaticString<64>, uint64_t >	pool{ 1u << 12 };

	for (uint i = 0; i < count; ++i)
	{
		uint64_t	index;
		TEST( pool.Assign( OUT index ));

		RandomString( OUT pool[index] );
	}
	
	auto	t_start = TimePoint_t::clock::now();
	uint	hash = 0;

	for (uint i = 0; i < count; ++i)
	{
		const auto&	str = pool[i];

		hash ^= str[i & 1];
		hash ^= str[i & 8];
	}

	auto	dt = TimePoint_t::clock::now() - t_start;

	FG_LOGI( "IndexPool_Test2: "s << ToString( dt ) << ", hash: " << ToString( hash ) );
}


static void IndexPool2_Test1 (const uint count)
{
	ChunkedIndexedPool2< StaticString<64>, uint64_t, (1u << 12) >	pool;
	
	auto	t_start = TimePoint_t::clock::now();

	for (uint i = 0; i < count; ++i)
	{
		uint64_t	index;
		TEST( pool.Assign( OUT index ));
	}

	auto	dt = TimePoint_t::clock::now() - t_start;
	FG_LOGI( "IndexPool2_Test1: "s << ToString( dt ) );
}


static void IndexPool2_Test2 (const uint count)
{
	ChunkedIndexedPool2< StaticString<64>, uint64_t, (1u << 12) >	pool;

	for (uint i = 0; i < count; ++i)
	{
		uint64_t	index;
		TEST( pool.Assign( OUT index ));

		RandomString( OUT pool[index] );
	}
	
	auto	t_start = TimePoint_t::clock::now();
	uint	hash = 0;

	for (uint i = 0; i < count; ++i)
	{
		const auto&	str = pool[i];

		hash ^= str[i & 1];
		hash ^= str[i & 8];
	}

	auto	dt = TimePoint_t::clock::now() - t_start;

	FG_LOGI( "IndexPool_Test2: "s << ToString( dt ) << ", hash: " << ToString( hash ) );
}


extern void PerformanceTest_IndexPool ()
{
#ifdef FG_DEBUG
	const uint	count = 100'000;
#else
	const uint	count = 4'000'000;
#endif

	IndexPool_Test1( count );
	IndexPool2_Test1( count );

	IndexPool_Test2( count );
	IndexPool2_Test2( count );

	FG_LOGI( "PerformanceTest_IndexPool - finished" );
}

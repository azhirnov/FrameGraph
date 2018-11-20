// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/ChunkedIndexedPool.h"
#include "stl/Containers/CachedIndexedPool.h"
#include "stl/CompileTime/Math.h"
#include "UnitTest_Common.h"


static void ChunkedIndexedPool_Test1 ()
{
	constexpr uint									count = 1024;
	ChunkedIndexedPool< int, uint, count/16, 16 >	pool;
	
	for (size_t i = 0; i < count; ++i)
	{
		uint	idx;
		TEST( pool.Assign( OUT idx ) == (i < count) );
		TEST( (idx == i) == (i < count) );
	}
	
	for (size_t i = 0; i < count; ++i)
	{
		pool.Unassign( uint(i) );
	}
}


static void ChunkedIndexedPool_Test2 ()
{
	constexpr uint									count = 1024;
	ChunkedIndexedPool< int, uint, count/16, 16 >	pool;
	FixedArray< uint, 16 >							arr;
	
	for (size_t i = 0; i < count; i += arr.capacity())
	{
		TEST( pool.Assign( ~0u, INOUT arr ) > 0 );
		TEST( arr.size() == arr.capacity() );
		pool.Unassign( ~0u, INOUT arr );
	}
}


static void CachedIndexedPool_Test1 ()
{
	CachedIndexedPool<uint, uint, 16, 16>	pool;

	uint	idx1, idx2;

	TEST( pool.Assign( OUT idx1 ));
	TEST( pool.Assign( OUT idx2 ));

	TEST( pool.Insert( idx1, 2 ).second );
	TEST( not pool.Insert( idx2, 2 ).second );
	
	pool.RemoveFromCache( idx2 );
	pool.RemoveFromCache( idx1 );
	pool.Unassign( idx2 );
	pool.Unassign( idx1 );
}


extern void UnitTest_IndexedPool ()
{
	ChunkedIndexedPool_Test1();
	ChunkedIndexedPool_Test2();
	CachedIndexedPool_Test1();

    FG_LOGI( "UnitTest_IndexedPool - passed" );
}

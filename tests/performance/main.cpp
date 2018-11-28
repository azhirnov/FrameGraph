// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"
#include "Containers/IndexPools.h"
#include <iostream>

extern void PerformanceTest_FixedMap ();
extern void PerformanceTest_IndexPool ();
extern void PerformanceTest_IndexPoolMT ();
extern void PerformanceTest_Immutable ();
extern void PerformanceTest_ResourceMT ();
extern void PerformanceTest_MemMngrMT ();


static void IndexedPool_Test1 ()
{
	IndexedPool<uint>	pool{ 4_Mb };

	TEST( pool.Capacity() == 524288 );
}


static void IndexedPool_Test2 ()
{
	using Index_t = IndexedPool<uint>::Index_t;

	IndexedPool<uint>	pool{ 1000 };
	Index_t				index;

	TEST( pool.Assign( OUT index ));
	pool.Unassign( index );
}


static void ChunkedIndexedPool1_Test1 ()
{
	ChunkedIndexedPool1<uint, uint>	pool{ 16 };
	Array<uint>						indices;

	for (uint i = 0; i < 200; ++i)
	{
		uint	idx;
		TEST( pool.Assign( OUT idx ));
		indices.push_back( idx );
	}

	for (auto& idx : indices)
	{
		TEST( pool.Unassign( idx ));
	}
}


static void ChunkedIndexedPool2_Test1 ()
{
	ChunkedIndexedPool2<uint, uint, 16>	pool;
	Array<uint>							indices;

	for (uint i = 0; i < 200; ++i)
	{
		uint	idx;
		TEST( pool.Assign( OUT idx ));
		indices.push_back( idx );
	}

	for (auto& idx : indices)
	{
		TEST( pool.Unassign( idx ));
	}
}


static void CachedIndexedPool1_Test1 ()
{
	CachedIndexedPool1<uint, uint>	pool{ 16 };

	uint	idx1, idx2;

	TEST( pool.Assign( OUT idx1 ));
	TEST( pool.Assign( OUT idx2 ));

	TEST( pool.Insert( idx1, 2 ).second );
	TEST( not pool.Insert( idx2, 2 ).second );
	
	TEST( pool.Unassign( idx2, true ));
	TEST( pool.Unassign( idx1, true ));
}


static void CachedIndexedPool2_Test1 ()
{
	CachedIndexedPool2<uint, uint, 16>	pool;

	uint	idx1, idx2;

	TEST( pool.Assign( OUT idx1 ));
	TEST( pool.Assign( OUT idx2 ));

	TEST( pool.Insert( idx1, 2 ).second );
	TEST( not pool.Insert( idx2, 2 ).second );
	
	TEST( pool.Unassign( idx2, true ));
	TEST( pool.Unassign( idx1, true ));
}


int main ()
{
	// unit tests
	IndexedPool_Test1();
	IndexedPool_Test2();
	ChunkedIndexedPool1_Test1();
	ChunkedIndexedPool2_Test1();
	CachedIndexedPool1_Test1();
	CachedIndexedPool2_Test1();

	//PerformanceTest_FixedMap();
	//PerformanceTest_IndexPool();
	PerformanceTest_IndexPoolMT();
	//PerformanceTest_Immutable();
	//PerformanceTest_ResourceMT();
	//PerformanceTest_MemMngrMT();

	FG_LOGI( "Tests.Performance finished" );
	
	std::cin.ignore();
	return 0;
}

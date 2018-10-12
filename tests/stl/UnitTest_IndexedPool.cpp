// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/IndexedPool.h"
#include "stl/Containers/ChunkedIndexedPool.h"
#include "UnitTestCommon.h"


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


static void ChunkedIndexedPool_Test1 ()
{
	ChunkedIndexedPool<uint, uint>	pool{ 16 };
	Array<uint>						indices;

	for (uint i = 0; i < 200; ++i)
	{
		uint	idx;
		TEST( pool.Assign( idx ));
		indices.push_back( idx );
	}

	for (auto& idx : indices)
	{
		pool.Unassign( idx );
	}
}


extern void UnitTest_IndexedPool ()
{
	IndexedPool_Test1();
	IndexedPool_Test2();
	ChunkedIndexedPool_Test1();

    FG_LOGI( "UnitTest_IndexedPool - passed" );
}

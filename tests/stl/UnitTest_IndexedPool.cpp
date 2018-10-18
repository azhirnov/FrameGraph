// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/ChunkedIndexedPool.h"
#include "stl/Containers/CachedIndexedPool.h"
#include "stl/Containers/ChunkedIndexedPool2.h"
#include "stl/Containers/CachedIndexedPool2.h"
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


static void CachedIndexedPool_Test1 ()
{
	CachedIndexedPool<uint, uint>	pool{ 16 };

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


static void BitTree_Test1 ()
{
	constexpr uint											bit_count = 333; //32*32;
	_fg_hidden_::BitTreeImpl< uint, uint, bit_count, 0 >	bit;	bit.Initialize( 0, bit_count );
	STATIC_ASSERT( bit.Capacity() >= bit_count );

	for (uint i = 0; i < bit_count+1; ++i)
	{
		uint	index;
		TEST( bit.Assign( OUT index ) == (i < bit_count) );
		TEST( (index == i) == (i < bit_count) );
	}

	for (uint i = 0; i < bit_count; ++i)
	{
		TEST( bit.IsAssigned( i ));
		bit.Unassign( i );
	}

	
	constexpr uint											bit2_count = 3333; //32*32*32;
	_fg_hidden_::BitTreeImpl< uint, uint, bit2_count, 1 >	bit2;	bit2.Initialize( 0, bit2_count );
	STATIC_ASSERT( bit2.Capacity() >= bit2_count );

	for (uint i = 0; i < bit2_count+1; ++i)
	{
		uint	index;
		TEST( bit2.Assign( OUT index ) == (i < bit2_count) );
		TEST( (index == i) == (i < bit2_count) );
	}
	
	for (uint i = 0; i < bit2_count; ++i)
	{
		TEST( bit2.IsAssigned( i ));
		bit2.Unassign( i );
	}

	
	constexpr uint											bit3_count = 3333; //32*32*32*32;
	_fg_hidden_::BitTreeImpl< uint, uint, bit3_count, 2 >	bit3;	bit3.Initialize( 0, bit3_count );
	STATIC_ASSERT( bit3.Capacity() >= bit3_count );

	for (uint i = 0; i < bit3_count+1; ++i)
	{
		uint	index;
		TEST( bit3.Assign( OUT index ) == (i < bit3_count) );
		TEST( (index == i) == (i < bit3_count) );
	}
	
	for (uint i = 0; i < bit3_count; ++i)
	{
		TEST( bit3.IsAssigned( i ));
		bit3.Unassign( i );
	}
}


extern void UnitTest_IndexedPool ()
{
	IndexedPool_Test1();
	IndexedPool_Test2();
	ChunkedIndexedPool_Test1();
	ChunkedIndexedPool2_Test1();
	CachedIndexedPool_Test1();
	CachedIndexedPool2_Test1();
	BitTree_Test1();

    FG_LOGI( "UnitTest_IndexedPool - passed" );
}

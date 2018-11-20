// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/BitTree.h"
#include "UnitTest_Common.h"


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

	for (uint i = 0; i < bit_count+1; ++i)
	{
		uint	index;
		TEST( bit.Assign( OUT index ) == (i < bit_count) );
		TEST( (index == i) == (i < bit_count) );
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
	
	for (uint i = 0; i < bit2_count+1; ++i)
	{
		uint	index;
		TEST( bit2.Assign( OUT index ) == (i < bit2_count) );
		TEST( (index == i) == (i < bit2_count) );
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
	
	for (uint i = 0; i < bit3_count+1; ++i)
	{
		uint	index;
		TEST( bit3.Assign( OUT index ) == (i < bit3_count) );
		TEST( (index == i) == (i < bit3_count) );
	}
}


extern void UnitTest_BitTree ()
{
	BitTree_Test1();

    FG_LOGI( "UnitTest_BitTree - passed" );
}

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/ThreadSafe/DoubleBuffer.h"
#include "UnitTestCommon.h"


static void DoubleBuffer_Test1 ()
{
	STATIC_ASSERT( DoubleBuffer<2>::Mask == 3 );
	STATIC_ASSERT( DoubleBuffer<3>::Mask == 3 );
	STATIC_ASSERT( DoubleBuffer<4>::Mask == 7 );

	// maximum size for 32 bit value:
	// 7+1 buffers * 3 bits per index = 24 bits
	STATIC_ASSERT( DoubleBuffer<7>::Mask == 7 );
	STATIC_ASSERT( DoubleBuffer<7>::Offset == 3 );
	STATIC_ASSERT(( IsSameTypes< typename DoubleBuffer<7>::Value_t, uint32_t > ));
	
	STATIC_ASSERT( DoubleBuffer<8>::Mask == 15 );
	STATIC_ASSERT(( IsSameTypes< typename DoubleBuffer<8>::Value_t, uint64_t > ));

	// maximum size for 64 bit value:
	// 15+1 buffers * 4 bits per index = 64 bits
	STATIC_ASSERT( DoubleBuffer<15>::Mask == 15 );
	STATIC_ASSERT( DoubleBuffer<15>::Offset == 4 );
	STATIC_ASSERT(( IsSameTypes< typename DoubleBuffer<15>::Value_t, uint64_t > ));
}


static void DoubleBuffer_Test2 ()
{
	DoubleBuffer<2>		tb;

	uint	first	= tb.Get( 0 );
	uint	second	= tb.Get( 1 );
	TEST( first != second );

	uint	third	= tb.Swap( 0 );
	TEST( first != third and second != third );

	uint	p1	= tb.Swap( 0 );
	TEST( first == p1 );
	
	uint	p2	= tb.Swap( 0 );
	TEST( third == p2 );

	uint	p3	= tb.Swap( 0 );
	TEST( first == p3 );
	
	uint	p4	= tb.Swap( 1 );
	TEST( third == p4 );

	uint	p5	= tb.Swap( 1 );
	TEST( second == p5 );
}


extern void UnitTest_DoubleBuffer ()
{
	DoubleBuffer_Test1();
	DoubleBuffer_Test2();
    FG_LOGI( "UnitTest_DoubleBuffer - passed" );
}

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/ThreadSafe/LfDoubleBuffer.h"
#include "UnitTest_Common.h"


static void DoubleBuffer_Test1 ()
{
	STATIC_ASSERT( LfDoubleBuffer<2>::Mask == 3 );
	STATIC_ASSERT( LfDoubleBuffer<3>::Mask == 3 );
	STATIC_ASSERT( LfDoubleBuffer<4>::Mask == 7 );

	// maximum size for 32 bit value:
	// 7+1 buffers * 3 bits per index = 24 bits
	STATIC_ASSERT( LfDoubleBuffer<7>::Mask == 7 );
	STATIC_ASSERT( LfDoubleBuffer<7>::Offset == 3 );
	STATIC_ASSERT(( IsSameTypes< typename LfDoubleBuffer<7>::Value_t, uint32_t > ));
	
	STATIC_ASSERT( LfDoubleBuffer<8>::Mask == 15 );
	STATIC_ASSERT(( IsSameTypes< typename LfDoubleBuffer<8>::Value_t, uint64_t > ));

	// maximum size for 64 bit value:
	// 15+1 buffers * 4 bits per index = 64 bits
	STATIC_ASSERT( LfDoubleBuffer<15>::Mask == 15 );
	STATIC_ASSERT( LfDoubleBuffer<15>::Offset == 4 );
	STATIC_ASSERT(( IsSameTypes< typename LfDoubleBuffer<15>::Value_t, uint64_t > ));
}


static void DoubleBuffer_Test2 ()
{
	LfDoubleBuffer<2>		tb;

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


extern void UnitTest_LfDoubleBuffer ()
{
	DoubleBuffer_Test1();
	DoubleBuffer_Test2();
	FG_LOGI( "UnitTest_LfDoubleBuffer - passed" );
}

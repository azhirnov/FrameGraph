// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Math/Math.h"
#include "stl/Math/BitMath.h"
#include "stl/CompileTime/Math.h"
#include "UnitTest_Common.h"


static void IsIntersects_Test1 ()
{
	TEST( IsIntersects( 2, 6, 5, 8 ));
	TEST( IsIntersects( 2, 6, 0, 3 ));
	TEST( IsIntersects( 2, 6, 3, 5 ));
	TEST( not IsIntersects( 2, 6, 6, 8 ));
	TEST( not IsIntersects( 2, 6, -3, 2 ));
}


static void IntLog2_Test1 ()
{
	int	val;
	val = IntLog2( 0 );				TEST( val < 0 );
	val = IntLog2( 0x100 );			TEST( val == 8 );
	val = IntLog2( 0x101 );			TEST( val == 8 );
	val = IntLog2( 0x80000000u );	TEST( val == 31 );
	
	STATIC_ASSERT( CT_IntLog2<0> < 0 );
	STATIC_ASSERT( CT_IntLog2<0x100> == 8 );
	STATIC_ASSERT( CT_IntLog2<0x101> == 8 );
	STATIC_ASSERT( CT_IntLog2<0x80000000u> == 31 );
	STATIC_ASSERT( CT_IntLog2<0x8000000000000000ull> == 63 );
}


static void BitScanForward_Test1 ()
{
	int	val;
	val = BitScanForward( 0 );			TEST( val < 0 );
	val = BitScanForward( 0x100 );		TEST( val == 8 );
	val = BitScanForward( 0x101 );		TEST( val == 0 );
}


static void Wrap_Test1 ()
{
	float b0 = Wrap( 1.0f, 2.0f, 5.0f );	TEST( Equals( b0, 4.0f ));
	float b1 = Wrap( 6.0f, 2.0f, 5.0f );	TEST( Equals( b1, 3.0f ));
	float b2 = Wrap( 4.0f, 2.0f, 5.0f );	TEST( Equals( b2, 4.0f ));
	float b4 = Wrap( 1.5f, 2.0f, 5.0f );	TEST( Equals( b4, 4.5f ));
	float b5 = Wrap( 5.5f, 2.0f, 5.0f );	TEST( Equals( b5, 2.5f ));
	float b6 = Wrap( 15.0f, 0.0f, 5.0f );	TEST( Equals( b6, 0.0f ));
	float b7 = Wrap( 2.0f, -5.0f, 0.0f );	TEST( Equals( b7, -3.0f ));
	float b10 = Wrap( 3.99f, 0.0f, 4.0f );	TEST( Equals( b10, 3.99f ));
}


extern void UnitTest_Math ()
{
	IsIntersects_Test1();
	IntLog2_Test1();
	BitScanForward_Test1();
	Wrap_Test1();

	FG_LOGI( "UnitTest_Math - passed" );
}

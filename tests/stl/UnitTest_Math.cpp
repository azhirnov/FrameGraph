// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Math/Math.h"
#include "UnitTestCommon.h"


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
}


static void BitScanForward_Test1 ()
{
	int	val;
	val = BitScanForward( 0 );			TEST( val < 0 );
	val = BitScanForward( 0x100 );		TEST( val == 8 );
	val = BitScanForward( 0x101 );		TEST( val == 0 );
}


extern void UnitTest_Math ()
{
	IsIntersects_Test1();
	IntLog2_Test1();
	BitScanForward_Test1();
    FG_LOGI( "UnitTest_Math - passed" );
}

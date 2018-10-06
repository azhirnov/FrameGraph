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


extern void UnitTest_Math ()
{
	IsIntersects_Test1();
    FG_LOGI( "UnitTest_Math - passed" );
}

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Algorithms/ArrayUtils.h"
#include "UnitTest_Common.h"


static void BinarySearch_Test1 ()
{
	Array<int>	arr = { 0, 1, 2, 3, 4, 4, 4, 5, 6, 7, 7, 7, 7, 7, 8, 9, 10, 11, 12 };
	ArrayView	view = arr;
	size_t		pos;
	
	pos = BinarySearch( view, 5 );	TEST( pos == 7 );
	pos = BinarySearch( view, 9 );	TEST( pos == 15 );
	pos = BinarySearch( view, 4 );	TEST( pos == 4 );
	pos = BinarySearch( view, 7 );	TEST( pos == 9 );
}


extern void UnitTest_Array ()
{
	BinarySearch_Test1();

	FG_LOGI( "UnitTest_Array - passed" );
}

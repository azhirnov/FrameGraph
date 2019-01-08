// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Algorithms/ArrayUtils.h"
#include "stl/Containers/Appendable.h"
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


static void Appendable_Test1 ()
{
	Array<int>		arr;
	Appendable<int>	app{ arr };

	app.push_back( 1 );
	app.push_back( 2 );

	TEST( arr.size() == 2 );
	TEST( arr[0] == 1 );
	TEST( arr[1] == 2 );
}


static Pair<int, float> Conv (int &&i) {
	return { i, float(i) };
}

static void Appendable_Test2 ()
{
	Array<Pair<int, float>>		arr;
	Appendable<int>				app{ arr, std::integral_constant< decltype(&Conv), &Conv >{} };

	app.push_back( 1 );
	app.push_back( 2 );

	TEST( arr.size() == 2 );
	TEST( arr[0].first == 1 );
	TEST( arr[1].first == 2 );
}


extern void UnitTest_Array ()
{
	BinarySearch_Test1();
	Appendable_Test1();
	Appendable_Test2();

	FG_LOGI( "UnitTest_Array - passed" );
}

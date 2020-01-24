// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/FixedTupleArray.h"
#include "UnitTest_Common.h"


static void FixedTupleArray_Test1 ()
{
	FixedTupleArray< 32, int, float >	arr;

	arr.push_back( 1, 2.2f );
	arr.push_back( 2, 3.3f );
	
	TEST( arr.size() == 2 );
	TEST( arr.get<int>()[0] == 1 );
	TEST( arr.get<int>()[1] == 2 );
	TEST( arr.get<float>()[0] == 2.2f );
	TEST( arr.get<float>()[1] == 3.3f );

	arr.clear();
	TEST( arr.size() == 0 );
}


static void FixedTupleArray_Test2 ()
{
	FixedTupleArray< 32, int, float >	arr;

	arr.insert( 0, 1, 2.2f );
	TEST( arr.size() == 1 );
	TEST( arr.get<int>()[0] == 1 );
	TEST( arr.get<float>()[0] == 2.2f );

	arr.insert( 0, 2, 3.3f );
	TEST( arr.size() == 2 );
	TEST( arr.get<int>()[0] == 2 );
	TEST( arr.get<int>()[1] == 1 );
	TEST( arr.get<float>()[0] == 3.3f );
	TEST( arr.get<float>()[1] == 2.2f );
	
	arr.insert( 3, 3, 4.4f );
	TEST( arr.size() == 3 );
	TEST( arr.get<int>()[0] == 2 );
	TEST( arr.get<int>()[1] == 1 );
	TEST( arr.get<int>()[2] == 3 );
	TEST( arr.get<float>()[0] == 3.3f );
	TEST( arr.get<float>()[1] == 2.2f );
	TEST( arr.get<float>()[2] == 4.4f );
	
	arr.insert( 2, 4, 5.5f );
	TEST( arr.size() == 4 );
	TEST( arr.get<int>()[0] == 2 );
	TEST( arr.get<int>()[1] == 1 );
	TEST( arr.get<int>()[2] == 4 );
	TEST( arr.get<int>()[3] == 3 );
	TEST( arr.get<float>()[0] == 3.3f );
	TEST( arr.get<float>()[1] == 2.2f );
	TEST( arr.get<float>()[2] == 5.5f );
	TEST( arr.get<float>()[3] == 4.4f );
}


extern void UnitTest_FixedTupleArray ()
{
	FixedTupleArray_Test1();
	FixedTupleArray_Test2();

	FG_LOGI( "UnitTest_FixedTupleArray - passed" );
}

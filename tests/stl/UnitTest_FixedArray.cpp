// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/FixedArray.h"
#include "UnitTest_Common.h"


static void FixedArray_Test1 ()
{
	FixedArray< std::shared_ptr<int>, 16 >	arr = {{
		std::make_shared<int>(0),
		std::make_shared<int>(1),
		std::make_shared<int>(2),
		std::make_shared<int>(3),
		std::make_shared<int>(4)
	}};

	std::weak_ptr<int>	elem0 = arr[0];
	std::weak_ptr<int>	elem1 = arr[1];
	std::weak_ptr<int>	elem2 = arr[2];
	std::weak_ptr<int>	elem3 = arr[3];
	std::weak_ptr<int>	elem4 = arr[4];

	arr.clear();

	TEST( elem0.expired() );
	TEST( elem1.expired() );
	TEST( elem2.expired() );
	TEST( elem3.expired() );
	TEST( elem4.expired() );
}


static void FixedArray_Test2 ()
{
	using T = DebugInstanceCounter< int, 1 >;

	T::ClearStatistic();
	{
		FixedArray< T, 8 >	arr;

		arr.resize( 3 );
		arr[0] = T(1);
		arr[1] = T(2);
		arr[2] = T(3);

		arr.resize( 6 );
		TEST( arr[0] == T(1) );
		TEST( arr[1] == T(2) );
		TEST( arr[2] == T(3) );
		TEST( arr[3] == T(0) );
		TEST( arr[4] == T(0) );
		TEST( arr[5] == T(0) );

		arr.resize( 2 );
		TEST( arr[0] == T(1) );
		TEST( arr[1] == T(2) );
		ASSERT( arr.data()[2] != T(3) );
	}
	TEST( T::CheckStatistic() );
}


static void FixedArray_Test3 ()
{
	using T = DebugInstanceCounter< int, 2 >;

	T::ClearStatistic();
	{
		FixedArray< T, 8 >	arr1 = { T(1), T(2), T(3) };
		FixedArray< T, 16 >	arr2 = { T(6), T(7), T(8), T(9), T(3), T(4) };

		arr2 = ArrayView<T>( arr1 );
		TEST( arr2 == arr1 );

		//arr1 = arr1;	// error
	}
	TEST( T::CheckStatistic() );
}


extern void UnitTest_FixedArray ()
{
	FixedArray_Test1();
	FixedArray_Test2();
	FixedArray_Test3();
	FG_LOGI( "UnitTest_FixedArray - passed" );
}

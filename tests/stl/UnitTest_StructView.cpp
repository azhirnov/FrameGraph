// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/StructView.h"
#include "UnitTest_Common.h"


static void StructView_Test1 ()
{
	const int		data[] = { 0, 1, 2, 3, 4, 5 };
	StructView<int>	view{ data };

	TEST( CountOf( data ) == view.size() );

	for (size_t i = 0; i < view.size(); ++i) {
		TEST( data[i] == view[i] );
	}
}


static void StructView_Test2 ()
{
	#pragma pack (push, 1)
	struct Data {
		int		field1;
		double	field2;
		uint8_t	field3;
	};
	#pragma pack (pop)

	const Data	data[] = {
		{ 1, 1.111, 11 },
		{ 2, 2.222, 22 },
		{ 3, 3.333, 33 }
	};

	StructView<double>	view{ ArrayView<Data>{data}, &Data::field2 };
	
	TEST( CountOf( data ) == view.size() );

	for (size_t i = 0; i < view.size(); ++i) {
		TEST( data[i].field2 == view[i] );
	}
}


static void StructView_Test3 ()
{
	struct Data {
		int		field1;
		double	field2;
		uint8_t	field3;
	};

	const Data	data[] = {
		{ 1, 1.111, 11 },
		{ 2, 2.222, 22 },
		{ 3, 3.333, 33 }
	};

	StructView<double>	view{ ArrayView<Data>{data}, &Data::field2 };
	
	TEST( CountOf( data ) == view.size() );

	for (size_t i = 0; i < view.size(); ++i) {
		TEST( data[i].field2 == view[i] );
	}
}


extern void UnitTest_StructView ()
{
	StructView_Test1();
	StructView_Test2();
	StructView_Test3();

	FG_LOGI( "UnitTest_StructView - passed" );
}

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Containers/FixedMap.h"
#include "UnitTest_Common.h"


static void TimeToStringTest ()
{
	String	s1 = ToString( std::chrono::seconds(10) );
	TEST( s1 == "10.00 s" );

	String	s2 = ToString( std::chrono::milliseconds(10) );
	TEST( s2 == "10.00 ms" );

	String	s3 = ToString( std::chrono::microseconds(10) );
	TEST( s3 == "10.00 us" );

	String	s4 = ToString( std::chrono::nanoseconds(10) );
	TEST( s4 == "10.00 ns" );

	String	s5 = ToString( std::chrono::minutes(10) );
	TEST( s5 == "10.00 m" );
}


extern void UnitTest_ToString ()
{
	TimeToStringTest();

	FG_LOGI( "UnitTest_ToString - passed" );
}

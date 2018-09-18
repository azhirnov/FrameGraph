// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/include/StaticString.h"
#include "UnitTestCommon.h"


static void StaticString_Test1 ()
{
	String				str2 = "12345678";
    StaticString<64>	str1 = StringView{str2};

	TEST( str1.length() == str2.length() );
	TEST( str1.size() == str2.size() );
	TEST( str1 == str2 );
}


extern void UnitTest_StaticString ()
{
	StaticString_Test1();
    FG_LOGI( "UnitTest_StaticString - passed" );
}

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/include/Common.h"
#include <iostream>

extern void UnitTest_StaticString ();
extern void UnitTest_FixedArray ();


int main ()
{
	UnitTest_StaticString();
	UnitTest_FixedArray();

    FG_LOGI( "Tests.STL finished" );
	
    DEBUG_ONLY( std::cin.ignore() );
	return 0;
}

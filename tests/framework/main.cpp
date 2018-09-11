// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "stl/include/Common.h"
#include <iostream>

using namespace FG;

extern void FW_Test1 ();
extern void FW_Test2 ();


int main ()
{
	//FW_Test1();
	FW_Test2();

    FG_LOGI( "Tests.Framework finished" );
	
    DEBUG_ONLY( std::cin.ignore() );
	return 0;
}

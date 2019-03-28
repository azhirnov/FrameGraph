// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"
#include <iostream>

extern void Test_CmdBatchSort ();


int main ()
{
	Test_CmdBatchSort();

	FG_LOGI( "Tests.Algorithms finished" );

	#ifdef PLATFORM_WINDOWS
	std::cin.ignore();
	#endif
	return 0;
}

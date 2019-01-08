// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"
#include <iostream>

using namespace FG;

extern void FW_Test1 ();
extern void FW_Test2 ();


int main ()
{
#ifdef PLATFORM_ANDROID
	// single-threaded
	FW_Test1();
#else
	// multi-threaded and multi-device
	FW_Test2();
#endif

	FG_LOGI( "Tests.Framework finished" );
	
	std::cin.ignore();
	return 0;
}

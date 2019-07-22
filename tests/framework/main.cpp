// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"
#include <iostream>

using namespace FGC;

extern void FW_Test1 ();
extern void FW_Test2 ();
extern void FW_Test3 ();


int main ()
{
#if 1
	// VR
	FW_Test3();
	
#elif defined(PLATFORM_ANDROID)
	// single-threaded
	FW_Test1();
#else
	// multi-threaded and multi-device
	FW_Test2();
#endif

	FG_LOGI( "Tests.Framework finished" );

	#ifdef PLATFORM_WINDOWS
	std::cin.ignore();
	#endif
	return 0;
}

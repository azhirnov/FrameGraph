// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

using namespace FGC;

extern void FW_Test1 ();
extern void FW_Test2 ();
extern void FW_Test3 ();


int main ()
{
#if 0
	// VR
	FW_Test3();
	
#elif defined(PLATFORM_ANDROID)
	// single-threaded
	FW_Test1();
#else
	// multi-threaded and multi-device
	FW_Test2();
#endif
	
	CHECK_FATAL( FG_DUMP_MEMLEAKS() );

	FG_LOGI( "Tests.Framework finished" );
	return 0;
}

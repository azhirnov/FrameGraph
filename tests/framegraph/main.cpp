// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "FGApp.h"
#include <iostream>

using namespace FG;


int main ()
{
	FGApp::Run();

	FG_LOGI( "Tests.FrameGraph finished" );
	
	#ifdef PLATFORM_WINDOWS
	std::cin.ignore();
	#endif
	return 0;
}

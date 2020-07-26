// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "UIApp.h"
#include <iostream>

using namespace FG;


int main ()
{
	UIApp::Run();
	
	CHECK_FATAL( FG_DUMP_MEMLEAKS() );

	FG_LOGI( "Tests.UI finished" );
	return 0;
}

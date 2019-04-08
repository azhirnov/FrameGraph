// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SceneApp.h"
#include <iostream>

using namespace FG;

extern void UnitTest_Transformation ();
extern void UnitTest_Frustum ();


int main ()
{
	UnitTest_Transformation();
	UnitTest_Frustum();

	/*{
		SceneApp	scene;
		CHECK_ERR( scene.Initialize(), -1 );

		for (; scene.Update();) {}
	}*/

	FG_LOGI( "Tests.Scene finished" );
	
	#ifdef PLATFORM_WINDOWS
	std::cin.ignore();
	#endif
	return 0;
}

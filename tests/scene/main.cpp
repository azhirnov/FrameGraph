// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SceneApp.h"

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
	
	CHECK_FATAL( FG_DUMP_MEMLEAKS() );

	FG_LOGI( "Tests.Scene finished" );
	return 0;
}

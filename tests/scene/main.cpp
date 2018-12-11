// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SceneApp.h"
#include <iostream>

using namespace FG;

extern void UnitTest_Transformation ();
extern void UnitTest_Frustum ();


int main ()
{
	UnitTest_Transformation();
	UnitTest_Frustum();

	{
		SceneApp	scene;

		scene.Initialize( R"#(D:\3dmodels\SUPER_TERRAIN_obj\OBJ\Models_OBJ\Terrain_50000.obj)#", {} );

		for (; scene.Update();) {}
	}

	FG_LOGI( "Tests.Scene finished" );
	
	std::cin.ignore();
	return 0;
}

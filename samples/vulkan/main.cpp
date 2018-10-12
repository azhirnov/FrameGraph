// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "stl/Common.h"
#include <iostream>

using namespace FG;

extern void MeshShader_FirstTriangle ();


int main ()
{
	MeshShader_FirstTriangle();
	
    FG_LOGI( "Vulkan samples finished" );
    std::cin.ignore();
	return 0;
}

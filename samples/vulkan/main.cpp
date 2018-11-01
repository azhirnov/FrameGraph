// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "stl/Common.h"
#include <iostream>

using namespace FG;

extern void MeshShader_Sample1 ();
extern void ShadingRateImage_Sample1 ();
extern void ImageFootprint_Sample1 ();
extern void FragShaderBarycentric_Sample1 ();
extern void SparseImage_Sample1 ();


int main ()
{
	MeshShader_Sample1();
	ShadingRateImage_Sample1();
	ImageFootprint_Sample1();
	FragShaderBarycentric_Sample1();
	SparseImage_Sample1();

    FG_LOGI( "Vulkan samples finished" );
    std::cin.ignore();
	return 0;
}

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"
#include <iostream>

using namespace FG;

extern void MeshShader_Sample1 ();
extern void RayTracing_Sample1 ();
extern void RayTracing_Sample2 ();
extern void ShadingRateImage_Sample1 ();
extern void ImageFootprint_Sample1 ();
extern void FragShaderBarycentric_Sample1 ();
extern void SparseImage_Sample1 ();
extern void AsyncCompute_Sample1 ();


int main ()
{
	MeshShader_Sample1();
	RayTracing_Sample1();
	RayTracing_Sample2();
	ShadingRateImage_Sample1();
	ImageFootprint_Sample1();
	FragShaderBarycentric_Sample1();
	SparseImage_Sample1();
	AsyncCompute_Sample1();

	FG_LOGI( "Vulkan samples finished" );
	std::cin.ignore();
	return 0;
}

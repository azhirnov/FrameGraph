// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

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
extern void SparseImage_Sample2 ();
extern void AsyncCompute_Sample1 ();


int main ()
{
	//SparseImage_Sample1();
	//AsyncCompute_Sample1();
	//MeshShader_Sample1();
	RayTracing_Sample1();
	//RayTracing_Sample2();
	//FragShaderBarycentric_Sample1();
	//SparseImage_Sample1();
	//AsyncCompute_Sample1();

	FG_LOGI( "Vulkan samples finished" );
	std::cin.ignore();
	return 0;
}

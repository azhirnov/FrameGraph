// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "FGApp.h"

using namespace FG;

extern void UnitTest_VertexInput ();
extern void UnitTest_ImageSwizzle ();
extern void UnitTest_PixelFormat ();
extern void UnitTest_ID ();
extern void UnitTest_VBuffer ();
extern void UnitTest_VImage ();


int main ()
{
	FG_LOGI( "Run tests for "s << IFrameGraph::GetVersion() );

	// unit tests
	{
		UnitTest_VertexInput();
		UnitTest_ImageSwizzle();
		UnitTest_PixelFormat();
		UnitTest_ID();
		UnitTest_VBuffer();
		UnitTest_VImage();
	}

	FGApp::Run();

	FG_LOGI( "Tests.FrameGraph finished" );
	return 0;
}

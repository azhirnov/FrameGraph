// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "FGApp.h"

using namespace FG;

extern void UnitTest_VertexInput ();
extern void UnitTest_ImageSwizzle ();
extern void UnitTest_PixelFormat ();
extern void UnitTest_ID ();
extern void UnitTest_VBuffer ();
extern void UnitTest_VImage ();
extern void UnitTest_ImageDesc ();


#ifdef PLATFORM_ANDROID
extern int Tests_FrameGraph_main (void* nativeHandle) {
#else
int main () {
	void* nativeHandle = null;
#endif

	FG_LOGI( "Run tests for "s << IFrameGraph::GetVersion() );

	// unit tests
	{
		UnitTest_VertexInput();
		UnitTest_ImageSwizzle();
		UnitTest_PixelFormat();
		UnitTest_ID();
		UnitTest_VBuffer();
		UnitTest_VImage();
		UnitTest_ImageDesc();
	}

	FGApp::Run( nativeHandle );
	
	CHECK_FATAL( FG_DUMP_MEMLEAKS() );

	FG_LOGI( "Tests.FrameGraph finished" );
	return 0;
}

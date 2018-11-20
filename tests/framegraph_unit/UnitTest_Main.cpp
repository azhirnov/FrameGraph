// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/VFG.h"
#include "VDevice.h"
#include "VMemoryManager.h"

using namespace FG;


extern void UnitTest_VertexInput ();
extern void UnitTest_ImageSwizzle ();
extern void UnitTest_ID ();
extern void UnitTest_VBuffer ();
extern void UnitTest_VImage ();

extern void UnitTest_Main ()
{
	UnitTest_VertexInput();
	UnitTest_ImageSwizzle();
	UnitTest_ID();
	UnitTest_VBuffer();
	UnitTest_VImage();
    FG_LOGI( "UnitTest_Main - finished" );
}
//-----------------------------------------------------------------------------



extern void UnitTest_VResourceManager (const FGThreadPtr &fg);

extern void UnitTest_FGMain (const FGThreadPtr &fg)
{
	UnitTest_VResourceManager( fg );
    FG_LOGI( "UnitTest_FGMain - finished" );
}

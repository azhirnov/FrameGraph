// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/VFG.h"
#include "VDevice.h"

using namespace FG;

extern void UnitTest_VertexInput ();
extern void UnitTest_ImageSwizzle ();


extern void UnitTest_Main ()
{
	UnitTest_VertexInput();
	UnitTest_ImageSwizzle();
    FG_LOGI( "UnitTest_Main - finished" );
}
//-----------------------------------------------------------------------------



extern void UnitTest_VPipelineCache (const VDevice &dev);
extern void UnitTest_VSamplerCache (const VDevice &dev);


extern void UnitTest_VkMain (const VulkanDeviceInfo &vdi)
{
	VDevice		dev{ vdi };

	UnitTest_VSamplerCache( dev );
	UnitTest_VPipelineCache( dev );
    FG_LOGI( "UnitTest_VkMain - finished" );
}
//-----------------------------------------------------------------------------



extern void UnitTest_VBuffer (const FrameGraphPtr &fg);
extern void UnitTest_VImage (const FrameGraphPtr &fg);


extern void UnitTest_FGMain (const FrameGraphPtr &fg)
{
	UnitTest_VBuffer( fg );
	UnitTest_VImage( fg );
    FG_LOGI( "UnitTest_FGMain - finished" );
}

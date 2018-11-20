// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/Public/ImageSwizzle.h"
#include "UnitTest_Common.h"


static void ImageSwizzle_Test1 ()
{
	constexpr ImageSwizzle	s1 = "RGBA"_swizzle;
	STATIC_ASSERT( s1.Get() == 0x4321 );
	STATIC_ASSERT(All( s1.ToVec() == uint4(1, 2, 3, 4) ));

	constexpr ImageSwizzle	s2 = "R000"_swizzle;
	STATIC_ASSERT(All( s2.ToVec() == uint4(1, 5, 5, 5) ));
	
	constexpr ImageSwizzle	s3 = "0G01"_swizzle;
	STATIC_ASSERT(All( s3.ToVec() == uint4(5, 2, 5, 6) ));
}


extern void UnitTest_ImageSwizzle ()
{
	ImageSwizzle_Test1();
    FG_LOGI( "UnitTest_ImageSwizzle - passed" );
}

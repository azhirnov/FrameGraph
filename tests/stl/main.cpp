// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"
#include <iostream>

extern void UnitTest_StaticString ();
extern void UnitTest_FixedArray ();
extern void UnitTest_FixedMap ();
extern void UnitTest_IndexedPool ();
extern void UnitTest_LinearAllocator ();
extern void UnitTest_Math ();
extern void UnitTest_Color ();
extern void UnitTest_ToString ();
extern void UnitTest_DoubleBuffer ();
extern void UnitTest_BitTree ();


int main ()
{
	UnitTest_Math();
	UnitTest_Color();
	UnitTest_StaticString();
	UnitTest_FixedArray();
	UnitTest_ToString();
	UnitTest_FixedMap();
	UnitTest_IndexedPool();
	UnitTest_LinearAllocator();
	UnitTest_DoubleBuffer();
	UnitTest_BitTree();

    FG_LOGI( "Tests.STL finished" );
	
    std::cin.ignore();
	return 0;
}

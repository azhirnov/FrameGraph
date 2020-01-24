// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

extern void UnitTest_StaticString ();
extern void UnitTest_FixedArray ();
extern void UnitTest_FixedMap ();
extern void UnitTest_IndexedPool ();
extern void UnitTest_LinearAllocator ();
extern void UnitTest_Math ();
extern void UnitTest_Matrix ();
extern void UnitTest_Color ();
extern void UnitTest_ToString ();
extern void UnitTest_LfDoubleBuffer ();
extern void UnitTest_BitTree ();
extern void UnitTest_LfFixedStack ();
extern void UnitTest_StructView ();
extern void UnitTest_Array ();
extern void UnitTest_StringParser ();
extern void UnitTest_FixedTupleArray ();
extern void UnitTest_LfIndexedPool ();
extern void UnitTest_Rectangle ();
extern void UnitTest_NtStringView ();
extern void UnitTest_TypeList ();


int main ()
{
	UnitTest_Math();
	UnitTest_Matrix();
	UnitTest_Color();
	UnitTest_StaticString();
	UnitTest_FixedArray();
	UnitTest_ToString();
	UnitTest_FixedMap();
	UnitTest_IndexedPool();
	UnitTest_LinearAllocator();
	UnitTest_LfDoubleBuffer();
	UnitTest_LfFixedStack();
	UnitTest_BitTree();
	UnitTest_StructView();
	UnitTest_Array();
	UnitTest_StringParser();
	UnitTest_FixedTupleArray();
	UnitTest_LfIndexedPool();
	UnitTest_Rectangle();
	UnitTest_NtStringView();
	UnitTest_TypeList();

	FG_LOGI( "Tests.STL finished" );
	return 0;
}

// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/ThreadSafe/LfFixedStack.h"
#include "UnitTestCommon.h"


static void LfFixedStack_Test1 ()
{
	LfFixedStack<int, 8>	stack;

	stack.Push( 1 );
	stack.Push( 2 );
	stack.Push( 3 );

	TEST( stack.Get()[0] == 1 );
	TEST( stack.Get()[1] == 2 );
	TEST( stack.Get()[2] == 3 );
}


static void LfFixedStack_Test2 ()
{
	LfFixedStack< std::tuple<int, float, uint64_t>, 8 >		stack;

	stack.Push( 1, 1.1f, 1ull << 60 );
	stack.Push( 2, 1.2f, 2ull << 60 );
	stack.Push( 3, 1.3f, 3ull << 60 );

	TEST( stack.Get<int>()[0] == 1 );
	TEST( stack.Get<int>()[1] == 2 );
	TEST( stack.Get<int>()[2] == 3 );
	
	TEST( stack.Get<float>()[0] == 1.1f );
	TEST( stack.Get<float>()[1] == 1.2f );
	TEST( stack.Get<float>()[2] == 1.3f );
	
	TEST( stack.Get<uint64_t>()[0] == (1ull << 60) );
	TEST( stack.Get<uint64_t>()[1] == (2ull << 60) );
	TEST( stack.Get<uint64_t>()[2] == (3ull << 60) );
}


extern void UnitTest_LfFixedStack ()
{
	LfFixedStack_Test1();
	LfFixedStack_Test2();
    FG_LOGI( "UnitTest_LfFixedStack - passed" );
}

// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/ThreadSafe/LfIndexedPool.h"
#include "UnitTest_Common.h"


static void LfIndexedPool_Test1 ()
{
	LfIndexedPool< int, uint, 32, 16 >	pool;

	for (uint i = 0; i < 32*16*10; ++i)
	{
		uint	index;
		TEST( pool.Assign( OUT index ));

		pool.Unassign( index );
	}
}


static void LfIndexedPool_Test2 ()
{
	using T = DebugInstanceCounter< int, 1 >;
	
	T::ClearStatistic();
	{
		LfIndexedPool< T, uint, 32, 16 >	pool;
	
		for (uint i = 0; i < 32*16*10; ++i)
		{
			uint	index;
			TEST( pool.Assign( OUT index ));

			pool.Unassign( index );
		}
	}
	TEST( T::CheckStatistic() );
}


static void LfIndexedPool_Test3 ()
{
	using T = DebugInstanceCounter< int, 2 >;
	
	T::ClearStatistic();
	{
		constexpr uint							count = 32*16;
		LfIndexedPool< T, uint, count/16, 16 >	pool;
	
		for (uint i = 0; i < count+1; ++i)
		{
			uint	index;
			bool	res = pool.Assign( OUT index );

			if ( i < count )
				TEST( res )
			else
				TEST( not res );
		}
		
		for (uint i = 0; i < count; ++i)
		{
			pool.Unassign( i );
		}
	}
	TEST( T::CheckStatistic() );
}


extern void UnitTest_LfIndexedPool ()
{
	LfIndexedPool_Test1();
	LfIndexedPool_Test2();
	LfIndexedPool_Test3();

	FG_LOGI( "UnitTest_LfIndexedPool - passed" );
}

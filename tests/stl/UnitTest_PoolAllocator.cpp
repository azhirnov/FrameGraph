// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Memory/PoolAllocator.h"
#include "UnitTestCommon.h"


static void PoolAllocator_Test1 ()
{
	using T = DebugInstanceCounter<uint, 1>;
	
	PoolAllocator	pool;
	pool.SetBlockSize( 4_Mb );


	T::ClearStatistic();
	{
		std::vector< T, StdPoolAllocator<T> >	vec{ pool };

		vec.resize( 100 );
		vec.push_back( T(101) );
	}
	TEST( T::CheckStatistic() );
}



extern void UnitTest_PoolAllocator ()
{
	PoolAllocator_Test1();
    FG_LOGI( "UnitTest_PoolAllocator - passed" );
}

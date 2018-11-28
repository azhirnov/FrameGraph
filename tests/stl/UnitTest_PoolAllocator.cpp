// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Memory/LinearAllocator.h"
#include "UnitTest_Common.h"


static void LinearAllocator_Test1 ()
{
	using T = DebugInstanceCounter<uint, 1>;
	
	LinearAllocator		pool;
	pool.SetBlockSize( 4_Mb );


	T::ClearStatistic();
	{
		std::vector< T, StdLinearAllocator<T> >	vec{ pool };

		vec.resize( 100 );
		vec.push_back( T(101) );
	}
	TEST( T::CheckStatistic() );
}



extern void UnitTest_LinearAllocator ()
{
	LinearAllocator_Test1();
	FG_LOGI( "UnitTest_LinearAllocator - passed" );
}

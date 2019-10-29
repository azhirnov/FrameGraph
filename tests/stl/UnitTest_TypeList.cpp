// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/CompileTime/TypeList.h"
#include "stl/CompileTime/TypeTraits.h"
#include "UnitTest_Common.h"

struct TL_Visitor 
{
	size_t sizeof_sum = 0;

	template <typename T, size_t I>
	void operator () () {
		sizeof_sum += sizeof(T);
	}
};

extern void UnitTest_TypeList ()
{
	STATIC_ASSERT(( TypeList<int, bool, float>::Count == 3 ));
	STATIC_ASSERT(( TypeList<int, int, int>::Count == 3 ));
	
	using TL = TypeList< int, float, bool, double >;
	STATIC_ASSERT( IsSameTypes< TL::Get<0>, int > );
	STATIC_ASSERT( IsSameTypes< TL::Get<1>, float > );
	STATIC_ASSERT( IsSameTypes< TL::Get<2>, bool > );
	STATIC_ASSERT( IsSameTypes< TL::Get<3>, double > );
	STATIC_ASSERT( TL::Index<int> == 0 );
	STATIC_ASSERT( TL::Index<float> == 1 );
	STATIC_ASSERT( TL::Index<bool> == 2 );
	STATIC_ASSERT( TL::Index<double> == 3 );

	TL_Visitor v;
	TL::Visit( v );
	TEST( v.sizeof_sum == (sizeof(int) + sizeof(float) + sizeof(bool) + sizeof(double)) );

	FG_LOGI( "UnitTest_TypeList - passed" );
}

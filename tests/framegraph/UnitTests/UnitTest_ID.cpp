// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/Public/IDs.h"
#include "UnitTest_Common.h"


static void ID_Test1 ()
{
	constexpr VertexID	id0{ "dnfksjdanflksdjfn" };
	constexpr VertexID	id1{ "dnfksjdanflksdjfn" };
	constexpr VertexID	id2{ "dnjdanflksdjfn" };

	STATIC_ASSERT( id0.GetHash() == id1.GetHash() );
	STATIC_ASSERT( id0.GetHash() != id2.GetHash() );

	String	str;
	str += "dnfksjd";
	str += "anflksdjfn";

	VertexID	id3{ str };

	TEST( id3.GetHash() == id0.GetHash() );
}


static void ID_Test2 ()
{
	using ID1 = _fg_hidden_::IDWithString< 32, 100, true, UMax >;
	using ID2 = _fg_hidden_::IDWithString< 32, 100, false, UMax >;

	ID2		a {"test"};
	ID1		b {"test"};
	TEST( b == a );
}


extern void UnitTest_ID ()
{
	ID_Test1();
	ID_Test2();
	FG_LOGI( "UnitTest_ID - passed" );
}

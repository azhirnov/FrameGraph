// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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


extern void UnitTest_ID ()
{
	ID_Test1();
    FG_LOGI( "UnitTest_ID - passed" );
}

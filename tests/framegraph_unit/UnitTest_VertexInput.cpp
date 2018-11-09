// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "framegraph/Public/VertexInputState.h"
#include "UnitTestCommon.h"

using VertexAttrib = VertexInputState::VertexAttrib;


inline bool TestVertexInput (const VertexInputState &state, const VertexID &id, uint requiredIndex)
{
	auto	iter = state.Vertices().find( id );

	if ( iter == state.Vertices().end() )
		return false;

	return	iter->second.index	== requiredIndex;
}


static void VertexInput_Test1 ()
{
	struct Vertex1
	{
		float3			position;
		Vec<short,2>	texcoord;
		RGBA8u			color;
	};

	const FixedArray<VertexAttrib, 16>	attribs = {{
		{ VertexID("position"),	2, EVertexType::Float3 },
		{ VertexID("texcoord"),	0, EVertexType::Float2 },
		{ VertexID("color"),	1, EVertexType::Float4 }
	}};

	VertexInputState	vertex_input;
	vertex_input.Bind( VertexBufferID(), SizeOf<Vertex1> );

	vertex_input.Add( VertexID("position"),	&Vertex1::position );
	vertex_input.Add( VertexID("texcoord"),	&Vertex1::texcoord, true );
	vertex_input.Add( VertexID("color"),	&Vertex1::color );

	TEST( vertex_input.ApplyAttribs( attribs ));

	TEST( TestVertexInput( vertex_input, VertexID("position"),	2 ));
	TEST( TestVertexInput( vertex_input, VertexID("texcoord"),	0 ));
	TEST( TestVertexInput( vertex_input, VertexID("color"),		1 ));
}


static void VertexInput_Test2 ()
{
	struct Vertex1
	{
		float3			position;
		Vec<short,2>	texcoord;
		RGBA8u			color;
	};

	const FixedArray<VertexAttrib, 16>	attribs = {{
		{ VertexID("position1"),	2, EVertexType::Float3 },
		{ VertexID("texcoord1"),	0, EVertexType::Float2 },
		{ VertexID("color1"),		1, EVertexType::Float4 },
		{ VertexID("position2"),	5, EVertexType::Float3 },
		{ VertexID("texcoord2"),	4, EVertexType::Float2 },
		{ VertexID("color2"),		3, EVertexType::Float4 }
	}};
	
	VertexInputState	vertex_input;
	vertex_input.Bind( VertexBufferID("frame1"), SizeOf<Vertex1> );
	vertex_input.Bind( VertexBufferID("frame2"), SizeOf<Vertex1> );

	vertex_input.Add( VertexID("position1"),	&Vertex1::position,			VertexBufferID("frame1") );
	vertex_input.Add( VertexID("texcoord1"),	&Vertex1::texcoord, true,	VertexBufferID("frame1") );
	vertex_input.Add( VertexID("color1"),		&Vertex1::color,			VertexBufferID("frame1") );
	vertex_input.Add( VertexID("position2"),	&Vertex1::position,			VertexBufferID("frame2") );
	vertex_input.Add( VertexID("texcoord2"),	&Vertex1::texcoord, true,	VertexBufferID("frame2") );
	vertex_input.Add( VertexID("color2"),		&Vertex1::color,			VertexBufferID("frame2") );
	
	TEST( vertex_input.ApplyAttribs( attribs ));

	TEST( TestVertexInput( vertex_input, VertexID("position1"),	2 ));
	TEST( TestVertexInput( vertex_input, VertexID("texcoord1"),	0 ));
	TEST( TestVertexInput( vertex_input, VertexID("color1"),	1 ));
	TEST( TestVertexInput( vertex_input, VertexID("position2"),	5 ));
	TEST( TestVertexInput( vertex_input, VertexID("texcoord2"),	4 ));
	TEST( TestVertexInput( vertex_input, VertexID("color2"),	3 ));
}


extern void UnitTest_VertexInput ()
{
	VertexInput_Test1();
	VertexInput_Test2();
    FG_LOGI( "UnitTest_VertexInput - passed" );
}

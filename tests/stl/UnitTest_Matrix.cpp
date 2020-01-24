// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Math/Matrix.h"
#include "UnitTest_Common.h"


static void Matrix_Test1 ()
{
	using CMat4x2_t = Matrix< float, 4, 2, EMatrixOrder::ColumnMajor >;
	using RMat4x2_t = Matrix< float, 4, 2, EMatrixOrder::RowMajor >;

	STATIC_ASSERT( CMat4x2_t::Column_t::size() == 2 );
	STATIC_ASSERT( CMat4x2_t::Row_t::size() == 4 );
	STATIC_ASSERT( sizeof(CMat4x2_t) == sizeof(float) * 4 * 2 );

	STATIC_ASSERT( RMat4x2_t::Row_t::size() == 4 );
	STATIC_ASSERT( RMat4x2_t::Column_t::size() == 2 );
	STATIC_ASSERT( sizeof(RMat4x2_t) == sizeof(float) * 4 * 2 );
};


static void Matrix_Test2 ()
{
	using CMat4x2_t = Matrix< float, 4, 2, EMatrixOrder::ColumnMajor, sizeof(float)*4 >;
	using RMat4x2_t = Matrix< float, 4, 2, EMatrixOrder::RowMajor, sizeof(float)*4 >;

	STATIC_ASSERT( CMat4x2_t::Column_t::size() == 2 );
	STATIC_ASSERT( CMat4x2_t::Row_t::size() == 4 );
	STATIC_ASSERT( sizeof(CMat4x2_t) == sizeof(float) * 4 * 4 );

	STATIC_ASSERT( RMat4x2_t::Row_t::size() == 4 );
	STATIC_ASSERT( RMat4x2_t::Column_t::size() == 2 );
	STATIC_ASSERT( sizeof(RMat4x2_t) == sizeof(float) * 4 * 2 );
};


static void Matrix_Test3 ()
{
	using float3x4_t = Matrix< float, 3, 4, EMatrixOrder::ColumnMajor >;
	using float4x3_t = Matrix< float, 4, 3, EMatrixOrder::ColumnMajor >;
	using float3x3_t = Matrix< float, 3, 3, EMatrixOrder::ColumnMajor >;

	float3 v1 = float4() * float3x4_t();
	float4 v2 = float3x4_t() * float3();

	float3x3_t m1 = float4x3_t() * float3x4_t();
}


static void Matrix_Test4 ()
{
	using float3x4_t = Matrix< float, 3, 4, EMatrixOrder::RowMajor >;
	using float4x3_t = Matrix< float, 4, 3, EMatrixOrder::RowMajor >;
	using float3x3_t = Matrix< float, 3, 3, EMatrixOrder::RowMajor >;

	float3 v1 = float3x4_t() * float4();
	float4 v2 = float3() * float3x4_t();
	
	float3x3_t m1 = float4x3_t() * float3x4_t();
}


static void Matrix_Test5 ()
{
	using float3x4_t = Matrix< float, 3, 4, EMatrixOrder::ColumnMajor >;

	const float3x4_t  m1{};

	const float3x4_t  m2{ float4(1.0f, 2.0f, 3.0f, 4.0f),
						  float4(5.0f, 6.0f, 7.0f, 8.0f),
						  float4(9.0f, 10.0f, 11.0f, 12.0f) };

	ASSERT(All( m2[0] == float4(1.0f, 2.0f, 3.0f, 4.0f) ));
	ASSERT(All( m2[1] == float4(5.0f, 6.0f, 7.0f, 8.0f) ));
	ASSERT(All( m2[2] == float4(9.0f, 10.0f, 11.0f, 12.0f) ));
	
	const float3x4_t  m3{ 1.0f, 2.0f, 3.0f, 4.0f,
						  5.0f, 6.0f, 7.0f, 8.0f,
						  9.0f, 10.0f, 11.0f, 12.0f };

	ASSERT(All( m3[0] == float4(1.0f, 2.0f, 3.0f, 4.0f) ));
	ASSERT(All( m3[1] == float4(5.0f, 6.0f, 7.0f, 8.0f) ));
	ASSERT(All( m3[2] == float4(9.0f, 10.0f, 11.0f, 12.0f) ));
}


static void Matrix_Test6 ()
{
	using float3x4_t = Matrix< float, 3, 4, EMatrixOrder::RowMajor >;

	const float3x4_t  m1{};

	const float3x4_t  m2{ float3(1.0f, 2.0f, 3.0f),
						  float3(4.0f, 5.0f, 6.0f),
						  float3(7.0f, 8.0f, 9.0f),
						  float3(10.0f, 11.0f, 12.0f) };

	ASSERT(All( m2[0] == float3(1.0f, 2.0f, 3.0f) ));
	ASSERT(All( m2[1] == float3(4.0f, 5.0f, 6.0f) ));
	ASSERT(All( m2[2] == float3(7.0f, 8.0f, 9.0f) ));
	ASSERT(All( m2[3] == float3(10.0f, 11.0f, 12.0f) ));

	const float3x4_t  m3{ 1.0f, 2.0f, 3.0f,
						  4.0f, 5.0f, 6.0f,
						  7.0f, 8.0f, 9.0f,
						  10.0f, 11.0f, 12.0f };

	ASSERT(All( m3[0] == float3(1.0f, 2.0f, 3.0f) ));
	ASSERT(All( m3[1] == float3(4.0f, 5.0f, 6.0f) ));
	ASSERT(All( m3[2] == float3(7.0f, 8.0f, 9.0f) ));
	ASSERT(All( m3[3] == float3(10.0f, 11.0f, 12.0f) ));
}


static void Matrix_Test7 ()
{
	using float4x4_t = Matrix< float, 4, 4, EMatrixOrder::ColumnMajor >;
	using float3x3_t = Matrix< float, 3, 3, EMatrixOrder::ColumnMajor >;

	float4x4_t	m1{
		3.0f, 4.0f, 5.0f, 1.0f,
		7.0f, 2.0f, 6.0f, 0.5f,
		3.0f, 1.0f, 9.0f, 4.0f,
		5.0f, 2.0f, 4.0f, 2.0f
	};
	float4x4_t	m2 = m1.Inverse();
	float4x4_t	m3 = m2.Inverse();
	
	for (uint c = 0; c < 4; ++c)
	for (uint r = 0; r < 4; ++r) {
		TEST(Equals( m1[c][r], m3[c][r], 0.01f ));
	}
	
	float3x3_t	m4{
		3.0f, 4.0f, 5.0f,
		7.0f, 2.0f, 6.0f,
		3.0f, 1.0f, 9.0f,
	};
	float3x3_t	m5 = m4.Inverse();
	float3x3_t	m6 = m5.Inverse();
	
	for (uint c = 0; c < 3; ++c)
	for (uint r = 0; r < 3; ++r) {
		TEST(Equals( m4[c][r], m6[c][r], 0.01f ));
	}
}


static void Matrix_Test8 ()
{
	/*using float4x4_t = Matrix< float, 4, 4, EMatrixOrder::RowMajor >;
	using float3x3_t = Matrix< float, 3, 3, EMatrixOrder::RowMajor >;

	float4x4_t	m1{
		3.0f, 4.0f, 5.0f, 1.0f,
		7.0f, 2.0f, 6.0f, 0.5f,
		3.0f, 1.0f, 9.0f, 4.0f,
		5.0f, 2.0f, 4.0f, 2.0f
	};
	float4x4_t	m2 = m1.Inverse();
	float4x4_t	m3 = m2.Inverse();
	
	for (uint c = 0; c < 4; ++c)
	for (uint r = 0; r < 4; ++r) {
		TEST(Equals( m1[c][r], m3[c][r], 0.01f ));
	}
	
	float3x3_t	m4{
		3.0f, 4.0f, 5.0f,
		7.0f, 2.0f, 6.0f,
		3.0f, 1.0f, 9.0f,
	};
	float3x3_t	m5 = m4.Inverse();
	float3x3_t	m6 = m5.Inverse();
	
	for (uint c = 0; c < 3; ++c)
	for (uint r = 0; r < 3; ++r) {
		TEST(Equals( m4[c][r], m6[c][r], 0.01f ));
	}*/
}


extern void UnitTest_Matrix ()
{
	Matrix_Test1();
	Matrix_Test2();
	Matrix_Test3();
	Matrix_Test4();
	Matrix_Test5();
	Matrix_Test6();
	Matrix_Test7();
	Matrix_Test8();

	FG_LOGI( "UnitTest_Matrix - passed" );
}

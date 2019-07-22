// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "SphericalCubeMath.h"

using namespace FG;

#define TEST	CHECK_FATAL


template <typename Projection>
static void Test_ForwardInverseProjection ()
{
	static constexpr uint	lod = 12;
	static constexpr double	err = 0.0001;

	for (uint face = 0; face < 6; ++face)
	{
		for (uint y = 1; y < lod+2; ++y)
		for (uint x = 1; x < lod+2; ++x)
		{
			const double2  ncoord = double2{ double(x)/(lod+2), double(y)/(lod+2) } * 2.0 - 1.0;

			const double3  forward = Projection::Forward( ncoord, ECubeFace(face) );

			auto[inv, inv_face] = Projection::Inverse( forward );

			TEST( uint(inv_face) == face );
			TEST(Equals( ncoord.x, inv.x, err ));
			TEST(Equals( ncoord.y, inv.y, err ));
		}
	}
}


extern void UnitTest_SphericalCubeMath ()
{
	Test_ForwardInverseProjection< OriginCube >();
	Test_ForwardInverseProjection< IdentitySphericalCube >();
	Test_ForwardInverseProjection< TangentialSphericalCube >();

	double3	c0 = IdentitySphericalCube::Forward( double2{0.0, 1.0101}, ECubeFace::XPos );
	double3	c1 = IdentitySphericalCube::Forward( double2{0.0, 1.01}, ECubeFace::XPos );
	double3	c2 = IdentitySphericalCube::Forward( double2{0.0, 1.00997}, ECubeFace::XPos );

	double3	b0 = IdentitySphericalCube::Forward( double2{1.01, 0.0}, ECubeFace::YNeg );
	double3	b1 = IdentitySphericalCube::Forward( double2{1.00, 0.0}, ECubeFace::YNeg );
	double3	b2 = IdentitySphericalCube::Forward( double2{0.99, 0.0}, ECubeFace::YNeg );

	FG_LOGI( "UnitTest_SphericalCubeMath" );
}

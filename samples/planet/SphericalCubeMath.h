// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Math/Spherical.h"

namespace FG
{

	enum class ECubeFace
	{
		XPos	= 0,
		XNeg	= 1,
		YPos	= 2,
		YNeg	= 3,
		ZPos	= 4,
		ZNeg	= 5,
	};

/*
=================================================
	RotateVec, RotateTexcoord
----
	project 2D coord to cube face
=================================================
*/
	ND_ inline double3  RotateVec (const double3 &c, ECubeFace face)
	{
		switch ( face )
		{
			case ECubeFace::XPos : return double3( c.z,  c.y, -c.x);	// X+
			case ECubeFace::XNeg : return double3(-c.z,  c.y,  c.x);	// X-
			case ECubeFace::YPos : return double3( c.x, -c.z,  c.y);	// Y+
			case ECubeFace::YNeg : return double3( c.x,  c.z, -c.y);	// Y-
			case ECubeFace::ZPos : return double3( c.x,  c.y,  c.z);	// Z+
			case ECubeFace::ZNeg : return double3(-c.x,  c.y, -c.z);	// Z-
		}
		return {};
	}
	
/*
=================================================
	InverseRotation
----
	project 3D coord on cube/sphere to face 2D coord
=================================================
*/
	ND_ inline Tuple<double2, double, ECubeFace>  InverseRotation (const double3 &c)
	{
		// front (xy space)
		if ( (Abs(c.x) <= c.z) & (c.z > 0.0) & (Abs(c.y) <= c.z) )
			return { double2(c.x, c.y), c.z, ECubeFace::ZPos };

		// right (zy space)
		if ( (Abs(c.z) <= c.x) & (c.x > 0.0) & (Abs(c.y) <= c.x) )
			return { double2(-c.z, c.y), c.x, ECubeFace::XPos };

		// back (xy space)
		if ( (Abs(c.x) <= -c.z) & (c.z < 0.0) & (Abs(c.y) <= -c.z) )
			return { double2(-c.x, c.y), -c.z, ECubeFace::ZNeg };

		// left (zy space)
		if ( (Abs(c.z) <= -c.x) & (c.x < 0.0) & (Abs(c.y) <= -c.x) )
			return { double2(c.z, c.y), -c.x, ECubeFace::XNeg };

		// up (xz space)
		if ( c.y > 0.0 )
			return { double2(c.x, -c.z), c.y, ECubeFace::YNeg };

		// down (xz space)
		return { double2(c.x, c.z), -c.y, ECubeFace::YPos };
	}

/*
=================================================
	OriginCube
=================================================
*/
	struct OriginCube
	{
		ND_ static double3  Forward (const double2 &ncoord, ECubeFace face)
		{
			return RotateVec( double3{ ncoord, 1.0 }, face );
		}

		ND_ static Pair<double2, ECubeFace>  Inverse (const double3 &coord)
		{
			auto[c, z, face] = InverseRotation( coord );
			return { c, face };
		}
	};
	
/*
=================================================
	IdentitySphericalCube
=================================================
*/
	struct IdentitySphericalCube
	{
		ND_ static double3  Forward (const double2 &ncoord, ECubeFace face)
		{
			return Normalize( RotateVec( double3{ ncoord, 1.0 }, face ));
		}

		ND_ static Pair<double2, ECubeFace>  Inverse (const double3 &coord)
		{
			auto[c, z, face] = InverseRotation( coord );
			c /= z;
			return { c, face };
		}
	};
	
/*
=================================================
	TangentialSphericalCube
=================================================
*/
	struct TangentialSphericalCube
	{
		static constexpr double  warp_theta		= 0.868734829276;
		static constexpr double  tan_warp_theta	= 1.182286685546; //tan( warp_theta );
		
		ND_ static double3  Forward (const double2 &ncoord, ECubeFace face)
		{
			double	x = tan( warp_theta * ncoord.x ) / tan_warp_theta;
			double	y = tan( warp_theta * ncoord.y ) / tan_warp_theta;

			return Normalize( RotateVec( double3{x, y, 1.0}, face ));
		}

		ND_ static Pair<double2, ECubeFace>  Inverse (const double3 &coord)
		{
			auto[c, z, face] = InverseRotation( coord );
			c /= z;

			double	x = atan( c.x * tan_warp_theta ) / warp_theta;
			double	y = atan( c.y * tan_warp_theta ) / warp_theta;

			return { double2(x,y), face };
		}
	};
	
/*
=================================================
	AdjustedSphericalCube
=================================================
*/
	struct AdjustedSphericalCube
	{
	};
	
/*
=================================================
	TextureProjection
=================================================
*/
	struct TextureProjection
	{
		ND_ static double3  Forward (const double2 &ncoord, ECubeFace face)
		{
			return IdentitySphericalCube::Forward( ncoord, face ) * 0.5;
		}

		ND_ static Pair<double2, ECubeFace>  Inverse (const double3 &coord)
		{
			return IdentitySphericalCube::Inverse( coord * 2.0 );
		}
	};


}	// FG

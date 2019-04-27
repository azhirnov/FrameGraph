// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Math/Radians.h"

namespace FGC
{

	//
	// Spherical Coordinates
	//

	template <typename T>
	struct SphericalTempl
	{
		STATIC_ASSERT( IsFloatPoint<T> );

	// types
	public:
		using Value_t	= T;
		using Angle_t	= RadiansTempl< T >;
		using Self		= SphericalTempl< T >;
		using Vec3_t	= Vec< T, 3 >;


	// variables
	public:
		Angle_t		theta;		// polar angle
		Angle_t		phi;		// azimuthal angle


	// methods
	public:
		SphericalTempl () {}
		SphericalTempl (T theta, T phi) : theta{theta}, phi{phi} {}
		SphericalTempl (Angle_t theta, Angle_t phi) : theta{theta}, phi{phi} {}

		ND_ static Pair<Self, Value_t>	FromCartesian (const Vec3_t &cartesian);
		ND_ Vec3_t						ToCartesian () const;
		ND_ Vec3_t						ToCartesian (Value_t radius) const;
	};


	using SphericalF	= SphericalTempl< float >;
	using SphericalD	= SphericalTempl< double >;


	
/*
=================================================
	FromCartesian
=================================================
*/
	template <typename T>
	inline Pair<SphericalTempl<T>, T>  SphericalTempl<T>::FromCartesian (const Vec<T,3> &cartesian)
	{
		const T	radius	= Length( cartesian );
		const T	len		= Equals( radius, T(0) ) ? T(1) : radius;

		SphericalTempl<T>	spherical{	ACos( cartesian.z / len ),
										ATan( cartesian.y, len )};
		return { spherical, radius };
	}
	
/*
=================================================
	ToCartesian
=================================================
*/
	template <typename T>
	inline Vec<T,3>  SphericalTempl<T>::ToCartesian () const
	{
		Vec<T,3>	cartesian;

#	if 0
		const T		sin_theta = Sin( theta );
		cartesian.x = sin_theta * Cos( phi );
		cartesian.y = sin_theta * Sin( phi );
		cartesian.z = Cos( theta );
#	else
		const T		sin_phi = Sin( phi );
		cartesian.x = sin_phi * Cos( theta );
		cartesian.y = sin_phi * Sin( theta );
		cartesian.z = Cos( phi );
#	endif

		return cartesian;
	}

	template <typename T>
	inline Vec<T,3>  SphericalTempl<T>::ToCartesian (T radius) const
	{
		return ToCartesian() * radius;
	}


}	// FGC

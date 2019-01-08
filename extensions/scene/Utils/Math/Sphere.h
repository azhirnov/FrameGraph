// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Utils/Math/GLM.h"

namespace FG
{
	template <typename T> struct AxisAlignedBoundingBox;



	//
	// Sphere
	//

	template <typename T>
	struct BoundingSphere
	{
	// types
		using Self		= BoundingSphere<T>;
		using Vec3_t	= glm::tvec3<T>;
		using Value_t	= T;


	// variables
		Vec3_t		center	= {T(0)};
		Value_t		radius	= T(0);


	// methods
		BoundingSphere () {}
		BoundingSphere (const Vec3_t &center, T radius) : center{center}, radius{abs(radius)} {}


		Self&  Move (const Vec3_t &delta)		{ center += delta;  return *this; }


		ND_ bool  IsIntersects (const BoundingSphere<T> &other) const
		{
			const T		dist = radius + other.radius;
			constexpr T	err  = std::numeric_limits<T>::epsilon();

			return	distance2(center, other.center) < (dist * dist + err);
		}

		ND_ bool  IsIntersects (const AxisAlignedBoundingBox<T> &aabb) const;

		ND_ AxisAlignedBoundingBox<T>  ToAABB () const;
	};


	using Sphere = BoundingSphere<float>;

}	// FG

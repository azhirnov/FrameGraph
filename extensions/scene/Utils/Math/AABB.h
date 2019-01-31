// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Utils/Math/Sphere.h"

namespace FG
{

	//
	// Axis-Aligned Bounding Box
	//

	template <typename T>
	struct AxisAlignedBoundingBox
	{
	// types
		using Self		= AxisAlignedBoundingBox<T>;
		using Vec3_t	= glm::tvec3<T>;
		using Value_t	= T;


	// variables
		Vec3_t		min;
		Vec3_t		max;


	// methods
		AxisAlignedBoundingBox () : min{T(0)}, max{T(0)} {}
		explicit AxisAlignedBoundingBox (const Vec3_t &point) : min{point}, max{point} {}


		ND_ Vec3_t	Center ()		const	{ return (max + min) * T(0.5); }
		ND_ Vec3_t	Extent ()		const	{ return (max - min); }
		ND_ Vec3_t	HalfExtent ()	const	{ return Extent() * T(0.5); }
		ND_ bool	Empty ()		const	{ return all( max == min ); }


		ND_ bool  IsIntersects (const Self &other) const
		{
			return not( max.x < other.min.x or max.y < other.min.y or max.z < other.min.z or
						min.x > other.max.x or min.y > other.max.y or min.z > other.max.z );
		}

		ND_ bool  IsIntersects (const BoundingSphere<T> &sphere) const
		{
			T			dist = sphere.radius * sphere.radius;
			constexpr T	err  = std::numeric_limits<T>::epsilon();

			dist -= (sphere.center.x < min.x ? Square( sphere.center.x - min.x ) : Square( sphere.center.x - max.x ));
			dist -= (sphere.center.y < min.y ? Square( sphere.center.y - min.y ) : Square( sphere.center.y - max.y ));
			dist -= (sphere.center.z < min.z ? Square( sphere.center.z - min.z ) : Square( sphere.center.z - max.z ));

			return dist > -err;
		}

		ND_ bool  IsIntersects (const Vec3_t &point) const
		{
			return all( point > min ) and all( point < max );
		}


		ND_ BoundingSphere<T>  ToInnerSphere () const
		{
			const Vec3_t	side = Extent();
			const Value_t	r    = T(0.5) * Min( side.x, side.y, side.z );
			return BoundingSphere<T>{ Center(), r };
		}

		ND_ BoundingSphere<T>  ToOuterSphere () const
		{
			constexpr T		sq3_div2 = T(0.86602540378443864676372317075294);
			const Vec3_t	side	 = Extent();
			const Value_t	r        = sq3_div2 * Max( side.x, side.y, side.z );
			return BoundingSphere<T>{ Center(), r };
		}


		Self&  Move (const Vec3_t &delta)
		{
			min += delta;
			max += delta;
			return *this;
		}
		
		Self&  Repair ()
		{
			if ( min.x > max.x )	std::swap( min.x, max.x );
			if ( min.y > max.y )	std::swap( min.y, max.y );
			if ( min.z > max.z )	std::swap( min.z, max.z );
			return *this;
		}

		Self&  Add (const Vec3_t &point)
		{
			this->min = glm::min( point, this->min );
			this->max = glm::max( point, this->max );
			return *this;
		}

		Self&  Add (const Self &aabb)
		{
			return Add( aabb.min ).Add( aabb.max );
		}

		Self&  SetExtent (const Vec3_t &extent)
		{
			Vec3_t	center		= Center();
			Vec3_t	halfextent	= extent * T(0.5);

			min = center - halfextent;
			max = center + halfextent;
			return *this;
		}

		Self&  SetCenter (const Vec3_t &center)
		{
			Vec3_t	halfextent = HalfExtent();
			min = center - halfextent;
			max = center + halfextent;
			return *this;
		}

		Self&  Transform (const Transformation<T> &tr)
		{
			Vec3_t	point = min;	min = max = tr.ToGlobalPosition( point );
			point.z = max.z;		Add( tr.ToGlobalPosition( point ));
			point.y = max.y;		Add( tr.ToGlobalPosition( point ));
			point.z = min.z;		Add( tr.ToGlobalPosition( point ));
			point.x = max.x;		Add( tr.ToGlobalPosition( point ));
			point.z = max.z;		Add( tr.ToGlobalPosition( point ));
			point.y = min.y;		Add( tr.ToGlobalPosition( point ));
			point.z = min.z;		Add( tr.ToGlobalPosition( point ));

			return *this;
		}
	};


	using AABB = AxisAlignedBoundingBox<float>;

	
	template <typename T>
	inline AxisAlignedBoundingBox<T>  BoundingSphere<T>::ToAABB () const
	{
		AxisAlignedBoundingBox<T>	result;
		result.min = center - radius;
		result.max = center + radius;
		return result;
	}
	
	template <typename T>
	inline bool  BoundingSphere<T>::IsIntersects (const AxisAlignedBoundingBox<T> &aabb) const
	{
		return aabb.IsIntersects( *this );
	}

}	// FG

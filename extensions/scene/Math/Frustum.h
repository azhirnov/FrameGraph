// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Math/AABB.h"
#include "scene/Math/OOBB.h"
#include "scene/Math/Sphere.h"
#include "scene/Math/Camera.h"

namespace FGC
{

	//
	// Frustum
	//

	template <typename T>
	struct FrustumTempl
	{
	// types
	public:
		using Self		= FrustumTempl<T>;
		using Vec2_t	= glm::tvec2<T>;
		using Vec3_t	= glm::tvec3<T>;
		using Value_t	= T;
		
		enum class EPlane
		{
			Near,
			Far,
			Left,
			Right,
			Top,
			Bottom,
			_Count
		};
		
	private:
		enum class ESide
		{
			None	= 0,
			Positive,
			Negative,
			Both,
		};

		struct Plane
		{
			Vec3_t		norm;
			Value_t		dist;
		};


	// variables
	private:
		StaticArray< Plane, uint(EPlane::_Count) >		_planes;

		DEBUG_ONLY( bool								_initialized; )

		static constexpr T								_err	= std::numeric_limits<T>::epsilon();


	// methods
	public:
		FrustumTempl () {}

		void Setup (const CameraTempl<T> &);
		void Setup (const CameraTempl<T> &, const Vec2_t &range);
		
		ND_ bool IsVisible (const BoundingSphere<T> &) const;
		ND_ bool IsVisible (const AxisAlignedBoundingBox<T> &) const;
		ND_ bool IsVisible (const ObjectOrientedBoundingBox<T> &) const;
		ND_ bool IsVisible (const Vec3_t &point) const;
		ND_ bool IsVisible (const FrustumTempl<T> &) const;

		// experimental
			void Test (const AxisAlignedBoundingBox<T> &, OUT bool &isVisible, OUT float &detailLevel) const;

		ND_ Plane const&  GetPlane (EPlane type) const		{ return _planes[ uint(type) ]; }
		
		ND_ AxisAlignedBoundingBox<T>  ToAABB () const;

		bool GetRays (OUT Vec3_t &leftTop, OUT Vec3_t &leftBottom, OUT Vec3_t &rightTop, OUT Vec3_t &rightBottom) const;

		ND_ Vec3_t  GetRay (const Vec2_t &unormCoord) const;


	private:
		void _SetPlane (EPlane type, T a, T b, T c, T d);
		bool _GetIntersection (EPlane lhs, EPlane rhs, OUT Vec3_t &result) const;
		void _GetCorners (OUT StaticArray<Vec3_t, 8> &) const;

		Vec3_t _IntersectPlanes (EPlane p0, EPlane p1, EPlane p2) const;
	};


	using Frustum  = FrustumTempl<float>;
	
	
/*
=================================================
	Setup
=================================================
*/
	template <typename T>
	inline void  FrustumTempl<T>::Setup (const CameraTempl<T> &camera)
	{
		auto	mat = camera.projection * camera.transform.ToMatrix();

		_SetPlane( EPlane::Top,    mat[0][3] - mat[0][1], mat[1][3] - mat[1][1], mat[2][3] - mat[2][1], -mat[3][3] + mat[3][1] );
		_SetPlane( EPlane::Bottom, mat[0][3] + mat[0][1], mat[1][3] + mat[1][1], mat[2][3] + mat[2][1], -mat[3][3] - mat[3][1] );
		_SetPlane( EPlane::Left,   mat[0][3] + mat[0][0], mat[1][3] + mat[1][0], mat[2][3] + mat[2][0], -mat[3][3] - mat[3][0] );
		_SetPlane( EPlane::Right,  mat[0][3] - mat[0][0], mat[1][3] - mat[1][0], mat[2][3] - mat[2][0], -mat[3][3] + mat[3][0] );
		_SetPlane( EPlane::Near,   mat[0][3] + mat[0][2], mat[1][3] + mat[1][2], mat[2][3] + mat[2][2], -mat[3][3] - mat[3][2] );
		_SetPlane( EPlane::Far,    mat[0][3] - mat[0][2], mat[1][3] - mat[1][2], mat[2][3] - mat[2][2], -mat[3][3] + mat[3][2] );

		DEBUG_ONLY( _initialized = true );
	}
	
/*
=================================================
	Setup
=================================================
*/
	template <typename T>
	inline void  FrustumTempl<T>::Setup (const CameraTempl<T> &camera, const Vec2_t &)
	{
		// temp
		Setup( camera );
	}

/*
=================================================
	_SetPlane
=================================================
*/
	template <typename T>
	inline void  FrustumTempl<T>::_SetPlane (EPlane type, T a, T b, T c, T d)
	{
		const T	len		= length(Vec3_t{ a, b, c });
		const T	inv_len	= Equals( len, T(0), _err ) ? T(1) : (T(1) / len); 

		_planes[ uint(type) ] = { Vec3_t{a, b, c} * inv_len, abs(d) * inv_len };
	}
	
/*
=================================================
	IsVisible (Point)
=================================================
*/
	template <typename T>
	inline bool  FrustumTempl<T>::IsVisible (const Vec3_t &point) const
	{
		ASSERT( _initialized );

		bool	inside = true;

		for (auto& plane : _planes)
		{
			inside &= (dot( plane.norm, point ) + plane.dist) > -_err;
		}
		return inside;
	}

/*
=================================================
	IsVisible (Sphere)
=================================================
*/
	template <typename T>
	inline bool  FrustumTempl<T>::IsVisible (const BoundingSphere<T> &sphere) const
	{
		ASSERT( _initialized );

		bool	inside	= true;
		const T	r		= -(sphere.radius + _err);

		for (auto& plane : _planes)
		{
			inside &= (dot( plane.norm, sphere.center ) + plane.dist) > r;
		}
		return inside;
	}

/*
=================================================
	IsVisible (AABB)
=================================================
*/
	template <typename T>
	inline bool  FrustumTempl<T>::IsVisible (const AxisAlignedBoundingBox<T> &aabb) const
	{
		ASSERT( _initialized );

		auto	center		= aabb.Center();
		auto	halfextent	= aabb.HalfExtent();
		bool	inside		= true;

		for (auto& plane : _planes)
		{
			const T	d = Max(aabb.min.x * plane.norm.x, aabb.max.x * plane.norm.x) +
						Max(aabb.min.y * plane.norm.y, aabb.max.y * plane.norm.y) +
						Max(aabb.min.z * plane.norm.z, aabb.max.z * plane.norm.z) +
						plane.dist;
			inside &= (d > -_err);
			/*
			const T	d	= dot( plane.norm, center ) + plane.dist;
			const T	m	= abs( plane.norm.x * halfextent.x ) + abs( plane.norm.y * halfextent.y ) + abs( plane.norm.z * halfextent.z );

			inside &= (d > -(m + _err));*/
		}
		return inside;
	}
	
/*
=================================================
	Test (AABB)
=================================================
*/
	template <typename T>
	inline void  FrustumTempl<T>::Test (const AxisAlignedBoundingBox<T> &aabb, OUT bool &isVisible, OUT float &detailLevel) const
	{
		ASSERT( _initialized );
		
		auto	center		= aabb.Center();
		auto	halfextent	= aabb.HalfExtent();
		T		distances [uint(EPlane::_Count)] = {};
		
		isVisible = true;
		for (size_t i = 0; i < _planes.size(); ++i)
		{
			auto&	plane	= _planes[i];
			const T	d		= dot( plane.norm, center ) + plane.dist;
			const T	m		= abs( plane.norm.x * halfextent.x ) + abs( plane.norm.y * halfextent.y ) + abs( plane.norm.z * halfextent.z );
			distances[i]	= m;
			isVisible		&= (d > -(m + _err));
		}
		
		float sideX = Max( distances[2] / _planes[2].dist, distances[3] / _planes[3].dist ) * 2.0f;
		float sideY = Max( distances[4] / _planes[4].dist, distances[5] / _planes[5].dist ) * 2.0f;
		float sideZ = distances[0] / _planes[0].dist * 2.0f;
		detailLevel = Max( sideX, sideY, sideZ );
	}

/*
=================================================
	IsVisible (OOBB)
=================================================
*/
	template <typename T>
	inline bool  FrustumTempl<T>::IsVisible (const ObjectOrientedBoundingBox<T> &oobb) const
	{
		return true;	// TODO
	}
	
/*
=================================================
	IsVisible (Frustum)
----
	from http://www.yosoygames.com.ar/wp/2016/12/frustum-vs-pyramid-intersection-also-frustum-vs-frustum/
=================================================
*/
	template <typename T>
	inline bool  FrustumTempl<T>::IsVisible (const FrustumTempl<T> &frustum) const
	{
		ASSERT( _initialized );

		StaticArray<Vec3_t, 8>	lhs_corners;	this->_GetCorners( OUT lhs_corners );
		StaticArray<Vec3_t, 8>	rhs_corners;	frustum._GetCorners( OUT rhs_corners );
		bool					inside	= true;

		for (uint j = 0; j < _planes.size(); ++j)
		{
			bool	lhs_any_point_inside	= false;
			bool	rhs_any_point_inside	= false;
			auto&	lhs_plane				= this->_planes[j];
			auto&	rhs_plane				= frustum._planes[j];

			for (uint i = 0; i < lhs_corners.size(); ++i)
			{
				auto&	lhs_point	= lhs_corners[i];
				auto&	rhs_point	= rhs_corners[i];

				lhs_any_point_inside |= (dot( lhs_plane.norm, rhs_point ) + lhs_plane.dist > -_err);
				rhs_any_point_inside |= (dot( rhs_plane.norm, lhs_point ) + rhs_plane.dist > -_err);
			}

			inside &= lhs_any_point_inside;
			inside &= rhs_any_point_inside;
		}
		return inside;
	}
	
/*
=================================================
	ToAABB
=================================================
*/
	template <typename T>
	inline AxisAlignedBoundingBox<T>  FrustumTempl<T>::ToAABB () const
	{
		ASSERT( _initialized );

		StaticArray<Vec3_t, 8>		points;		_GetCorners( OUT points );
		AxisAlignedBoundingBox<T>	result{ points[0] };

		for (size_t i = 1; i < points.size(); ++i) {
			result.Add( points[i] );
		}
		return result;
	}

/*
=================================================
	_GetCorners
=================================================
*/
	template <typename T>
	inline void  FrustumTempl<T>::_GetCorners (OUT StaticArray<Vec3_t, 8> &result) const
	{
		result[0] = _IntersectPlanes( EPlane::Near, EPlane::Left,  EPlane::Bottom );
		result[1] = _IntersectPlanes( EPlane::Near, EPlane::Left,  EPlane::Top    );
		result[2] = _IntersectPlanes( EPlane::Near, EPlane::Right, EPlane::Top    );
		result[3] = _IntersectPlanes( EPlane::Near, EPlane::Right, EPlane::Bottom );
		result[4] = _IntersectPlanes( EPlane::Far,  EPlane::Left,  EPlane::Bottom );
		result[5] = _IntersectPlanes( EPlane::Far,  EPlane::Left,  EPlane::Top    );
		result[6] = _IntersectPlanes( EPlane::Far,  EPlane::Right, EPlane::Top    );
		result[7] = _IntersectPlanes( EPlane::Far,  EPlane::Right, EPlane::Bottom );
	}
	
/*
=================================================
	_IntersectPlanes
=================================================
*/
	template <typename T>
	inline typename FrustumTempl<T>::Vec3_t
		FrustumTempl<T>::_IntersectPlanes (EPlane p0, EPlane p1, EPlane p2) const
	{
		auto&	P0	= _planes[ uint(p0) ];
		auto&	P1	= _planes[ uint(p1) ];
		auto&	P2	= _planes[ uint(p2) ];

		Vec3_t	bxc	= cross( P1.norm, P2.norm );
		Vec3_t	cxa	= cross( P2.norm, P0.norm );
		Vec3_t	axb	= cross( P0.norm, P1.norm );
		Vec3_t	r	= -P0.dist * bxc - P1.dist * cxa - P2.dist * axb;
		return r * (T(1) / dot(P0.norm, bxc));
	}

/*
=================================================
	GetRays
----
	returns frustum rays for ray tracing
=================================================
*/
	template <typename T>
	inline bool  FrustumTempl<T>::GetRays (OUT Vec3_t &leftTop, OUT Vec3_t &leftBottom, OUT Vec3_t &rightTop, OUT Vec3_t &rightBottom) const
	{
		ASSERT( _initialized );

		return	_GetIntersection( EPlane::Bottom, EPlane::Left,   OUT leftBottom  ) &
				_GetIntersection( EPlane::Left,   EPlane::Top,    OUT leftTop     ) &
				_GetIntersection( EPlane::Right,  EPlane::Bottom, OUT rightBottom ) &
				_GetIntersection( EPlane::Top,    EPlane::Right,  OUT rightTop    );
	}
	
/*
=================================================
	GetRay
=================================================
*/
	template <typename T>
	inline typename FrustumTempl<T>::Vec3_t  FrustumTempl<T>::GetRay (const Vec2_t &unormCoord) const
	{
		Vec3_t	left_bottom, left_top, right_bottom, right_top;
		_GetIntersection( EPlane::Bottom, EPlane::Left,   OUT left_bottom  );
		_GetIntersection( EPlane::Left,   EPlane::Top,    OUT left_top     );
		_GetIntersection( EPlane::Right,  EPlane::Bottom, OUT right_bottom );
		_GetIntersection( EPlane::Top,    EPlane::Right,  OUT right_top    );

		const Vec3_t	vec	= mix(  mix( left_bottom, right_bottom, unormCoord.x ),
									mix( left_top, right_top, unormCoord.x ),
									unormCoord.y );

		return normalize( vec );
	}

/*
=================================================
	_GetIntersection
=================================================
*/
	template <typename T>
	inline bool  FrustumTempl<T>::_GetIntersection (EPlane lhs, EPlane rhs, OUT Vec3_t &result) const
	{
		auto&	lp = _planes[ uint(lhs) ];
		auto&	rp = _planes[ uint(rhs) ];

		auto	dir = cross( lp.norm, rp.norm );
		auto	len = length2( dir );

		if ( Equals( len, T(0), _err ) )
			return false;

		result = dir * (T(1) / Sqrt(len));
		return true;
	}


}	// FGC

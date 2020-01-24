// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Math/Transform.h"
#include "scene/Math/Radians.h"
#include "stl/Math/Rectangle.h"

namespace FGC
{

	//
	// Camera
	//

	template <typename T>
	struct CameraTempl
	{
	// types
		using Transform_t	= Transformation<T>;
		using Mat4_t		= typename Transform_t::Mat4_t;
		using Vec3_t		= typename Transform_t::Vec3_t;
		using Quat_t		= typename Transform_t::Quat_t;
		using Rect_t		= Rectangle<T>;
		using Vec2_t		= glm::tvec2<T>;
		using Radians_t		= RadiansTempl<T>;
		using Self			= CameraTempl<T>;
		using Value_t		= T;


	// variables
		Transform_t		transform;
		Mat4_t			projection;


	// methods
		CameraTempl () {}
		
		ND_ Mat4_t  ToModelViewProjMatrix ()	const	{ return projection * transform.ToMatrix(); }
		ND_ Mat4_t	ToViewProjMatrix ()			const	{ return projection * transform.ToRotationMatrix(); }
		ND_ Mat4_t	ToViewMatrix ()				const	{ return transform.ToRotationMatrix(); }


		// for transformation
		Self&	Move (const Vec3_t &delta)
		{
			transform.Move( delta );
			return *this;
		}

		Self&  SetPosition (const Vec3_t &value)
		{
			transform.position = value;
			return *this;
		}

		Self&	Rotate (const Quat_t &delta)
		{
			transform.Rotate( delta );
			return *this;
		}

		Self&  Rotate (Radians_t angle, const Vec3_t &normal)
		{
			transform.Rotate( glm::rotate( Quat_Identity, T(angle), normal ));
			return *this;
		}

		Self&  SetOrientation (const Quat_t &value)
		{
			transform.orientation = value;
			return *this;
		}


		// for projection
		Self&  SetOrtho (const Rect_t &viewport, const Vec2_t &range)
		{
			projection = glm::ortho( viewport.left, viewport.right, viewport.bottom, viewport.top, range[0], range[1] );
			return *this;
		}

		Self&  SetPerspective (Radians_t fovY, Value_t aspect, Value_t zNear)
		{
			projection = glm::infinitePerspective( T(fovY), aspect, zNear );
			return *this;
		}

		Self&  SetPerspective (Radians_t fovY, Value_t aspect, const Vec2_t &range)
		{
			projection = glm::perspective( T(fovY), aspect, range[0], range[1] );
			return *this;
		}

		Self&  SetPerspective (Radians_t fovY, const Vec2_t &viewport, const Vec2_t &range)
		{
			projection = glm::perspectiveFov( T(fovY), viewport.x, viewport.y, range[0], range[1] );
			return *this;
		}

		Self&  SetFrustum (const Rect_t &viewport, const Vec2_t &range)
		{
			projection = glm::frustum( viewport.left, viewport.right, viewport.bottom, viewport.top, range[0], range[1] );
			return *this;
		}
	};


	using Camera  = CameraTempl<float>;


}	// FGC

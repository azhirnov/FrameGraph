// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Math/Camera.h"
#include "scene/Math/Frustum.h"

namespace FGC
{

	//
	// VR Camera
	//

	struct VRCamera final
	{
	// types
	private:
		using Transform_t	= Transformation<float>;
		using Mat4_t		= typename Transform_t::Mat4_t;
		using Mat3_t		= glm::tmat4x4< typename Transform_t::Value_t >;
		using Vec3_t		= typename Transform_t::Vec3_t;
		using Quat_t		= typename Transform_t::Quat_t;
		using Vec2_t		= glm::tvec2<float>;
		using Mat4Pair_t	= StaticArray< Mat4_t, 2 >;
		using Radians_t		= RadiansTempl<float>;
		using FrustumRef_t	= StaticArray< Frustum const*, 2 >;
		using Self			= VRCamera;

		struct PerEye
		{
			Mat4_t		viewProj;
			Frustum		frustum;
			Mat4_t		proj;
			Mat4_t		view;
		};

	// variables
	private:
		PerEye			_perEye[2];
		Mat4_t			_devicePose;
		Vec3_t			_devicePosition;
		float			_deviceScale	= 1.0f;
		Vec3_t			_worldPosition;
		Transform_t		_transform;


	// methods
	public:
		VRCamera () {}

		void SetViewProjection (const Mat4_t &leftProj, const Mat4_t &leftView,
								const Mat4_t &rightProj, const Mat4_t &rightView,
								const Mat4_t &devicePose, const Vec3_t &position);
		
		ND_ Mat4Pair_t  ToModelViewProjMatrix () const;
		ND_ Mat4Pair_t  ToModelViewMatrix () const;
		ND_ Mat4Pair_t  ToViewProjMatrix () const;
		ND_ Mat4Pair_t  ToProjectionMatrix () const;
		ND_ Mat4Pair_t  ToViewMatrix () const;

		ND_ FrustumRef_t  GetFrustum () const;

		// for transformation
		Self&  Move (const Vec3_t &delta);
		Self&  Move2 (const Vec3_t &delta);

		ND_ Vec3_t const&	Position ()			const	{ return _transform.position; }
		ND_ Quat_t const&	Orientation ()		const	{ return _transform.orientation; }
		ND_ Vec3_t const&	HmdOffset ()		const	{ return _devicePosition; }
		ND_ Vec3_t const&	PositionInWorld ()	const	{ return _worldPosition; }
		ND_ Mat4_t			ToRotationMatrix () const	{ return _devicePose * _transform.ToRotationMatrix(); }
		ND_ Mat4_t			ToModelMatrix ()	const	{ return _devicePose * _transform.ToMatrix(); }

		Self&  Rotate (const Quat_t &delta);
		Self&  Rotate (Radians_t angle, const Vec3_t &normal);

		Self&  SetOrientation (const Quat_t &value);
		Self&  SetPosition (const Vec3_t &value);
		Self&  SetHmdOffsetScale (float value);
	};
	

/*
=================================================
	SetViewProjection
=================================================
*/
	inline void VRCamera::SetViewProjection (const Mat4_t &leftProj, const Mat4_t &leftView,
											 const Mat4_t &rightProj, const Mat4_t &rightView,
											 const Mat4_t &devicePose, const Vec3_t &position)
	{
		_devicePose		= devicePose;
		_devicePosition	= position;

		_perEye[0].proj		= leftProj;
		_perEye[0].view		= leftView;
		_perEye[0].viewProj	= leftProj * leftView;

		_perEye[1].proj		= rightProj;
		_perEye[1].view		= rightView;
		_perEye[1].viewProj	= rightProj * rightView;

		Mat4Pair_t	mvp = ToViewProjMatrix();
		_perEye[0].frustum.Setup( mvp[0] );
		_perEye[1].frustum.Setup( mvp[1] );
	}
		
/*
=================================================
	To***Matrix
=================================================
*/
	inline typename VRCamera::Mat4Pair_t  VRCamera::ToModelViewProjMatrix () const
	{
		const Mat4_t mat = ToModelMatrix();
		return {{ _perEye[0].viewProj * mat, _perEye[1].viewProj * mat }};
	}

	inline typename VRCamera::Mat4Pair_t  VRCamera::ToModelViewMatrix () const
	{
		const Mat4_t mat = ToModelMatrix();
		return {{ _perEye[0].view * mat, _perEye[1].view * mat }};
	}

	inline typename VRCamera::Mat4Pair_t  VRCamera::ToViewProjMatrix () const
	{
		const Mat4_t mat = ToRotationMatrix();
		return {{ _perEye[0].viewProj * mat, _perEye[1].viewProj * mat }};
	}
	
	inline typename VRCamera::Mat4Pair_t  VRCamera::ToViewMatrix () const
	{
		const Mat4_t mat = ToRotationMatrix();
		return {{ _perEye[0].view * mat, _perEye[1].view * mat }};
	}
	
	inline typename VRCamera::Mat4Pair_t  VRCamera::ToProjectionMatrix () const
	{
		return {{ _perEye[0].proj, _perEye[1].proj }};
	}
	
/*
=================================================
	GetFrustum
=================================================
*/
	inline typename VRCamera::FrustumRef_t  VRCamera::GetFrustum () const
	{
		return {{ &_perEye[0].frustum, &_perEye[1].frustum }};
	}
	
/*
=================================================
	SetPosition
=================================================
*/
	inline VRCamera&  VRCamera::SetPosition (const Vec3_t &value)
	{
		_worldPosition = value;
		_transform.position = _devicePosition * _deviceScale + _worldPosition;
		return *this;
	}
	
/*
=================================================
	SetHmdOffsetScale
=================================================
*/
	inline VRCamera&  VRCamera::SetHmdOffsetScale (float value)
	{
		_deviceScale = value;
		return *this;
	}

/*
=================================================
	Rotate
=================================================
*/
	inline VRCamera&  VRCamera::Rotate (const Quat_t &delta)
	{
		_transform.Rotate( delta );
		return *this;
	}

	inline VRCamera&  VRCamera::Rotate (Radians_t angle, const Vec3_t &normal)
	{
		_transform.Rotate( glm::rotate( Quat_Identity, float(angle), normal ));
		return *this;
	}
	
/*
=================================================
	SetOrientation
=================================================
*/
	inline VRCamera&  VRCamera::SetOrientation (const Quat_t &value)
	{
		_transform.orientation = value;
		return *this;
	}

/*
=================================================
	Move
----
	x - forward/backward
	y - side
	z - up/down
=================================================
*/
	inline VRCamera&  VRCamera::Move (const vec3 &delta)
	{
		const mat4x4	view_mat	= ToRotationMatrix();
		const vec3		up_dir		{ 0.0f, 1.0f, 0.0f };
		const vec3		axis_x		{ view_mat[0][0], view_mat[1][0], view_mat[2][0] };
		const vec3		forwards	= normalize( cross( up_dir, axis_x ));
		vec3			pos			= _worldPosition;

		pos += forwards *  delta.x;
		pos += axis_x   * -delta.y;
		pos += up_dir   *  delta.z;

		return SetPosition( pos );
	}

/*
=================================================
	Move2
----
	x - forward/backward
	y - side
	z - up/down
=================================================
*/
	inline VRCamera&  VRCamera::Move2 (const vec3 &delta)
	{
		const mat4x4	view_mat	= ToRotationMatrix();
		const vec3		up_dir		{ 0.0f, 1.0f, 0.0f };
		const vec3		axis_x		{ view_mat[0][0], view_mat[1][0], view_mat[2][0] };
		const vec3		axis_z		{ view_mat[0][2], view_mat[1][2], view_mat[2][2] };
		vec3			pos			= _worldPosition;
		
		pos += axis_z * -delta.x;
		pos += axis_x * -delta.y;
		pos += up_dir *  delta.z;
		
		return SetPosition( pos );
	}

}	// FGC

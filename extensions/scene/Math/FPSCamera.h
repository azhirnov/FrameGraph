// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "scene/Math/Camera.h"
#include "scene/Math/Frustum.h"

namespace FGC
{

	//
	// Camera for first person shooter
	//

	struct FPSCamera final
	{
	// variables
	private:
		Camera		_camera;
		Frustum		_frustum;


	// methods
	public:
		FPSCamera () {}

		ND_ Camera const&	GetCamera ()	const	{ return _camera; }
		ND_ Frustum const&	GetFrustum ()	const	{ return _frustum; }

		void SetPerspective (Rad fovY, float aspect, float zNear, float zFar);

		void Rotate (float horizontal, float vertical);

		void Move (const vec3 &delta);
		void Move2 (const vec3 &delta);
		void SetPosition (const vec3 &pos);
	};
	
	
/*
=================================================
	SetPerspective
=================================================
*/
	inline void  FPSCamera::SetPerspective (Rad fovY, float aspect, float zNear, float zFar)
	{
		_camera.SetPerspective( fovY, aspect, vec2{zNear, zFar} );
		_frustum.Setup( _camera );
	}

/*
=================================================
	Rotate
=================================================
*/
	inline void  FPSCamera::Rotate (float horizontal, float vertical)
	{
		quat&	q		= _camera.transform.orientation;
		bool	has_ver	= not Equals( vertical, 0.0f );
		bool	has_hor	= not Equals( horizontal, 0.0f );

		if ( not (has_hor or has_ver) )
			return;

		if ( has_ver )
			q = quat{ cos(vertical * 0.5f), sin(vertical * 0.5f), 0.0f, 0.0f } * q;

		if ( has_hor )
			q = q * quat{ cos(horizontal * 0.5f), 0.0f, sin(horizontal * 0.5f), 0.0f };

		q = normalize( q );

		_frustum.Setup( _camera );
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
	inline void  FPSCamera::Move (const vec3 &delta)
	{
		const mat4x4	view_mat	= _camera.ToViewMatrix();
		const vec3		up_dir		{ 0.0f, 1.0f, 0.0f };
		const vec3		axis_x		{ view_mat[0][0], view_mat[1][0], view_mat[2][0] };
		const vec3		forwards	= normalize( cross( up_dir, axis_x ));
		auto&			pos			= _camera.transform.position;

		pos += forwards * delta.x;
		pos += axis_x   * delta.y;
		pos += up_dir   * delta.z;
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
	inline void  FPSCamera::Move2 (const vec3 &delta)
	{
		const mat4x4	view_mat	= _camera.ToViewMatrix();
		const vec3		up_dir		{ 0.0f, 1.0f, 0.0f };
		const vec3		axis_x		{ view_mat[0][0], view_mat[1][0], view_mat[2][0] };
		const vec3		axis_z		{ view_mat[0][2], view_mat[1][2], view_mat[2][2] };
		auto&			pos			= _camera.transform.position;
		
		pos += axis_z * -delta.x;
		pos += axis_x *  delta.y;
		pos += up_dir *  delta.z;
	}
	
/*
=================================================
	SetPosition
=================================================
*/
	inline void  FPSCamera::SetPosition (const vec3 &pos)
	{
		_camera.transform.position = pos;
	}


}	// FGC

/*
	Quaternion functions
*/

#include "Math.glsl"


struct quat
{
	float4	data;
};


quat	QIdentity ();
quat	QCreate (const float4 vec);

quat	QNormalize (const quat q);
quat	QInverse (const quat q);

quat	QMul (const quat left, const quat right);
float3	QMul (const quat left, const float3 right);

float	QDot (const quat left, const quat right);
quat	QSlerp (const quat qx, const quat qy, const float factor);

float3	QDirection (const quat q);

quat	QRotationX (const float angleRad);
quat	QRotationY (const float angleRad);
quat	QRotationZ (const float angleRad);
quat	QRotation (const float3 anglesRad);


//-----------------------------------------------------------------------------


/*
=================================================
	QIdentity
=================================================
*/
quat QIdentity ()
{
	quat	ret;
	ret.data = float4( 0.0, 0.0, 0.0, 1.0 );
	return ret;
}

/*
=================================================
	QCreate
=================================================
*/
quat QCreate (const float4 vec)
{
	quat	ret;
	ret.data = vec;
	return ret;
}

/*
=================================================
	QNormalize
=================================================
*/
quat QNormalize (const quat q)
{
	quat	ret = q;
	float	n	= Dot( q.data, q.data );
	
	if ( n == 1.0 )
		return ret;
	
	ret.data /= Sqrt( n );
	return ret;
}

/*
=================================================
	QInverse
=================================================
*/
quat QInverse (const quat q)
{
	quat	ret;
	ret.data.xyz = -q.data.xyz;
	ret.data.w   = q.data.w;
	return ret;
}

/*
=================================================
	QMul
=================================================
*/
quat QMul (const quat left, const quat right)
{
	quat	ret;

	ret.data.xyz	= left.data.w * right.data.xyz + 
					  left.data.xyz * right.data.w +
					  Cross( left.data.xyz, right.data.xyz );

	float4	dt		= left.data.xyzw * right.data.xyzw;
	ret.data.w		= Dot( dt, float4( -1.0, -1.0, -1.0, 1.0 ) );

	return ret;
}

/*
=================================================
	QMul
=================================================
*/
float3 QMul (const quat left, const float3 right)
{
	float3	q	= left.data.xyz;
	float3	uv	= Cross( q, right );
	float3	uuv	= Cross( q, uv );

	return right + ((uv * left.data.w) + uuv) * 2.0;
}

/*
=================================================
	QDot
=================================================
*/
float QDot (const quat left, const quat right)
{
	return Dot( left.data, right.data );
}

/*
=================================================
	QSlerp
=================================================
*/
quat QSlerp (const quat qx, const quat qy, const float factor)
{
	quat	ret;
	float4	qz			= qy.data;
	float	cos_theta	= Dot( qx.data, qy.data );

	if ( cos_theta < 0.0 )
	{
		qz			= -qy.data;
		cos_theta	= -cos_theta;
	}

	if ( cos_theta > 1.0 - Epsilon() )
	{
		ret.data = Lerp( qx.data, qy.data, factor );
	}
	else
	{
		float	angle = ACos( cos_theta );

		ret.data =	( Sin( (1.0 - factor) * angle ) * qx.data +
					  Sin( factor * angle ) * qz ) / Sin( angle );
	}
	return ret;
}

/*
=================================================
	QDirection
=================================================
*/
float3 QDirection (const quat q)
{
	return float3( 2.0 * q.data.x * q.data.z + 2.0 * q.data.y * q.data.w,
				  2.0 * q.data.z * q.data.y - 2.0 * q.data.x * q.data.w,
				  1.0 - 2.0 - q.data.x * q.data.x - 2.0 * q.data.y * q.data.y );
}

/*
=================================================
	QRotationX
=================================================
*/
quat QRotationX (const float angleRad)
{
	quat	q;
	float	a = angleRad * 0.5;

	q.data = float4( Sin(a), 0.0, 0.0, Cos(a) );
	return q;
}

/*
=================================================
	QRotationY
=================================================
*/
quat QRotationY (const float angleRad)
{
	quat	q;
	float	a = angleRad * 0.5;

	q.data = float4( 0.0, Sin(a), 0.0, Cos(a) );
	return q;
}

/*
=================================================
	QRotationZ
=================================================
*/
quat QRotationZ (const float angleRad)
{
	quat	q;
	float	a = angleRad * 0.5;

	q.data = float4( 0.0, 0.0, Sin(a), Cos(a) );
	return q;
}

/*
=================================================
	QRotation
=================================================
*/
quat QRotation (const float3 anglesRad)
{
	return QMul( QMul( QRotationX( anglesRad.x ), QRotationY( anglesRad.y ) ), QRotationZ( anglesRad.z ) );
}

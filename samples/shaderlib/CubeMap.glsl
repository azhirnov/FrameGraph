/*
	Cube map utilites
*/

#include "Math.glsl"

float3  CM_RotateVec (const float3 c, int face)
{
	switch ( face )
	{
		case 0 : return float3( c.z,  c.y, -c.x);	// X+
		case 1 : return float3(-c.z,  c.y,  c.x);	// X-
		case 2 : return float3( c.x, -c.z,  c.y);	// Y+
		case 3 : return float3( c.x,  c.z, -c.y);	// Y-
		case 4 : return float3( c.x,  c.y,  c.z);	// Z+
		case 5 : return float3(-c.x,  c.y, -c.z);	// Z-
	}
	return float3(0.0);
}

float4  CM_InverseRotation (const float3 c)
{
	// front (xy space)
	if ( (Abs(c.x) <= c.z) and (c.z > 0.0) and (Abs(c.y) <= c.z) )
		return float4( c.x, c.y, c.z, 4.0 );

	// right (zy space)
	if ( (Abs(c.z) <= c.x) and (c.x > 0.0) and (Abs(c.y) <= c.x) )
		return float4( -c.z, c.y, c.x, 0.0 );

	// back (xy space)
	if ( (Abs(c.x) <= -c.z) and (c.z < 0.0) and (Abs(c.y) <= -c.z) )
		return float4( -c.x, c.y, -c.z, 5.0 );

	// left (zy space)
	if ( (Abs(c.z) <= -c.x) and (c.x < 0.0) and (Abs(c.y) <= -c.x) )
		return float4( c.z, c.y, -c.x, 1.0 );

	// up (xz space)
	if ( c.y > 0.0 )
		return float4( c.x, c.z, c.y, 2.0 );

	// down (xz space)
	return float4( c.x, -c.z, -c.y, 3.0 );
}


// Identity Spherical Cube Projection
float3  CM_IdentitySC_Forward (const float2 snormCoord, int face)
{
	return Normalize( CM_RotateVec( float3(snormCoord, 1.0), face ));
}

float3  CM_IdentitySC_Inverse (const float3 coord)
{
	float4	coord_face = CM_InverseRotation( coord );
	coord_face.xy /= coord_face.z;
	return coord_face.xyw;
}


// Tangential Spherical Cube Projection
float3  CM_TangentialSC_Forward (const float2 snormCoord, int face)
{
	const float	warp_theta		= 0.868734829276;
	const float	tan_warp_theta	= 1.182286685546; //tan( warp_theta );
	float2		coord			= Tan( warp_theta * snormCoord ) / tan_warp_theta;

	return Normalize( CM_RotateVec( float3(coord.x, coord.y, 1.0), face ));
}

float3  CM_TangentialSC_Inverse (const float3 coord)
{
	const float	warp_theta		= 0.868734829276;
	const float	tan_warp_theta	= 1.182286685546; //tan( warp_theta );

	float4	coord_face = CM_InverseRotation( coord );
	coord_face.xy /= coord_face.z;

	float2	c = ATan( coord_face.xy * tan_warp_theta ) / warp_theta;
	return float3( c.xy, coord_face.w );
}

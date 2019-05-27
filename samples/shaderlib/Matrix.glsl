/*
	Matrix functions
*/

#include "Math.glsl"


float3x3  LookAt (const float3 dir, const float3 up)
{
	float3x3 m;
	m[2] = dir;
	m[0] = Normalize( Cross( up, m[2] ));
	m[1] = Cross( m[2], m[0] );
	return m;
}
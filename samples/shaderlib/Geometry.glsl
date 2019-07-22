/*
	Geometry functions
*/

#include "Math.glsl"


void GetRayPerpendicular (const float3 dir, out float3 outLeft, out float3 outUp)
{
	const float3	a	 = Abs( dir );
	const float2	c	 = float2( 1.0f, 0.0f );
	const float3	axis = a.x < a.y ? (a.x < a.z ? c.xyy : c.yyx) :
									   (a.y < a.z ? c.xyx : c.yyx);
		
	outLeft = Normalize( Cross( dir, axis ));
	outUp   = Normalize( Cross( dir, outLeft ));
}


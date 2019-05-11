/*
	Geometry functions
*/

#include "Math.glsl"


void GetRayPerpendicular (const vec3 dir, out vec3 outLeft, out vec3 outUp)
{
	const vec3	a	 = abs( dir );
	const vec2	c	 = vec2( 1.0f, 0.0f );
	const vec3	axis = a.x < a.y ? (a.x < a.z ? c.xyy : c.yyx) :
									(a.y < a.z ? c.xyx : c.yyx);
		
	outLeft = normalize(cross( dir, axis ));
	outUp   = normalize(cross( dir, outLeft ));
}


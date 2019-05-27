
#include "Math.glsl"

#define EXPERIMENTAL	0
#define QUAD			1
#define RHOMBUS			2
#define HEXAGON			3

#define SHAPE			HEXAGON

#if SHAPE == QUAD
const float2 points[] = {
	float2(  0.0,  0.0 ),
	float2(  2.0,  0.0 ),
	float2( -2.0,  0.0 ),
	float2(  0.0,  2.0 ),
	float2(  0.0, -2.0 )
};

#elif SHAPE == RHOMBUS
const float2 points[] = {
	float2(  0.0,  0.0 ),
	float2(  2.0,  2.0 ),
	float2( -2.0,  2.0 ),
	float2( -2.0, -2.0 ),
	float2(  2.0, -2.0 )
};

#elif SHAPE == HEXAGON
const float2 points[] = {
	float2(  0.0,  0.0 ),
	float2(  2.0,  4.0 ),
	float2(  3.0,  0.0 ),
	float2(  2.0, -4.0 ),
	float2( -2.0, -4.0 ),
	float2( -3.0,  0.0 ),
	float2( -2.0,  4.0 )
};

#else
const float2 points[] = {
	float2(  0.0,  0.0 ),
	float2(  2.0,  0.0 ),
	float2( -2.0,  0.0 ),
	float2(  0.0,  2.0 ),
	float2(  0.0, -2.0 ),
	float2(  1.0,  1.0 )
};
#endif


void mainImage (out float4 fragColor, in float2 fragCoord)
{
	float2	uv = ((fragCoord - iResolution.xy*0.5) / min(iResolution.x, iResolution.y)) * 10.0;
	float	md = -1.0e+10;

	for (int i = 1; i < points.length(); ++i)
	{
		float2	n = Normalize( points[0] - points[i] );

		md = Max( md, Dot( uv, n ));
	}

	float	w = 0.01; //fwidth( md );

	fragColor = float4( SmoothStep( 2.0+w, 2.0-w, md ));
}


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

	// draw shape
	//float	dtoc	= Distance( points[0], uv );	// distance to center
	float	md		= 1.0e+10;	// minimal distance to point
	float2	mr;					// vector from nearest point to pixel (uv)
	int		idx		= -1;		// index of nearest point

	for (int i = 0; i < points.length(); ++i)
	{
		float2	r = points[i] - uv;
		float	d = Length( r );

		if ( d < md )
		{
			md  = d;
			mr  = r;
			idx = i;
		}
	}

	if ( idx == 0 )
	{
		md = 1.0e+10;
		
		for (int i = 1; i < points.length(); ++i)
		{
			float2	r  = points[i] - uv;
			md = Min( md, Dot( 0.5*(mr + r), Normalize(r - mr) ) );
		}
		md = -md;
	}
	else
	{
		float2	r  = points[0] - uv;
		md = Dot( 0.5*(mr + r), Normalize(r - mr) );
	}

	//fragColor = float4( smoothstep( dtoc-0.2, dtoc, md ),
	//					smoothstep( 0.1, 0.05, md ),
	//					(dtoc - md) * 0.05,
	//				    1.0 );

	//fragColor = float4(abs(mr) * 0.1, 0.0, 1.0);
	//fragColor = float4( Abs(md - Abs(Sin(iTime))*3.0 ) < 0.02 ? 1.0 : 0.0 );
	//fragColor = float4( md * 0.1 );
	
	// isolines
	fragColor = (md + 2.0) * (0.5 + 0.5*Sin(64.0*md)) * float4(1.0);
}

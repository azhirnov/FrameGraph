
#include "Math.glsl"
#include "Hash.glsl"
#include "SDF.glsl"

#define EXPERIMENTAL	0
#define QUAD			1
#define RHOMBUS			2
#define HEXAGON			3
#define DIAMOND			4
#define RANDOM_1		1000
#define RANDOM_2		1001

#define SHAPE			RANDOM_1


float2 SDF_Plane2D (const float2 center, const float2 planePos, const float2 pos)
{
	float2	res;
	float2	v = planePos - center;
	float	d = Length( v );
	res.x = Dot( v / d, pos ) - d;
	res.y = SDF_Line( pos, center, planePos );
	return res;
}

float2 MD_Init ()
{
	return float2(-1.0e+10, 1.0e+10);
}

float2 Unite (const float2 a, const float2 b)
{
	return float2(Max(a.x, b.x), Min(a.y, b.y));
}

#define SDF_FUNC() \
	float2 SDF (const float2 uv) \
	{ \
		float2	md = MD_Init(); \
		for (int i = 1; i < points.length(); ++i) { \
			md = Unite( md, SDF_Plane2D( points[0], points[i], uv )); \
		} \
		return md; \
	}
	

#if SHAPE == QUAD
const float2 points[] = {
	float2(  0.0,  0.0 ),
	float2(  2.0,  0.0 ),
	float2( -2.0,  0.0 ),
	float2(  0.0,  2.0 ),
	float2(  0.0, -2.0 )
};
SDF_FUNC();


#elif SHAPE == RHOMBUS
const float2 points[] = {
	float2(  0.0,  0.0 ),
	float2(  2.0,  2.0 ),
	float2( -2.0,  2.0 ),
	float2( -2.0, -2.0 ),
	float2(  2.0, -2.0 )
};
SDF_FUNC();


#elif SHAPE == HEXAGON
const float2 points[] = {
	float2(  0.0,  0.0 ),
	float2(  2.0,  3.0 ),
	float2(  3.0,  0.0 ),
	float2(  2.0, -3.0 ),
	float2( -2.0, -3.0 ),
	float2( -3.0,  0.0 ),
	float2( -2.0,  3.0 )
};
SDF_FUNC();


#elif SHAPE == DIAMOND
float2 SDF (const float2 uv)
{
	float2	md			= MD_Init();
	float2	up_center	= float2(0.0,  0.3);
	float2	down_center	= float2(0.0, -0.2);

	md = Unite( md, SDF_Plane2D( up_center, float2( 2.0, 2.0), uv ));
	md = Unite( md, SDF_Plane2D( up_center, float2(-2.0, 2.0), uv ));
	md = Unite( md, SDF_Plane2D( up_center, float2( 0.0, 2.5), uv ));

	md = Unite( md, SDF_Plane2D( down_center, float2( 1.5, -1.5), uv ));
	md = Unite( md, SDF_Plane2D( down_center, float2(-1.5, -1.5), uv ));

	return md;
}


#elif SHAPE == RANDOM_1
float2 SDF (float2 uv)
{
	//uv.x = uv.x*uv.x;

	const int	seed		= int(iTime * 0.25);
	const int	plane_count = 12;
	
	float2	md		= MD_Init();
	float2	center	= float2(0.0);

	for (int i = 0; i < plane_count; ++i)
	{
		float	angle	= Pi2() * DHash11( (1.87532*i) + seed*3.627 );
		float2	norm	= SinCos( angle ).yx;
		float	radius	= 2.0; // + DHash11( (i*4.8293) + seed*11.5258 ) * 1.5;
		float2	plane	= norm * radius;
		
		md = Unite( md, SDF_Plane2D( center, plane, uv ));
	}
	md = Max( md, Length( uv ) - 3.0);
	return md;
}


#else
const float2 points[] = {
	float2(  0.0,  0.0 ),
	float2(  2.0,  0.0 ),
	float2( -2.0,  0.0 ),
	float2(  0.0,  2.0 ),
	float2(  0.0, -2.0 ),
	float2(  1.0,  1.0 )
};
SDF_FUNC();

#endif


void mainImage (out float4 fragColor, in float2 fragCoord)
{
	float2	uv = ((fragCoord - iResolution.xy*0.5) / min(iResolution.x, iResolution.y)) * 10.0;
	float2	md = SDF( uv );

#if 0
	// isolines
	fragColor = (0.5 + 0.5*Sin(64.0*md)) * (md > 0.0 ? float4(0.0, 0.0, 1.0, 1.0) : float4(1.0, 0.0, 0.0, 1.0));
#else
	float	w = fwidth( md.y );
	fragColor = float4(0.0);
	fragColor.g = SmoothStep( 0.025+w, 0.025-w, md.y );
	fragColor.r = SmoothStep( w, -w, md.x );
#endif
}


#include "SDF.glsl"
#include "RayTracing.glsl"

#define EXPERIMENTAL	0
#define QUAD			1
#define RHOMBUS			2
#define HEXAGON			3
#define DIAMOND			4

#define SHAPE			DIAMOND


float SDF_Plane (const float3 center, const float3 p, const float3 pos)
{
	float3	v = center - p;
	float	d = Length( v );
	return Dot( v / d, pos ) - d;
}

#define SDF_FUNC() \
	float SDF (float3 pos) \
	{ \
		float	md = 0.0; \
		for (int i = 1; i < points.length(); ++i) { \
			md = Max( md, SDF_Plane( points[0], points[i], pos )); \
		} \
		return md; \
	}


#if SHAPE == QUAD
const float3 points[] = {
	float3(  0.0,  0.0,  0.0 ),
	float3(  0.0,  2.0,  0.0 ),
	float3(  0.0, -2.0,  0.0 ),
	float3(  2.0,  0.0,  0.0 ),
	float3( -2.0,  0.0,  0.0 ),
	float3(  0.0,  0.0,  2.0 ),
	float3(  0.0,  0.0, -2.0 )
};
SDF_FUNC();


#elif SHAPE == RHOMBUS
const float3 points[] = {
	float3(  0.0,  0.0,  0.0 ),
	float3( -2.0,  2.0, -2.0 ),
	float3( -2.0,  2.0,  2.0 ),
	float3(  2.0,  2.0,  2.0 ),
	float3(  2.0,  2.0, -2.0 ),
	float3( -2.0, -2.0, -2.0 ),
	float3( -2.0, -2.0,  2.0 ),
	float3(  2.0, -2.0,  2.0 ),
	float3(  2.0, -2.0, -2.0 )
};
SDF_FUNC();


#elif SHAPE == HEXAGON
const float3 points[] = {
	float3(  0.0,  0.0,  0.0 ),
	float3( -2.0,  4.0, -2.0 ),
	float3( -2.0,  4.0,  2.0 ),
	float3(  2.0,  4.0,  2.0 ),
	float3(  2.0,  4.0, -2.0 ),
	float3( -2.0, -4.0, -2.0 ),
	float3( -2.0, -4.0,  2.0 ),
	float3(  2.0, -4.0,  2.0 ),
	float3(  2.0, -4.0, -2.0 ),
	float3(  3.0,  0.0,  0.0 ),
	float3( -3.0,  0.0,  0.0 ),
	float3(  0.0,  0.0,  3.0 ),
	float3(  0.0,  0.0, -3.0 )
};
SDF_FUNC();


#elif SHAPE == DIAMOND
float SDF (const float3 pos)
{
	const float3	up_center	= float3(0.0, 0.45, 0.0);
	const float3	up_center2	= float3(0.0, 0.45, 0.0);
	const float3	down_center	= float3(0.0, 0.0, 0.0);
	const float4	bottom		= float4(0.0, 1.0, 0.0, -0.6);
	const float		radius		= 1.0;

	float	md = -1.0e+10;

	// up
	for (float a = 0.0; a <= 8.0; a += 1.0)
	{
		float2	sc = SinCos( Pi() * 2.0 * (a - 0.25)/8.0 ) * radius;

		md = Max( md, SDF_Plane( up_center,   float3(sc, 1.3).xzy, pos ));
		md = Max( md, SDF_Plane( down_center, float3(sc, -1.3).xzy, pos ));
	}
	
	for (float a = 0.0; a <= 16.0; a += 1.0)
	{
		float2	sc = SinCos( Pi() * 2.0 * a/16.0 ) * radius;

		md = Max( md, SDF_Plane( up_center2,  float3(sc, 1.25).xzy, pos ));
		md = Max( md, SDF_Plane( down_center, float3(sc, -1.4).xzy, pos ));
	}

	// center

	// bottom
	md = Max( md, SDF_Plane( pos, bottom ));

	return md - 1.0;
}

#else
#endif


GEN_SDF_NORMAL_FN( SDFNormal, SDF )


void mainImage (out float4 fragColor, in float2 fragCoord)
{
	Ray	ray = Ray_From( iCameraFrustumLB, iCameraFrustumRB, iCameraFrustumLT, iCameraFrustumRT,
						iCameraPos, 1.0, fragCoord / iResolution.xy );
	
	const int		max_iter	= 256;
	const float		min_dist	= 0.00625f;
	const float		max_dist	= 100.0;
	const float3	obj_pos		= float3(0.0, 0.0, -4.0);

	int i = 0;
	for (; i < max_iter; ++i)
	{
		float	dist = SDF( SDF_Move( ray.pos, obj_pos ));

		ray.t  += dist*0.75;
		ray.pos = ray.t * ray.dir + ray.origin;

		if ( Abs(dist) < min_dist or ray.t > max_dist )
			break;
	}

	if ( ray.t > max_dist )
	{
		fragColor = float4(0.0, 0.0, 0.3, 1.0);
		return;
	}

	float3	norm		= SDFNormal( SDF_Move( ray.pos, obj_pos ));
	float3	light_dir	= -ray.dir; // Normalize(float3( 0.0, 1.0, 0.2 ));

	float	shading = Dot( norm, light_dir );

	//fragColor = float4( ToUNorm(norm), 1.0 );
	fragColor = float4( shading );
}

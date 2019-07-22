
#include "SDF.glsl"
#include "RayTracing.glsl"
#include "CubeMap.glsl"
#include "Geometry.glsl"
#include "Hash.glsl"

#define EXPERIMENTAL	0
#define QUAD			1
#define RHOMBUS			2
#define HEXAGON			3
#define DIAMOND			4
#define RANDOM_1		1000
#define RANDOM_1		1001

#define SHAPE			RANDOM_1


float SDF_Plane (const float3 center, const float3 planePos, const float3 pos)
{
	float3	v = center - planePos;
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
		float2	sc = SinCos( Pi2() * (a - 0.25)/8.0 ) * radius;

		md = Max( md, SDF_Plane( up_center,   float3(sc, 1.3).xzy, pos ));
		md = Max( md, SDF_Plane( down_center, float3(sc, -1.3).xzy, pos ));
	}
	
	for (float a = 0.0; a <= 16.0; a += 1.0)
	{
		float2	sc = SinCos( Pi2() * a/16.0 ) * radius;

		md = Max( md, SDF_Plane( up_center2,  float3(sc, 1.25).xzy, pos ));
		md = Max( md, SDF_Plane( down_center, float3(sc, -1.4).xzy, pos ));
	}

	// center

	// bottom
	md = Max( md, SDF_Plane( pos, bottom.xyz, bottom.w ));

	return md - 1.0;
}


#elif SHAPE == RANDOM_1
float SDF (const float3 pos)
{
	const int	seed		= 11;
	const int	plane_count	= 32;

	float	md		 = -1.0e+10;
	float3	center	= float3(0.0);

	for (int i = 0; i < plane_count; ++i)
	{
		int		face	= (i + seed) % 6;
		float2	scoord	= DHash21( i*51.85 + seed*2.0 );
		float	radius	= 1.0 + DHash11( i*9.361 + seed*3.94521 ) * 0.5;
		float3	norm	= Normalize( CM_TangentialSC_Forward( scoord, face ));
		float3	plane	= radius * norm;

		md = Max( md, SDF_Plane( center, plane, pos ));
	}
	
	md = Max( md, Length( pos ) - 3.0);
	return md;
}


#else
#endif


float SDFScene (const float3 pos)
{
	return SDF( SDF_Move( pos, float3(0.0, 0.0, -4.0) ));
}

GEN_SDF_NORMAL_FN( SDFNormal, SDFScene, )


void mainImage (out float4 fragColor, in float2 fragCoord)
{
	Ray	ray = Ray_From( iCameraFrustumLB, iCameraFrustumRB, iCameraFrustumLT, iCameraFrustumRT,
						iCameraPos, 0.5, fragCoord / iResolution.xy );
#if 1
	const int		max_iter	= 256;
	const float		min_dist	= 0.00625f;
	const float		max_dist	= 100.0;

	int i = 0;
	for (; i < max_iter; ++i)
	{
		float	dist = SDFScene( ray.pos );
		
		Ray_Move( INOUT ray, dist );

		if ( Abs(dist) < min_dist or ray.t > max_dist )
			break;
	}

	if ( ray.t > max_dist )
	{
		fragColor = float4(0.0, 0.0, 0.3, 1.0);
		return;
	}

	float3	norm		= SDFNormal( ray.pos );
	float3	light_dir	= -ray.dir; // Normalize(float3( 0.0, 1.0, 0.2 ));

	float	shading = Dot( norm, light_dir );

	//fragColor = float4( ToUNorm(norm), 1.0 );
	fragColor = float4( shading );

#else
	// debug
	Ray	ray2 = Ray_From( iCameraFrustumLB, iCameraFrustumRB, iCameraFrustumLT, iCameraFrustumRT,
						 iCameraPos, 0.5, float2(0.5) );
	// plane:
	float3	center	= float3(0.0);
	float	dist	= Distance( iCameraPos, center );
	float3	norm	= ray2.dir;
	
	// plane-ray intersection
	float	denom	= Dot( norm, ray.dir );

	if ( denom > 1.0e-5 )
	{
		float3	p = center - ray.origin;
		float	t = Dot( p, norm ) / denom;

		if ( t > -1.0e-5 )
		{
			Ray_SetLength( INOUT ray, t );

			float	dist = SDFScene( ray.pos );
			
			fragColor = float4(dist);
			//fragColor = (dist + 2.0) * (0.5 + 0.5*Sin(16.0*dist)) * float4(1.0);
			return;
		}
	}

	fragColor = float4(0.0, 0.0, 0.3, 1.0);
#endif
}

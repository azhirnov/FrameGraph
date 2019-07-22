
#include "Math.glsl"
#include "SDF.glsl"
#include "RayTracing.glsl"


struct DistAndMat
{
	float	dist;
	float3	pos;
	int		mtrIndex;
};

DistAndMat  DM_Create ()
{
	DistAndMat	dm;
	dm.dist		= 1.0e+10;
	dm.pos		= float3(0.0);
	dm.mtrIndex	= -1;
	return dm;
}

DistAndMat  DM_Create (const float d, const float3 pos, const int mtr)
{
	DistAndMat	dm;
	dm.dist		= d;
	dm.pos		= pos;
	dm.mtrIndex	= mtr;
	return dm;
}

DistAndMat  DM_Min (const DistAndMat x, const DistAndMat y)
{
	return x.dist < y.dist ? x : y;
}

const int	MTR_GROUND		= 0;
const int	MTR_BUILLDING_1	= 1;


//-----------------------------------------------------------------------------
// scene

DistAndMat SDFBuilding1 (const float3 position)
{
	const float3 center = float3(0.0, 0.0, -4.0);
	const float3 pos	= SDF_Move( position, center );
	const float  d		= SDF_OpExtrusion( pos.y, SDF_OpUnite(SDF_Rect( pos.xz + float2(0.75, 0.0), float2(1.0, 0.25) ),
															  SDF_Rect( pos.xz + float2(0.0, 0.75), float2(0.25, 1.0) )), 1.0 );
	return DM_Create( d, center, MTR_BUILLDING_1 );
}

DistAndMat SDFScene (const float3 pos)
{
	return SDFBuilding1( pos );
}

GEN_SDF_NORMAL_FN( SDFNormal, SDFScene, .dist )


//-----------------------------------------------------------------------------
// material

float3 MtrBuilding1 (const Ray ray, const DistAndMat dm, const float3 norm)
{
	float3	pos = ray.pos - dm.pos;
	float2	uv  = float2(0.0, ToUNorm(pos.y) );

	if ( Abs(norm.y) < 0.7 )
		uv.x = ToUNorm( Abs(norm.x) > Abs(norm.z) ? -pos.z : -pos.x );

	uv = Fract( uv * 8.0*float2(1.0, 2.0) );
	
	// panel
	float d2 = 1.0e10;
	d2 = Min( d2, SDF_Rect( uv - float2(0.5, 0.5), float2(0.5, 0.5) ));
	d2 = SmoothStep( 0.01, -0.01, d2 );
    
	// windows
	float d1 = 1.0e+10;
	d1 = Min( d1, SDF_Rect( uv - float2(0.37, 0.5), float2(0.08, 0.25) ));
	d1 = Min( d1, SDF_Rect( uv - float2(0.2, 0.5), float2(0.08, 0.25) ));
	d1 = Min( d1, SDF_Rect( uv - float2(0.78, 0.5), float2(0.12, 0.25) ));
	d1 = SmoothStep( -0.01, 0.01, d1 );
    
	float mtr = d2*0.5 + d1*0.5;


	return float3(mtr);
}


//-----------------------------------------------------------------------------

void mainImage (out float4 fragColor, in float2 fragCoord)
{
	Ray	ray = Ray_From( iCameraFrustumLB, iCameraFrustumRB, iCameraFrustumLT, iCameraFrustumRT,
						iCameraPos, 0.1, fragCoord / iResolution.xy );

	const int	max_iter	= 256;
	const float	min_dist	= 0.00625;
	const float	max_dist	= 100.0;
	DistAndMat	dm			= DM_Create();
	
	for (int i = 0; i < max_iter; ++i)
	{
		dm = SDFScene( ray.pos );

		Ray_Move( INOUT ray, dm.dist );

		if ( Abs(dm.dist) < min_dist or ray.t > max_dist )
			break;
	}

	if ( dm.dist < 0.1 )
	{
		float3	norm = SDFNormal( ray.pos );

		switch ( dm.mtrIndex )
		{
			case MTR_BUILLDING_1 :	fragColor = float4(MtrBuilding1( ray, dm, norm ), 0.0);  break;
		}
	}
	else
	{
		fragColor = float4(0.412, 0.796, 1.0, 0.0);
	}
}

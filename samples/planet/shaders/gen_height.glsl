
#extension GL_GOOGLE_include_directive : require
#extension GL_KHR_memory_scope_semantics : require

layout (local_size_x_id = 0, local_size_y_id = 1, local_size_z = 1) in;

#include "GlobalIndex.glsl"
#include "SDF.glsl"
#include "CubeMap.glsl"
#include "Hash.glsl"


layout(push_constant, std140) uniform PushConst {
	int2	faceDim;
	int		face;
} pc;

// @discard
layout(binding=0) writeonly restrict uniform image2D  un_OutHeight;

// @discard
layout(binding=1) writeonly restrict uniform image2D  un_OutNormal;


float3 CenterOfVoronoiCell (float3 local, float3 global, float time)
{
    float3 coord = local + global;
    return local + (sin( time * DHash33( coord ) * 0.628 ) * 0.5 + 0.5);
}

float VoronoiCircles (in float3 coord, float time)
{
    const int radius = 1;

    float3	ipoint	= Floor( coord );
    float3	fpoint	= Fract( coord );
    
    float3	center	= fpoint;
    float3	icenter	= float3(0.0);
    
    float	md		= 2147483647.0;
    
	// find nearest circle
	for (int z = -radius; z <= radius; ++z)
	for (int y = -radius; y <= radius; ++y)
	for (int x = -radius; x <= radius; ++x)
	{
        float3	cur	= float3(x, y, z);
		float3	c	= CenterOfVoronoiCell( float3(cur), ipoint, time );
		float	d	= Dot( c - fpoint, c - fpoint );

		if ( d < md )
		{
			md = d;
			center = c;
			icenter = cur;
		}
	}

	return md;
}


float4 GetPosition (const int2 coord)
{
	float2	ncoord	= ToSNorm( float2(coord) / float2(pc.faceDim - 1) );
	float3	pos		= PROJECTION( ncoord, pc.face );
	float	height	= -VoronoiCircles( pos * 4.251 + 3.333, 3.5 ) * 0.1;
	//float	height	= SDF_Sphere( Fract( pos * 4.0 ) - 0.5, 0.25 ) * 0.1;
	return float4( pos * (1.0 + height), height );
}


// positions with 1 pixel border for normals calculation
shared float3 s_Positions[ gl_WorkGroupSize.x * gl_WorkGroupSize.y ];

float3  ReadPosition (int2 local)
{
	local += 1;
	return s_Positions[ local.x + local.y * gl_WorkGroupSize.x ];
}


void main ()
{
	const int2		local		= GetLocalCoord().xy - 1;
	const int2		lsize		= GetLocalSize().xy - 2;
	const int2		group		= GetGroupCoord().xy;
	const int2		coord		= local + lsize * group;
	const float4	center		= GetPosition( coord );
	const bool4		is_active	= bool4( greaterThanEqual( local, int2(0) ), lessThan( local, lsize ));

	s_Positions[gl_LocalInvocationIndex ] = center.xyz;

	if ( All( is_active ))
	{
		barrier();
		memoryBarrier( gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquireRelease );

		const int3		offset	= int3(-1, 0, 1);
		const float3	v0		= ReadPosition( local + offset.xx );
		const float3	v1		= ReadPosition( local + offset.yx );
		const float3	v2		= ReadPosition( local + offset.zx );
		const float3	v3		= ReadPosition( local + offset.xy );
		const float3	v4		= center.xyz;
		const float3	v5		= ReadPosition( local + offset.zy );
		const float3	v6		= ReadPosition( local + offset.xz );
		const float3	v7		= ReadPosition( local + offset.yz );
		const float3	v8		= ReadPosition( local + offset.zz );
		float3			normal	= float3(0.0);

		normal += Cross( v1 - v4, v2 - v4 );	// 1-4, 2-4
		normal += Cross( v5 - v4, v8 - v4 );	// 5-4, 8-4
		normal += Cross( v7 - v4, v6 - v4 );	// 7-4, 6-4
		normal += Cross( v3 - v4, v0 - v4 );	// 3-4, 0-4
		normal  = Normalize( normal );

		imageStore( un_OutHeight, coord, float4(center.w) );
		imageStore( un_OutNormal, coord, float4(normal, 0.0) );
	}
}

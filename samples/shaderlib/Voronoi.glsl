// requires extension: GL_EXT_control_flow_attributes

#include "Math.glsl"
#include "Hash.glsl"


// range [0..inf]
float3  Voronoi2D (const float2 coord, const float seed)
{
	float2	ipoint	= Floor( coord );
	float2	fpoint	= Fract( coord );
	float	md		= 1.0e+10;
	float2	center	= float2(0.0);

	[[unroll]] for (int y = -1; y <= 1; ++y)
	[[unroll]] for (int x = -1; x <= 1; ++x)
	{
		float2	cur = float2( x, y );
		float2	off = DHash22( ipoint + cur + seed ) + cur - fpoint;
		float	d   = Dot( off, off );
		
		if ( d < md )
		{
			md = Min( md, d );
			center = ipoint + cur;
		}
	}
	return float3( md, center );
}


// range [0..inf]
float4  Voronoi3D (const float3 coord, const float seed)
{
	float3	ipoint	= Floor( coord );
	float3	fpoint	= Fract( coord );
	float	md		= 1.0e+10;
	float3	center	= float3(0.0);

	[[unroll]] for (int z = -1; z <= 1; ++z)
	[[unroll]] for (int y = -1; y <= 1; ++y)
	[[unroll]] for (int x = -1; x <= 1; ++x)
	{
		float3	cur = float3( x, y, z );
		float3	off = DHash33( ipoint + cur + seed ) + cur - fpoint;
		float	d   = Dot( off, off );
		
		if ( d < md )
		{
			md = Min( md, d );
			center = ipoint + cur;
		}
	}
	return float4( md, center );
}


// range [0..inf]
// based on shader from https://www.shadertoy.com/view/ldl3W8
float  VoronoiCircles (const float2 coord, const float radiusScale, const float seed)
{
	const int radius = 1;
	
	float2	ipoint	= Floor( coord );
	float2	fpoint	= Fract( coord );
	
	float2	icenter	= float2(0.0);
	float	md		= 2147483647.0;
	float	mr		= 2147483647.0;
	
	// find nearest circle
	for (int y = -1; y <= 1; ++y)
	for (int x = -1; x <= 1; ++x)
	{
		float2	cur	= float2(x, y);
		float2	off	= DHash22( cur + ipoint + seed ) + cur - fpoint;
		float	d	= Dot( off, off );

		if ( d < md )
		{
			md = d;
			icenter = cur;
		}
	}
	
	// calc circle radius
	for (int y = -2; y <= 2; ++y)
	for (int x = -2; x <= 2; ++x)
	{
		if ( AllEqual( int2(x,y), int2(0) ))
			continue;
		
		float2	cur = icenter + float2(x, y);
		float2	off	= DHash22( cur + ipoint + seed ) + cur - fpoint;
		float	d	= Dot( off, off );
		
		if ( d < mr )
			mr = d;
	}
	
	md = Sqrt( md );
	mr = Sqrt( mr ) * 0.5 * radiusScale;
	
	if ( md < mr )
		return 1.0 / (Square( md / mr ) * 16.0) - 0.07;

	return 0.0;
}

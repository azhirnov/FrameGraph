
#include "Math.glsl"
#include "Hash.glsl"


// range [0..inf]
float3  Voronoi2D (const float2 pos, const float seed)
{
	float2	n		= Floor( pos );
	float2	f		= Fract( pos );
	float	md		= 1.0e+10;
	float2	center	= float2(0.0);

	[[unroll]] for (int y = -1; y <= 1; ++y)
	[[unroll]] for (int x = -1; x <= 1; ++x)
	{
        float2	g = float2( x, y );
		float2	o = DHash22( n + g + seed );
        float2	r = g + o - f;
		float	d = Dot( r, r );
        
		if ( d < md )
		{
			md = Min( md, d );
			center = n + g;
		}
	}
	return float3( md, center );
}


// range [0..inf]
float4  Voronoi3D (const float3 pos, const float seed)
{
	float3	n		= Floor( pos );
	float3	f		= Fract( pos );
	float	md		= 1.0e+10;
	float3	center	= float3(0.0);

	[[unroll]] for (int z = -1; z <= 1; ++z)
	[[unroll]] for (int y = -1; y <= 1; ++y)
	[[unroll]] for (int x = -1; x <= 1; ++x)
	{
        float3	g = float3( x, y, z );
		float3	o = DHash33( n + g + seed );
        float3	r = g + o - f;
		float	d = Dot( r, r );
        
		if ( d < md )
		{
			md = Min( md, d );
			center = n + g;
		}
	}
	return float4( md, center );
}


// range [0..inf]
float  VoronoiCircles (const float2 coord, const float radiusScale, const float seed)
{
	const int radius = 1;
	
	float2	ipoint	= Floor( coord );
	float2	fpoint	= Fract( coord );
	
	float2	icenter	= float2(0.0);
	float	md		= 2147483647.0;
	float	mr		= 2147483647.0;
	
	// find nearest circle
	for (int y = -radius; y <= radius; ++y)
	for (int x = -radius; x <= radius; ++x)
	{
		float2	cur	= float2(x, y);
		float2	c	= DHash22( cur + ipoint + seed );
		float	d	= Dot( c - fpoint, c - fpoint );

		if ( d < md )
		{
			md = d;
			icenter = cur;
		}
	}
	
	// calc circle radius
	for (int y = -radius; y <= radius; ++y)
	for (int x = -radius; x <= radius; ++x)
	{
		if ( x == 0 and y == 0 )
			continue;
		
		float2	cur = icenter + float2(x, y);
		float2	c	= DHash22( cur + ipoint + seed );
		float	d	= Dot( c - fpoint, c - fpoint );
		
		if ( d < mr )
			mr = d;
	}
	
	md = Sqrt( md );
	mr = Sqrt( mr ) * 0.5 * radiusScale;
	
	if ( md < mr )
		return md / mr;

	return 0.0;
}

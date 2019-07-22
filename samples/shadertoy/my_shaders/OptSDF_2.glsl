
#include "Math.glsl"


void mainImage (out float4 fragColor, in float2 fragCoord)
{
	if ( iFrame > 1 )
		discard;

	int2	coord		= int2(fragCoord.x, iResolution.y - fragCoord.y);
	float2	dist		= float2(RADIUS);
	bool	is_outer	= texelFetch( iChannel0, coord, 0 ).r < 0.25;

	for (int y = -RADIUS; y <= RADIUS; ++y)
	for (int x = -RADIUS; x <= RADIUS; ++x)
	{
		int2	pos			= coord + int2(x,y);
		bool	is_outer2	= texelFetch( iChannel0, pos, 0 ).r < 0.25;
		float	d			= Distance( float2(coord), float2(pos) );
		
		if ( is_outer != is_outer2 )
		{
#		if 1
			float2	norm	= Abs(Normalize( float2(pos) - float2(coord) ));
			bool2	bnorm	= Greater( norm, float2(0.0001) );

			if ( bnorm.x )	dist.x = Min( dist.x, d / norm.x );
			if ( bnorm.y )	dist.y = Min( dist.y, d / norm.y );
#		else
			dist = float2(Min( dist.x, d ));
#		endif
		}
	}

	dist -= 0.5;

	if ( !is_outer )
		dist = -dist;

	fragColor = float4(dist.rgrg);
}

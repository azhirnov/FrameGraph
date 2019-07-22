
#include "Math.glsl"


void mainImage (out float4 fragColor, in float2 fragCoord)
{
	fragColor = float4(0.0);
	fragColor.b = texture( iChannel0, fragCoord/iResolution.xy ).r;
	
	const float2	light_pos	= SinCos( iTime * 0.5 ) * 0.35 + 0.5;
	const float2	img_scale	= 1.0 / float2(textureSize( iChannel1, 0 ).xy);
	const float		scale		= Min( img_scale.x, img_scale.y );
	const int		max_iter	= 64;
	const float2	origin		= fragCoord / iResolution.xy;
	const float2	dir			= Normalize( light_pos - origin );
	const float2	abs_dir		= Abs( dir );
	const float		min_dist	= 0.00625;
	const float		max_dist	= float(RADIUS);
	const float		tmax		= Max( Length( light_pos - origin ) - 0.0001, 0.001 );
	const bool2		bdir		= Greater( abs_dir, vec2(0.0001) );
	float			t			= 0.0;
	float2			pos			= origin;

	int i = 0;
	for (; i < max_iter; ++i)
	{
#	if 1
		float2	dd = texture( iChannel1, pos ).rg;
		dd.x = (bdir.x ? dd.x / abs_dir.x : max_dist);
		dd.y = (bdir.y ? dd.y / abs_dir.y : max_dist);
		float	d = Min( dd.x, dd.y ) * 0.5;
#	else

		float	d = texture( iChannel1, pos ).r;
#	endif
		
		if ( d < min_dist )
			break;

		pos += dir * d * scale;
		t = Length( pos - origin );

		if ( t > tmax )
			break;
	}

#if 1
	if ( t > tmax )
		fragColor.r = 0.1 / tmax;
#else
	fragColor = float4(float(i) / max_iter);
#endif
}

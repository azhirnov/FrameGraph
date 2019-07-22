
#include "Math.glsl"
#include "Hash.glsl"
#include "SDF.glsl"


float SDF (const float2 coord)
{
	float2	ipart	= Floor( coord );
	float2	fpart	= Fract( coord );
	float	md		= 2147483647.0;
	float	seed	= 83.6572;

	for (int y = -2; y <= 2; ++y)
	for (int x = -2; x <= 2; ++x)
	{
		float2	cur		= float2(x,y);
		float4	h		= DHash42( ipart + cur + seed );
		float2	hsize	= Lerp( float2(0.1), float2(1.1), h.xy );
		float2	pos		= h.zw + cur - fpart;
		float	d		= SDF_Box( float3(pos, 0.0), float3(hsize, 1.0) );

		md = Min( md, d );
	}
	return md;
}

void mainImage (out float4 fragColor, in float2 fragCoord)
{
	if ( iFrame > 1 )
		discard;

	float	scale	= 8.0 / min(iResolution.x, iResolution.y);
	float2	uv		= fragCoord * scale;
	float	d		= SDF( uv );

	fragColor = float4( d < 0.0 ? 0.0 : 1.0 );
}

/*
	Perlin noise
*/

#include "Math.glsl"
#include "Hash.glsl"


//	This file based on code from "libNoise" project by Jason Bevins http://libnoise.sourceforge.net/
//	Released under the terms of the GNU Lesser General Public License.

float _perlin_MakeInt32Range (const float n)
{
	const float VALUE = 1073741824.0;
	return n >= VALUE ? 2.0 * Mod( n, VALUE ) - VALUE : ( n <= -VALUE ? 2.0 * Mod( n, VALUE ) + VALUE : n );
}

float2 _perlin_MakeInt32Range (const float2 n)
{
	return float2( _perlin_MakeInt32Range(n.x), _perlin_MakeInt32Range(n.y) );
}

float3 _perlin_MakeInt32Range (const float3 n)
{
	return float3( _perlin_MakeInt32Range(n.x), _perlin_MakeInt32Range(n.y), _perlin_MakeInt32Range(n.z) );
}

#define SCurve5DEF( _type_ ) \
	const _type_ a3 = a * a * a; \
	const _type_ a4 = a3 * a; \
	const _type_ a5 = a4 * a; \
	return 6.0 * a5 - 15.0 * a4 + 10.0 * a3;

float  _perlin_SCurve5 (const float a)  { SCurve5DEF( float ) }
float2 _perlin_SCurve5 (const float2 a) { SCurve5DEF( float2 ) }
float3 _perlin_SCurve5 (const float3 a) { SCurve5DEF( float3 ) }

#undef SCurve5DEF

float _perlin_GradientNoise3DV3 (const float3 f, const int3 i, const int seed)
{
	/*const int X_NOISE_GEN		= 1619;
	const int Y_NOISE_GEN		= 31337;
	const int Z_NOISE_GEN		= 6971;
	const int SEED_NOISE_GEN	= 1013;
	const int SHIFT_NOISE_GEN	= 8;

	int vectorIndex = ( X_NOISE_GEN    * i.x +
						Y_NOISE_GEN    * i.y +
						Z_NOISE_GEN    * i.z +
						SEED_NOISE_GEN * seed) & 0xffffffff;
	vectorIndex ^= (vectorIndex >> SHIFT_NOISE_GEN);
	vectorIndex &= 0xff;
	float3 gradient	= unRandomVectors[ vectorIndex ].xyz;*/

	const float3 coord		= f - float3(i);
	const float3 gradient	= ToSNorm(float3(
									Hash_Uniform( (i.xy + 10.0) * 0.587362, (i.z + 10.0) * 0.239186 ),
									Hash_Uniform( (i.yx + 20.0) * 0.881538, (i.z + 20.0) * 0.170744 ),
									Hash_Uniform( (i.xz + 30.0) * 0.367614, (i.y + 30.0) * 0.922091 )
								  ));
	return Dot( gradient, coord ) * 2.12;
}

float _perlin_GradientNoise3D (const float3 f, const int ix, const int iy, const int iz, const int seed)
{
	return _perlin_GradientNoise3DV3( f, int3(ix, iy, iz), seed );
}

float _perlin_GradientCoherentNoise2D (const float2 p, const int seed)
{
	const int2		p0	= int2(	p.x > 0.0 ? int(p.x) : int(p.x) - 1,
								p.y > 0.0 ? int(p.y) : int(p.y) - 1 );
	const int2		p1	= p0 + 1;
	const float2	s	= _perlin_SCurve5( p - float2(p0) );
	float2			n, i0;
	
	n.x  = _perlin_GradientNoise3D( float3(p, 0.0), p0.x, p0.y, 0, seed );
	n.y  = _perlin_GradientNoise3D( float3(p, 0.0), p1.x, p0.y, 0, seed );
	i0.x = Lerp( n.x, n.y, s.x );

	n.x  = _perlin_GradientNoise3D( float3(p, 0.0), p0.x, p1.y, 0, seed );
	n.y  = _perlin_GradientNoise3D( float3(p, 0.0), p1.x, p1.y, 0, seed );
	i0.y = Lerp( n.x, n.y, s.x );

	return Lerp( i0.x, i0.y, s.y );
}

float _perlin_GradientCoherentNoise3D (const float3 p, const int seed)
{
	const int3		p0	= int3( p.x > 0.0 ? int(p.x) : int(p.x) - 1,
								p.y > 0.0 ? int(p.y) : int(p.y) - 1,
								p.z > 0.0 ? int(p.z) : int(p.z) - 1 );
	const int3		p1	= p0 + 1;
	const float3	s	= _perlin_SCurve5( p - float3(p0) );

	float2	n, i0, i1;
			
	n.x  = _perlin_GradientNoise3D( p, p0.x, p0.y, p0.z, seed );
	n.y  = _perlin_GradientNoise3D( p, p1.x, p0.y, p0.z, seed );
	i0.x = Lerp( n.x, n.y, s.x );

	n.x  = _perlin_GradientNoise3D( p, p0.x, p1.y, p0.z, seed );
	n.y  = _perlin_GradientNoise3D( p, p1.x, p1.y, p0.z, seed );
	i1.x = Lerp( n.x, n.y, s.x );

	i0.y = Lerp( i0.x, i1.x, s.y );


	n.x  = _perlin_GradientNoise3D( p, p0.x, p0.y, p1.z, seed );
	n.y  = _perlin_GradientNoise3D( p, p1.x, p0.y, p1.z, seed );
	i0.x = Lerp( n.x, n.y, s.x );

	n.x  = _perlin_GradientNoise3D( p, p0.x, p1.y, p1.z, seed );
	n.y  = _perlin_GradientNoise3D( p, p1.x, p1.y, p1.z, seed );
	i1.x = Lerp( n.x, n.y, s.x );

	i1.y = Lerp( i0.x, i1.x, s.y );

	return Lerp( i0.y, i1.y, s.z );
}


// range [-1; 1]
float PerlinNoise2D (in float2 coord, const float lacunarity, const float persistence, const int octaveCount, const int seed)
{
	float	value	= 0.0;
	float	pers	= 1.0;
	
	for (int octave = 0; octave < octaveCount; octave++)
	{
		const float2	n		= _perlin_MakeInt32Range( coord );
		const int		oseed	= (seed + octave) & 0xffffffff;
		const float		signal	= _perlin_GradientCoherentNoise2D( n, oseed );

		value += signal * pers;
		coord *= lacunarity;
		pers  *= persistence;
	}
	return value;
}

// range [-1; 1]
float PerlinNoise3D (in float3 coord, const float lacunarity, const float persistence, const int octaveCount, const int seed)
{
	float	value	= 0.0;
	float	pers	= 1.0;
	
	for (int octave = 0; octave < octaveCount; octave++)
	{
		const float3	n		= _perlin_MakeInt32Range( coord );
		const int		oseed	= (seed + octave) & 0xffffffff;
		const float		signal	= _perlin_GradientCoherentNoise3D( n, oseed );

		value += signal * pers;
		coord *= lacunarity;
		pers  *= persistence;
	}
	return value;
}

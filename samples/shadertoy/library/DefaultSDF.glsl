/*
	Default signed distance fields.

	from http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
*/

#include "Quaternion.glsl"


// Shapes
float SDF_Sphere (const float3 position, const float radius);
float SDF_Ellipsoid (const float3 position, const float3 radius);
float SDF_Box (const float3 position, const float3 halfSize);
float SDF_Torus (const float3 position, const float2 firstAndSecondRadius);
float SDF_Cylinder (const float3 position, const float2 radiusHeight);
//float SDF_Cone (const float3 position, const float2 radiusHeight);
float SDF_Plane (const float3 position, const float4 n);
float SDF_HexagonalPrism (const float3 position, const float2 h);
float SDF_TriangularPrism (const float3 position, const float2 h);
float SDF_Capsule (const float3 position, const float3 a, const float3 b, const float r);
float SDF_CappedCone (const float3 position, const float height, const float r1, const float r2);


// Unions
float SDF_OpUnite (const float d1, const float d2);
float SDF_OpSub (const float d1, const float d2);
float SDF_OpIntersect (const float d1, const float d2);


// Transformation
float3 SDF_Move (const float3 position, const float3 delta);
float3 SDF_Rotate (const float3 position, const quat q);
float3 SDF_Transform (const float3 position, const quat q, const float3 delta);



//-----------------------------------------------------------------------------
// Shapes

float SDF_Sphere (const float3 position, const float radius)
{
	return Length( position ) - radius;
}


float SDF_Ellipsoid (const float3 position, const float3 radius)
{
    return ( Length( position / radius ) - 1.0 ) * Min( Min( radius.x, radius.y ), radius.z );
}


float SDF_Box (const float3 position, const float3 halfSize)
{
	const float3	d = Abs( position ) - halfSize;
	return Min( Max( d.x, Max( d.y, d.z ) ), 0.0 ) + Length( Max( d, 0.0 ) );
}


float SDF_Torus (const float3 position, const float2 firstAndSecondRadius)
{
	const float2	q = float2( Length( position.xz ) - firstAndSecondRadius.x, position.y );

	return Length( q ) - firstAndSecondRadius.y;
}


float SDF_Cylinder (const float3 position, const float2 radiusHeight)
{
	const float2	d = Abs( float2( Length( position.xz ), position.y ) ) - radiusHeight;

	return Min( Max( d.x, d.y ), 0.0 ) + Length( Max( d, 0.0 ) );
}


float SDF_Cone (float3 position, float2 radiusHeight)
{
	// c must be normalized
	float q = Length( position.xy );
	return Dot( radiusHeight, float2( q, position.z ));
}


float SDF_Plane (const float3 position, const float4 n)
{
	// n must be normalized
	return Dot( position, n.xyz ) + n.w;
}


float SDF_HexagonalPrism (const float3 position, const float2 h)
{
	const float3	q = Abs( position );

	return Max( q.z - h.y, Max( q.x * 0.866025 + q.y * 0.5, q.y ) - h.x );
}


float SDF_TriangularPrism (const float3 position, const float2 h)
{
    const float3	q = Abs( position );

    return Max( q.z - h.y, Max( q.x * 0.866025 + position.y * 0.5, -position.y ) - h.x * 0.5 );
}


float SDF_Capsule (const float3 position, const float3 a, const float3 b, const float r)
{
	const float3	pa = position - a;
	const float3	ba = position - a;
	const float		h  = Clamp( Dot( pa, ba ) / Dot( ba, ba ), 0.0, 1.0 );

	return Length( pa - ba * h ) - r;
}


float SDF_CappedCone (const float3 position, const float height, const float r1, const float r2)
{
	float2 q  = float2( Length( position.xz ), position.y );
	float2 k1 = float2( r2, height );
	float2 k2 = float2( r2 - r1, 2.0 * height );
	float2 ca = float2( q.x - Min( q.x, (q.y < 0.0) ? r1 : r2 ), Abs( q.y ) - height );
	float2 cb = q - k1 + k2 * Clamp( Dot( k1 - q, k2 ) / Dot( k2, k2 ), 0.0, 1.0 );
	float  s  = (cb.x < 0.0 and ca.y < 0.0) ? -1.0 : 1.0;
	return s * Sqrt( Min( Dot( ca, ca ), Dot( cb, cb )));
}


//-----------------------------------------------------------------------------
// Unions

float SDF_OpUnite (const float d1, const float d2)
{
	return Min( d1, d2 );
}


float SDF_OpSub (const float d1, const float d2)
{
	return Max( d1, -d2 );
}


float SDF_OpIntersect (const float d1, const float d2)
{
	return Max( d1, d2 );
}


//-----------------------------------------------------------------------------
// Transformation

float3 SDF_Move (const float3 position, const float3 delta)
{
	return position - delta;
}


float3 SDF_Rotate (const float3 position, const quat q)
{
	return QMul( QInverse( q ), position );
}


float3 SDF_Transform (const float3 position, const quat q, const float3 delta)
{
	return SDF_Rotate( SDF_Move( position, delta ), q );
}


//-----------------------------------------------------------------------------
// Utils

#define GEN_SDF_NORMAL_FN( _fnName_, _sdf_ ) \
	float3 _fnName_ (const float3 pos) \
	{ \
		const float2	eps  = float2( 0.001, 0.0 ); \
		float3			norm = float3( \
			_sdf_( pos + eps.xyy ) - _sdf_( pos - eps.xyy ), \
			_sdf_( pos + eps.yxy ) - _sdf_( pos - eps.yxy ), \
			_sdf_( pos + eps.yyx ) - _sdf_( pos - eps.yyx ) ); \
		return Normalize( norm ); \
	}

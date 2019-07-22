/*
	Default math types and functions
*/

#define float2		vec2
#define float3		vec3
#define float4		vec4
#define float2x2	mat2x2
#define float3x3	mat3x3
#define float4x4	mat4x4

#define int2		ivec2
#define int3		ivec3
#define int4		ivec4

#define uint2		uvec2
#define uint3		uvec3
#define uint4		uvec4

#define bool2		bvec2
#define bool3		bvec3
#define bool4		bvec4

#define and			&&
#define or			||

#define Any				any
#define All				all
#define Abs				abs
#define ACos			acos
#define ASin			asin
#define ASinH			asinh
#define ACosH			acosh
#define ATan			atan			// result in range [-Pi...+Pi]
#define BitScanReverse	findMSB
#define BitScanForward	findLSB
#define ATanH			atanh
#define Clamp			clamp
#define Ceil			ceil
#define Cos				cos
#define CosH			cosh
#define Cross			cross
#define Distance		distance
#define Dot				dot
#define Exp				exp
#define Exp2			exp2
#define Fract			fract
#define Floor			floor
#define IsNaN			isnan
#define IsInfinity		isinf
#define IsFinite( x )	(! IsInfinity( x ) && ! IsNaN( x ))
#define InvSqrt			inversesqrt
#define IntLog2			BitScanReverse
#define Length			length
#define Lerp			mix
#define Ln				log
#define Log2			log2
#define Log( x, base )	(Ln(x) / Ln(base))
#define Log10( x )		(Ln(x) * 0.4342944819032518)
#define Min				min
#define Max				max
#define Mod				mod
#define MatInverse		inverse
#define MatTranspose	transpose
#define Normalize		normalize
#define Pow				pow
#define Round			round
#define Reflect			reflect
#define Refract			refract
#define Step			step
#define SmoothStep		smoothstep
#define Saturate( x )	clamp( x, 0.0, 1.0 )
#define Sqrt			sqrt
#define Sin				sin
#define SinH			sinh
#define SignOrZero		sign
#define Tan				tan
#define TanH			tanh
#define Trunc			trunc
#define ToUNorm( x )	((x) * 0.5 + 0.5)
#define ToSNorm( x )	((x) * 2.0 - 1.0)


// to mark 'out' and 'inout' argument in function call
// in function argument list use defined by GLSL qualificators: in, out, inout
#define OUT
#define INOUT

#define Less					lessThan
#define Greater					greaterThan
#define LessEqual				lessThanEqual
#define GreaterEqual			greaterThanEqual
#define Not						not

bool  Equals (const float  lhs, const float  rhs)  { return lhs == rhs; }
bool2 Equals (const float2 lhs, const float2 rhs)  { return equal( lhs, rhs ); }
bool3 Equals (const float3 lhs, const float3 rhs)  { return equal( lhs, rhs ); }
bool4 Equals (const float4 lhs, const float4 rhs)  { return equal( lhs, rhs ); }

#define AllLess( a, b )			All( Less( (a), (b) ))
#define AllLessEqual( a, b )	All( LessEqual( (a), (b) ))

#define AllGreater( a, b )		All( Greater( (a), (b) ))
#define AllGreaterEqual( a, b )	All( GreaterEqual( (a), (b) ))

#define AnyLess( a, b )			Any( Less( (a), (b) ))
#define AnyLessEqual( a, b )	Any( LessEqual( (a), (b) ))

#define AnyGreater( a, b )		Any( Greater( (a), (b) ))
#define AnyGreaterEqual( a, b )	Any( GreaterEqual( (a), (b) ))

#define AllEqual( a, b )		All( Equals( (a), (b) ))
#define AnyEqual( a, b )		Any( Equals( (a), (b) ))

#define AllNotEqual( a, b )		All( Not( Equals( (a), (b) )))
#define AnyNotEqual( a, b )		Any( Not( Equals( (a), (b) )))

#define NotAllEqual( a, b )		!All( Equals( (a), (b) ))
#define NotAnyEqual( a, b )		!Any( Equals( (a), (b) ))


float Epsilon ()	{ return 0.00001f; }
float Pi ()			{ return 3.14159265358979323846f; }
float Pi2 ()		{ return Pi() * 2.0; }


float  Sign (const float x)  { return  x < 0.0 ? -1.0 : 1.0; }
int    Sign (const int x)    { return  x < 0 ? -1 : 1; }


float2  SinCos (const float x)  { return float2(sin(x), cos(x)); }


//-----------------------------------------------------------------------------
// square

float  Square (const float x)   { return x * x; }
float2 Square (const float2 x)  { return x * x; }
float3 Square (const float3 x)  { return x * x; }
float4 Square (const float4 x)  { return x * x; }

int  Square (const int x)	  { return x * x; }
int2 Square (const int2 x)    { return x * x; }
int3 Square (const int3 x)    { return x * x; }
int4 Square (const int4 x)    { return x * x; }

uint  Square (const uint x)   { return x * x; }
uint2 Square (const uint2 x)  { return x * x; }
uint3 Square (const uint3 x)  { return x * x; }
uint4 Square (const uint4 x)  { return x * x; }


//-----------------------------------------------------------------------------
// square length and distance

float Length2 (const float2 x)  { return Dot( x, x ); }
float Length2 (const float3 x)  { return Dot( x, x ); }

float Distance2 (const float2 x, const float2 y)  { float2 r = x - y;  return Dot( r, r ); }
float Distance2 (const float3 x, const float3 y)  { float3 r = x - y;  return Dot( r, r ); }


//-----------------------------------------------------------------------------
// clamp / wrap

float ClampOut (const float x, const float minVal, const float maxVal)
{
	float mid = (minVal + maxVal) * 0.5;
	return x < mid ? (x < minVal ? x : minVal) : (x > maxVal ? x : maxVal);
}

int ClampOut (const int x, const int minVal, const int maxVal)
{
	int mid = (minVal+1)/2 + (maxVal+1)/2;
	return x < mid ? (x < minVal ? x : minVal) : (x > maxVal ? x : maxVal);
}


float Wrap (const float x, const float minVal, const float maxVal)
{
	if ( maxVal < minVal ) return minVal;
	float size = maxVal - minVal;
	float res = minVal + Mod( x - minVal, size );
	if ( res < minVal ) return res + size;
	return res;
}

int Wrap (const int x, const int minVal, const int maxVal)
{
	if ( maxVal < minVal ) return minVal;
	int size = maxVal+1 - minVal;
	int res = minVal + ((x - minVal) % size);
	if ( res < minVal ) return res + size;
	return res;
}


//-----------------------------------------------------------------------------
// bit operations

int BitRotateLeft  (const int x, uint shift)
{
	const uint mask = 31;
	shift = shift & mask;
	return (x << shift) | (x >> ( ~(shift-1) & mask ));
}

uint BitRotateLeft  (const uint x, uint shift)
{
	const uint mask = 31;
	shift = shift & mask;
	return (x << shift) | (x >> ( ~(shift-1) & mask ));
}


int BitRotateRight (const int x, uint shift)
{
	const uint mask = 31;
	shift = shift & mask;
	return (x >> shift) | (x << ( ~(shift-1) & mask ));
}

uint BitRotateRight (const uint x, uint shift)
{
	const uint mask = 31;
	shift = shift & mask;
	return (x >> shift) | (x << ( ~(shift-1) & mask ));
}


//-----------------------------------------------------------------------------
// interpolation

float   BaryLerp (const float  a, const float  b, const float  c, const float3 barycentrics)  { return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z; }
float2  BaryLerp (const float2 a, const float2 b, const float2 c, const float3 barycentrics)  { return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z; }
float3  BaryLerp (const float3 a, const float3 b, const float3 c, const float3 barycentrics)  { return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z; }
float4  BaryLerp (const float4 a, const float4 b, const float4 c, const float3 barycentrics)  { return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z; }


float   BiLerp (const float  x1y1, const float  x2y1, const float  x1y2, const float  x2y2, const float2 factor)  { return Lerp( Lerp( x1y1, x2y1, factor.x ), Lerp( x1y2, x2y2, factor.x ), factor.y ); }
float2  BiLerp (const float2 x1y1, const float2 x2y1, const float2 x1y2, const float2 x2y2, const float2 factor)  { return Lerp( Lerp( x1y1, x2y1, factor.x ), Lerp( x1y2, x2y2, factor.x ), factor.y ); }
float3  BiLerp (const float3 x1y1, const float3 x2y1, const float3 x1y2, const float3 x2y2, const float2 factor)  { return Lerp( Lerp( x1y1, x2y1, factor.x ), Lerp( x1y2, x2y2, factor.x ), factor.y ); }
float4  BiLerp (const float4 x1y1, const float4 x2y1, const float4 x1y2, const float4 x2y2, const float2 factor)  { return Lerp( Lerp( x1y1, x2y1, factor.x ), Lerp( x1y2, x2y2, factor.x ), factor.y ); }


// map 'x' in 'src' interval to 'dst' interval
float   Remap (const float2 src, const float2 dst, const float  x)  { return (x - src.x) / (src.y - src.x) * (dst.y - dst.x) + dst.x; }
float2  Remap (const float2 src, const float2 dst, const float2 x)  { return (x - src.x) / (src.y - src.x) * (dst.y - dst.x) + dst.x; }
float3  Remap (const float2 src, const float2 dst, const float3 x)  { return (x - src.x) / (src.y - src.x) * (dst.y - dst.x) + dst.x; }
float4  Remap (const float2 src, const float2 dst, const float4 x)  { return (x - src.x) / (src.y - src.x) * (dst.y - dst.x) + dst.x; }


//-----------------------------------------------------------------------------
// transformations

vec3  RotateVec (const vec4 qleft, const vec3 vright)
{
	vec3  uv  = cross( qleft.xyz, vright );
	vec3  uuv = cross( qleft.xyz, uv );
	return vright + (uv * 2.0f * qleft.w) + (uuv * 2.0f);
}

vec3  Transform (const vec3 pos, const vec3 position, const vec4 orientation, const float scale)
{
	return RotateVec( orientation, pos * scale ) + position;
}

vec4  ToNonLinear (const vec4 color)
{
	return vec4(pow( color.rgb, vec3(2.2f) ), color.a ); 
}

vec2  BaryLerp (const vec2 a, const vec2 b, const vec2 c, const vec3 barycentrics)
{
	return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec3  BaryLerp (const vec3 a, const vec3 b, const vec3 c, const vec3 barycentrics)
{
	return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

//-----------------------------------------------------------------------------
// color

vec3 HSVtoRGB (const vec3 hsv)
{
	// from http://chilliant.blogspot.ru/2014/04/rgbhsv-in-hlsl-5.html
	vec3 col = vec3( abs( hsv.x * 6.0 - 3.0 ) - 1.0,
					 2.0 - abs( hsv.x * 6.0 - 2.0 ),
					 2.0 - abs( hsv.x * 6.0 - 4.0 ) );
	return (( clamp( col, vec3(0.0), vec3(1.0) ) - 1.0 ) * hsv.y + 1.0 ) * hsv.z;
}

//-----------------------------------------------------------------------------
// geometry utils

void GetRayPerpendicular (const vec3 dir, out vec3 outLeft, out vec3 outUp)
{
	const vec3	a	 = abs( dir );
	const vec2	c	 = vec2( 1.0f, 0.0f );
	const vec3	axis = a.x < a.y ? (a.x < a.z ? c.xyy : c.yyx) :
									(a.y < a.z ? c.xyx : c.yyx);
		
	outLeft = normalize(cross( dir, axis ));
	outUp   = normalize(cross( dir, outLeft ));
}

//-----------------------------------------------------------------------------
// lighting

float Attenuation (const vec3 attenuation, float dist)
{
	return clamp( 1.0f / (attenuation.x + attenuation.y * dist + attenuation.z * dist * dist), 0.0f, 1.0f );
}


vec3 DiffuseLighting (const vec3 color, const vec3 norm, const vec3 lightDir)
{
	float NdotL = max( 0.0f, dot( norm, lightDir ));
	return color * NdotL;
}

//-----------------------------------------------------------------------------
// opacity

void ApplyOpticalDepth (inout vec3 inoutColor, const vec3 materialAlbedo, float distance, float opticalDepth)
{
	//inoutColor *= exp( -distance * opticalDepth ) * materialAlbedo;
	//inoutColor = max( inoutColor - exp( -distance * opticalDepth ) * (1.0f - materialAlbedo), vec3(0.0f) );

	inoutColor = mix( inoutColor * materialAlbedo, inoutColor, exp(-distance * opticalDepth) );
}


//-----------------------------------------------------------------------------
// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define SH_VERTEX			(1 << 0)
#define SH_TESS_CONTROL		(1 << 1)
#define SH_TESS_EVALUATION	(1 << 2)
#define SH_FRAGMENT			(1 << 3)

#define USE_QUADS	1

#if SHADER & (SH_TESS_CONTROL | SH_TESS_EVALUATION | SH_FRAGMENT)
layout(binding=0, std140) uniform un_PlanetData
{
	mat4x4		viewProj;
	vec4		position;
	vec2		clipPlanes;

	vec3		lightDirection;	// temp
	float		tessLevel;
} ub;
#endif
//-----------------------------------------------------------------------------



#if SHADER & SH_VERTEX
layout(location=0) in vec3  at_Position;
layout(location=1) in vec3  at_TextureUV;

layout(location=0) out vec3  out_Texcoord;

void main ()
{
	gl_Position  = vec4(at_Position.xyz, 1.0f);
	out_Texcoord = at_TextureUV.xyz;
}
#endif	// SH_VERTEX
//-----------------------------------------------------------------------------



#if USE_QUADS && (SHADER & SH_TESS_CONTROL)
layout(vertices = 4) out;

layout(location=0) in  vec3  in_Texcoord[];
layout(location=0) out vec3  out_Texcoord[];

void main ()
{
#	define I	gl_InvocationID
	
	if ( I == 0 ) {
		gl_TessLevelInner[0] = ub.tessLevel;
		gl_TessLevelInner[1] = ub.tessLevel;
		gl_TessLevelOuter[0] = ub.tessLevel;
		gl_TessLevelOuter[1] = ub.tessLevel;
		gl_TessLevelOuter[2] = ub.tessLevel;
		gl_TessLevelOuter[3] = ub.tessLevel;
	}
	gl_out[I].gl_Position = gl_in[I].gl_Position;
	out_Texcoord[I] = in_Texcoord[I];
}
#endif	// SH_TESS_CONTROL
//-----------------------------------------------------------------------------



#if !USE_QUADS && (SHADER & SH_TESS_CONTROL)
layout(vertices = 3) out;

layout(location=0) in  vec3  in_Texcoord[];
layout(location=0) out vec3  out_Texcoord[];

void main ()
{
#	define I	gl_InvocationID
	
	if ( I == 0 ) {
		gl_TessLevelInner[0] = ub.tessLevel;
		gl_TessLevelOuter[0] = ub.tessLevel;
		gl_TessLevelOuter[1] = ub.tessLevel;
		gl_TessLevelOuter[2] = ub.tessLevel;
	}
	gl_out[I].gl_Position = gl_in[I].gl_Position;
	out_Texcoord[I] = in_Texcoord[I];
}
#endif	// SH_TESS_CONTROL
//-----------------------------------------------------------------------------



#if SHADER & SH_TESS_EVALUATION
layout(binding=1) uniform samplerCube  un_HeightMap;

layout(location=0) in  vec3  in_Texcoord[];
layout(location=0) out vec3  out_Texcoord;

# if USE_QUADS
	layout(quads, equal_spacing, ccw) in;

	#define Interpolate( _arr_, _field_ ) \
		(mix( mix( _arr_[0] _field_, _arr_[1] _field_, gl_TessCoord.x ), \
				mix( _arr_[3] _field_, _arr_[2] _field_, gl_TessCoord.x ), \
				gl_TessCoord.y ))

# else
	layout(triangles, equal_spacing, ccw) in;

	#define Interpolate( _arr_, _field_ ) \
		( gl_TessCoord.x * _arr_[0] _field_ + \
			gl_TessCoord.y * _arr_[1] _field_ + \
			gl_TessCoord.z * _arr_[2] _field_ )

# endif	// USE_QUADS


void main ()
{
	vec3	texc	= Interpolate( in_Texcoord, );
	float	height	= texture( un_HeightMap, texc ).r;
	vec4	pos		= Interpolate( gl_in, .gl_Position );
	vec3	surf_n	= normalize( pos.xyz );
	
	out_Texcoord = texc;
	pos.xyz		 = surf_n * (1.0 + height);
	gl_Position	 = ub.viewProj * (ub.position + pos);
}
#endif	// SH_TESS_EVALUATION
//-----------------------------------------------------------------------------



#if SHADER & SH_FRAGMENT
layout(location=0) out vec4  out_Color;

layout(binding=2) uniform samplerCube  un_NormalMap;
layout(binding=3) uniform samplerCube  un_ColorMap;

layout(location=0) in vec3  in_Texcoord;

void main ()
{
	vec3	norm	 = texture( un_NormalMap, in_Texcoord ).xyz;
	float	lighting = clamp( dot( norm, ub.lightDirection ), 0.2, 1.0 );

	out_Color = vec4( norm * 0.5 + 0.5, 1.0 );
	//out_Color = texture( un_ColorMap, in_Texcoord );
}
#endif	// SH_FRAGMENT
//-----------------------------------------------------------------------------

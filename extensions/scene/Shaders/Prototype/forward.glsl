
// @set 0 PerObject
// @set 1 PerPass

#ifdef VERTEX_SHADER

	vec3 GetWorldPosition ();
	vec2 GetTextureCoordinate0 ();

	// @export
	layout(set=1, binding=0, std140) uniform CameraUB {
		mat4x4		viewProj;
		vec3		position;
		vec3		frustumRayLeftBottom;
		vec3		frustumRayRightBottom;
		vec3		frustumRayLeftTop;
		vec3		frustumRayRightTop;
		vec2		clipPlanes;
	} camera;

// opaque, translucent
# if defined(LAYER_OPAQUE) || defined(LAYER_TRANSLUCENT)
	layout(location=0) out vec3  outWorldPos;
	layout(location=1) out vec2  outTexcoord0;

	void main ()
	{
		outWorldPos		= GetWorldPosition() + camera.position.xyz;
		outTexcoord0	= GetTextureCoordinate0();
		gl_Position		= camera.viewProj * vec4(outWorldPos, 1.0f);
	}
# endif	// LAYER_OPAQUE or LAYER_TRANSLUCENT
	

// shadow map, depth pre-pass
# if defined(LAYER_SHADOWMAP) || defined(LAYER_DEPTHPREPASS)
	layout(location=1) out vec2  outTexcoord0;

	void main ()
	{
		outTexcoord0 = GetTextureCoordinate0();
		gl_Position  = camera.viewProj * vec4(GetWorldPosition() + camera.position.xyz, 1.0f);
	}
# endif	// LAYER_SHADOWMAP or LAYER_DEPTHPREPASS

#endif	// VERTEX_SHADER
//-----------------------------------------------------------------------------


#ifdef FRAGMENT_SHADER
	
	vec4  SampleAlbedoLinear (const vec2 texcoord);

/*
// shadow map, depth pre-pass
# if defined(LAYER_SHADOWMAP) || defined(LAYER_DEPTHPREPASS)

	void main ()
	{
		// TODO: alpha test
	}
# endif	// LAYER_SHADOWMAP or LAYER_DEPTHPREPASS*/


# if defined(LAYER_OPAQUE) || defined(LAYER_TRANSLUCENT)
	layout(location=0) out vec4  out_Color;
# endif

// Opaque, PBR IBL
# if 0 //defined(LAYER_OPAQUE) && defined(ALBEDO_MAP) && defined(NORMAL_MAP) && defined(ROUGHTNESS_MAP) && defined(METALLIC_MAP) && defined(REFLECTION_MAP)
	
	layout(location=0) in vec3  inWorldPos;

	void main ()
	{
		// TODO
	}


// Opaque, color only
# elif defined(LAYER_OPAQUE) && defined(ALBEDO_MAP)
	layout(location=0) in vec3  inWorldPos;
	layout(location=1) in vec2  inTexcoord0;

	void main ()
	{
		out_Color = SampleAlbedoLinear( inTexcoord0 );
	}


// Opaque
# elif defined(LAYER_OPAQUE)
	layout(location=1) in vec2  inTexcoord0;

	void main ()
	{
		out_Color = vec4(inTexcoord0, 0.0f, 1.0f);
	}
# endif	// Opaque

/*
// Translucent
# ifdef LAYER_TRANSLUCENT
	layout(location=1) in vec2  inTexcoord0;

	void main ()
	{
		out_Color = SampleAlbedoLinear( inTexcoord0 );
	}
# endif	// LAYER_TRANSLUCENT*/

#endif	// FRAGMENT_SHADER
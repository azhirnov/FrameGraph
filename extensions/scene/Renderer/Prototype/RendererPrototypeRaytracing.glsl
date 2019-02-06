R"#(
#define PRIMARY_RAY_LOC	0
#define SHADOW_RAY_LOC	1

// @set 0 PerObject
// @set 1 RenderTargets

struct PrimaryRayPayload
{
	vec3	color;
	float	distance;
	vec3	normal;
	int		objectID;
};

struct ShadowRayPayload
{
	float	distance;
};

#ifdef RAYGEN_SHADER
	layout(set=0, binding=0) uniform accelerationStructureNV   un_RtScene;

	layout(set=1, binding=0, rgba16f) writeonly uniform image2D  un_OutColor;

	layout(set=1, binding=1, std140) uniform CameraUB {
		mat4x4		viewProj;
		vec3		position;
		vec3		frustumRayLeftBottom;
		vec3		frustumRayRightBottom;
		vec3		frustumRayLeftTop;
		vec3		frustumRayRightTop;
		vec2		clipPlanes;
	} camera;

	layout(location = PRIMARY_RAY_LOC) rayPayloadNV PrimaryRayPayload  PrimaryRay;
	layout(location = SHADOW_RAY_LOC)  rayPayloadNV ShadowRayPayload   ShadowRay;

	void main ()
	{
		const vec2 uv = vec2(gl_LaunchIDNV.xy) / vec2(gl_LaunchSizeNV.xy - 1);

		const vec3 dir = mix( mix( camera.frustumRayLeftBottom, camera.frustumRayRightBottom, uv.x ),
							  mix( camera.frustumRayLeftTop,    camera.frustumRayRightTop,    uv.x ), uv.y );

		traceNV( /*topLevel*/un_RtScene, /*rayFlags*/gl_RayFlagsNoneNV, /*cullMask*/0xFF,
				 /*sbtRecordOffset*/0, /*sbtRecordStride*/2, /*missIndex*/0,
				 /*origin*/camera.position, /*Tmin*/camera.clipPlanes.x, /*direction*/dir, /*Tmax*/camera.clipPlanes.y,
				 /*payload*/PRIMARY_RAY_LOC );

		imageStore( un_OutColor, ivec2(gl_LaunchIDNV), vec4(PrimaryRay.normal * 0.5f + 0.5f, 1.0f) );
	}
#endif	// RAYGEN_SHADER
//-----------------------------------------------------------------------------


#ifdef PRIMARY_MISS_SHADER
	layout(location = PRIMARY_RAY_LOC) rayPayloadInNV PrimaryRayPayload  PrimaryRay;

	void main ()
	{
		PrimaryRay.color	= vec3(0.412f, 0.796f, 1.0f);
		PrimaryRay.distance	= gl_RayTmaxNV * 2.0f;
		PrimaryRay.normal	= vec3(0.0f);
		PrimaryRay.objectID	= -1;
	}
#endif	// PRIMARY_MISS_SHADER
//-----------------------------------------------------------------------------


#ifdef PRIMARY_CLOSEST_HIT_SHADER
	struct PrimitiveData
	{
		uvec3	face;
		uint	materialID;
	};

	struct VertexAttrib
	{
		vec3	normal;
		vec2	texcoord0;
		vec2	texcoord1;
	};

	layout(set=0, binding=1, std430) readonly buffer PrimitivesSSB {
		PrimitiveData	primitives[];
	};
	layout(set=0, binding=2, std430) readonly buffer VertexAttribsSSB {
		VertexAttrib	attribs[];
	};

	layout(location = PRIMARY_RAY_LOC) rayPayloadInNV PrimaryRayPayload  PrimaryRay;

	hitAttributeNV vec2  HitAttribs;

	vec2  BaryLerp (const vec2 a, const vec2 b, const vec2 c, const vec3 barycentrics)
	{
		return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
	}

	vec3  BaryLerp (const vec3 a, const vec3 b, const vec3 c, const vec3 barycentrics)
	{
		return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
	}

	void main ()
	{
		const vec3			barycentrics = vec3(1.0f - HitAttribs.x - HitAttribs.y, HitAttribs.x, HitAttribs.y);
		const PrimitiveData	prim		 = primitives[gl_PrimitiveID];
		const vec3			norm		 = BaryLerp( attribs[prim.face.x].normal, attribs[prim.face.y].normal, attribs[prim.face.z].normal, barycentrics );

		PrimaryRay.color	= barycentrics;
		PrimaryRay.distance	= gl_HitTNV;
		PrimaryRay.normal	= norm;
		PrimaryRay.objectID	= gl_PrimitiveID;
	}
#endif	// PRIMARY_CLOSEST_HIT_SHADER


#ifdef SHADOW_MISS_SHADER
	layout(location = SHADOW_RAY_LOC) rayPayloadInNV ShadowRayPayload  ShadowRay;

	void main ()
	{
		ShadowRay.distance = gl_RayTmaxNV * 2.0f;
	}
#endif	// SHADOW_MISS_SHADER
//-----------------------------------------------------------------------------


#ifdef SHADOW_CLOSEST_HIT_SHADER
	layout(location = SHADOW_RAY_LOC) rayPayloadInNV ShadowRayPayload  ShadowRay;

	void main ()
	{
		ShadowRay.distance = gl_HitTNV;
	}
#endif	// SHADOW_CLOSEST_HIT_SHADER

)#"s

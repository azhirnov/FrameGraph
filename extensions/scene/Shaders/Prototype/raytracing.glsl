
// @set 1 RenderTargets

struct PrimaryRayPayload
{
	vec3	color;
	float	distance;
};

struct ShadowRayPayload
{
	float	distance;
};

struct IntermMaterial
{
	vec3	albedo;
	float	alpha;
	vec3	specular;
	float	roughtness;
	vec3	normal;
	float	metallic;
};

void TracePrimaryRay (uint rayFlags, const vec3 origin, float Tmin, const vec3 direction, float Tmax);
void TraceShadowRay (uint rayFlags, const vec3 origin, float Tmin, const vec3 direction, float Tmax);
//-----------------------------------------------------------------------------


#ifdef RAYSHADER_Main
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

		TracePrimaryRay( gl_RayFlagsOpaqueNV, camera.position, camera.clipPlanes.x, dir, camera.clipPlanes.y );

		imageStore( un_OutColor, ivec2(gl_LaunchIDNV), vec4(PrimaryRay.color, 1.0f) );
	}
#endif	// RAYSHADER_Main
//-----------------------------------------------------------------------------


#ifdef RAYSHADER_PrimaryMiss
	layout(location = PRIMARY_RAY_LOC) rayPayloadInNV PrimaryRayPayload  PrimaryRay;

	void main ()
	{
		PrimaryRay.color		= vec3(0.412f, 0.796f, 1.0f);
		PrimaryRay.distance		= gl_RayTmaxNV * 2.0f;
	}
#endif	// RAYSHADER_PrimaryMiss
//-----------------------------------------------------------------------------


#ifdef RAYSHADER_ShadowMiss
	layout(location = SHADOW_RAY_LOC) rayPayloadInNV ShadowRayPayload  ShadowRay;

	void main ()
	{
		ShadowRay.distance = gl_RayTmaxNV * 2.0f;
	}
#endif	// RAYSHADER_ShadowMiss
//-----------------------------------------------------------------------------


#ifdef RAYSHADER_PrimaryHit
	layout(location = PRIMARY_RAY_LOC) rayPayloadInNV PrimaryRayPayload  PrimaryRay;
	layout(location = SHADOW_RAY_LOC)  rayPayloadInNV ShadowRayPayload   ShadowRay;

	hitAttributeNV vec2  HitAttribs;
	
	struct Light
	{
		vec3	position;
		float	radius;
		vec3	color;
		vec3	attenuation;
	};

	layout(set=1, binding=2, std140) uniform LightsUB {
		uint	lightCount;
		Light	lights [MAX_LIGHT_COUNT];
	};

	IntermMaterial  LoadMaterial (uint primitiveID, const vec3 barycentrics, float lod);
	
	float Attenuation (const vec3 attenuation, float dist)
	{
		return clamp( 1.0f / (attenuation.x + attenuation.y * dist + attenuation.z * dist * dist), 0.0f, 1.0f );
	}

	vec3 DiffuseLighting (const vec3 color, const vec3 norm, const vec3 lightDir)
	{
		float NdotL = max( 0.0f, dot( norm, lightDir ));
		return color * NdotL;
	}

	float CastShadow (const vec3 lightPos, const vec3 origin)
	{
		const float	tmin		= 0.001f;
		const vec3  light_vec	= lightPos - origin;
		const vec3	light_dir	= normalize( light_vec );
		const float	tmax		= length( light_vec );
		
	#if 0
		// if 'gl_RayFlagsSkipClosestHitShaderNV' setted there is only miss-shader call
		// that set distance to max value, so initialize 'ShadowRay' with zero.
		ShadowRay.distance = 0.0f;

		TraceShadowRay( gl_RayFlagsOpaqueNV | gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsSkipClosestHitShaderNV,
						origin, tmin, light_dir, tmax + tmin );
		return (ShadowRay.distance > tmax ? 1.0f : 0.0f);

	#else
		TraceShadowRay( gl_RayFlagsOpaqueNV, origin, tmin, light_dir, tmax + tmin );
		float shadow = (ShadowRay.distance > tmax ? 1.0f : (ShadowRay.distance / tmax));
		return shadow * shadow;
	#endif
	}

	void main ()
	{
		const vec3				origin		 = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;
		const vec3				barycentrics = vec3(1.0f - HitAttribs.x - HitAttribs.y, HitAttribs.x, HitAttribs.y);
		const IntermMaterial	mtr			 = LoadMaterial( gl_PrimitiveID, barycentrics, 0.0f );
		vec3					light		 = vec3(0.0f);

		// TODO: alpha test

		// shading
		for (uint i = 0; i < lightCount; ++i)
		{
			const vec3	light_pos	= lights[i].position;
			const float	light_dist	= length( light_pos - origin );
			const vec3	light_dir	= normalize( light_pos - origin );
			const float	light_r		= lights[i].radius;
			float		shading		= 0.0f;
			vec3		left_norm, up_norm;
			GetRayPerpendicular( light_dir, left_norm, up_norm );

			vec3	left_up   = normalize( left_norm + up_norm ) * light_r * 0.5f;
			vec3	left_down = normalize( left_norm - up_norm ) * light_r * 0.5f;
					left_norm *= light_r;
					up_norm   *= light_r;

			shading += CastShadow( light_pos, origin ) * 2.0f;

			shading += CastShadow( light_pos + left_up, origin );
			shading += CastShadow( light_pos - left_up, origin );
			shading += CastShadow( light_pos + left_down, origin );
			shading += CastShadow( light_pos - left_down, origin );
		#if 1
			shading *= 2.0f;
			shading += CastShadow( light_pos + left_norm, origin );
			shading += CastShadow( light_pos - left_norm, origin );
			shading += CastShadow( light_pos + up_norm, origin );
			shading += CastShadow( light_pos - up_norm, origin );
			shading /= 16.0f;
		#else
			shading /= 6.0f;
		#endif

			light += DiffuseLighting( lights[i].color, mtr.normal, light_dir ) *
					 Attenuation( lights[i].attenuation, light_dist ) *
					 shading;
		}

		PrimaryRay.color	= mtr.albedo * light;
		PrimaryRay.distance	= gl_HitTNV;
	}
#endif	// RAYSHADER_PrimaryHit
//-----------------------------------------------------------------------------


#ifdef RAYSHADER_ShadowHit
	layout(location = SHADOW_RAY_LOC) rayPayloadInNV ShadowRayPayload  ShadowRay;

	void main ()
	{
		ShadowRay.distance = gl_HitTNV;
	}
#endif	// RAYSHADER_ShadowHit

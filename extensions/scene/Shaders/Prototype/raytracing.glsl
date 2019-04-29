
// @set 1 RenderTargets

struct IntermMaterial
{
	vec3	albedo;
	float	opticalDepth;
	vec3	specular;
	float	roughtness;
	vec3	normal;
	float	metallic;
	float	indexOfRefraction;
	uint	objectID;
};

struct RayPayload
{
	// in
	float	indexOfRefraction;
	uint	depth;

	// out
	vec3	color;
	float	distance;
	uint	objectID;
};

#if SHADER & SH_RAY_CLOSESTHIT
bool IsFrontFace (const vec3 lhs, const vec3 rhs)
{
	return dot( lhs, rhs ) < 0.0001f;
}
#endif

void TracePrimaryRay (uint rayFlags, const vec3 origin, float Tmin, const vec3 direction, float Tmax);
void TraceShadowRay (uint rayFlags, const vec3 origin, float Tmin, const vec3 direction, float Tmax);
IntermMaterial  LoadMaterial (uint instanceID, uint primitiveID, const vec3 barycentrics, float lod);
//-----------------------------------------------------------------------------


#ifdef RAYSHADER_Main
	layout(set=1, binding=0) writeonly uniform image2D  un_OutColor;

	layout(set=1, binding=1, std140) uniform CameraUB {
		mat4x4		viewProj;
		vec3		position;
		vec3		frustumRayLeftBottom;
		vec3		frustumRayRightBottom;
		vec3		frustumRayLeftTop;
		vec3		frustumRayRightTop;
		vec2		clipPlanes;
	} camera;

	layout(location = PRIMARY_RAY_LOC) rayPayloadNV RayPayload  PrimaryRay;
	
	void main ()
	{
		const vec2 uv = vec2(gl_LaunchIDNV.xy) / vec2(gl_LaunchSizeNV.xy - 1);

		vec3  direction = normalize(mix( mix( camera.frustumRayLeftBottom, camera.frustumRayRightBottom, uv.x ),
										 mix( camera.frustumRayLeftTop,    camera.frustumRayRightTop,    uv.x ), uv.y ));

		PrimaryRay.indexOfRefraction	= 1.0f;
		PrimaryRay.depth				= 0;

		TracePrimaryRay( 0, camera.position, camera.clipPlanes.x, direction, camera.clipPlanes.y );

		imageStore( un_OutColor, ivec2(gl_LaunchIDNV), vec4(PrimaryRay.color, 1.0f) );
	}
#endif	// RAYSHADER_Main
//-----------------------------------------------------------------------------


#ifdef RAYSHADER_PrimaryMiss
	layout(location = PRIMARY_RAY_LOC) rayPayloadInNV RayPayload  PrimaryRay;

	void main ()
	{
		PrimaryRay.color	= vec3(0.412f, 0.796f, 1.0f);
		PrimaryRay.distance	= gl_RayTmaxNV * 2.0f;
		PrimaryRay.objectID	= ~0u;
	}
#endif	// RAYSHADER_PrimaryMiss
//-----------------------------------------------------------------------------


#ifdef RAYSHADER_ShadowMiss
	layout(location = PRIMARY_RAY_LOC) rayPayloadInNV RayPayload  PrimaryRay;

	void main ()
	{
		PrimaryRay.color	= vec3(1.0f);
		PrimaryRay.distance	= gl_RayTmaxNV * 2.0f;
		PrimaryRay.objectID	= ~0u;
	}
#endif	// RAYSHADER_ShadowMiss
//-----------------------------------------------------------------------------


#ifdef RAYSHADER_OpaquePrimaryHit
	layout(location = PRIMARY_RAY_LOC) rayPayloadInNV RayPayload  PrimaryRay;

	hitAttributeNV vec2  HitAttribs;
	
	layout (constant_id = 1) const uint maxRecursionDepth = 0;
	
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
	
	vec3  CastShadow (const vec3 lightPos, const vec3 origin)
	{
		const float	tmin		= 0.001f;
		const vec3  light_vec	= lightPos - origin;
		const vec3	light_dir	= normalize( light_vec );
		const float	tmax		= length( light_vec );
		TraceShadowRay( 0, origin, tmin, light_dir, tmax + tmin );
		return PrimaryRay.color;
	}

	vec3  LightingPass (const vec3 origin, const vec3 normal)
	{
		vec3	light = vec3(0.0f);
		for (uint i = 0; i < lightCount; ++i)
		{
			const vec3	light_pos	= lights[i].position;
			const float	light_dist	= length( light_pos - origin );
			const vec3	light_dir	= normalize( light_pos - origin );
			const float	light_r		= lights[i].radius;
			vec3		shading		= vec3(0.0f);
			shading += CastShadow( light_pos, origin );
			light   += DiffuseLighting( lights[i].color, normal, light_dir ) *
					   Attenuation( lights[i].attenuation, light_dist ) *
					   shading;
		}
		return light;
	}
	

	void main ()
	{
		const vec3		barycentrics = vec3(1.0f - HitAttribs.x - HitAttribs.y, HitAttribs.x, HitAttribs.y);		const vec3		origin		 = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;
		IntermMaterial	mtr			 = LoadMaterial( gl_InstanceCustomIndexNV, gl_PrimitiveID, barycentrics, 0.0f );
		vec3			color		 = mtr.albedo;
		
		// trace next ray
		if ( PrimaryRay.depth < maxRecursionDepth )
		{
			PrimaryRay.depth += 1;
			color *= LightingPass( origin, mtr.normal );
		}

		PrimaryRay.color	= color;
		PrimaryRay.distance	= gl_HitTNV;
		PrimaryRay.objectID	= mtr.objectID;
	}
#endif	// RAYSHADER_OpaquePrimaryHit
//-----------------------------------------------------------------------------


#ifdef RAYSHADER_TranslucentPrimaryHit
	layout(location = PRIMARY_RAY_LOC) rayPayloadInNV RayPayload  PrimaryRay;

	hitAttributeNV vec2  HitAttribs;
	
	layout (constant_id = 1) const uint maxRecursionDepth = 0;

	void main ()
	{
		// read object material
		const vec3		barycentrics = vec3(1.0f - HitAttribs.x - HitAttribs.y, HitAttribs.x, HitAttribs.y);
		const float		curr_ior	 = PrimaryRay.indexOfRefraction;
		IntermMaterial	mtr			 = LoadMaterial( gl_InstanceCustomIndexNV, gl_PrimitiveID, barycentrics, 0.0f );
		
		// trace next ray
		if ( PrimaryRay.depth < maxRecursionDepth )
		{			const vec3	origin		= gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;
			const vec2	clip_planes	= vec2(0.001f, 100.0f);
			const bool	front_face	= IsFrontFace( mtr.normal, gl_WorldRayDirectionNV );
			vec3		normal;
			float		eta;

			if ( front_face ) {
				normal	= mtr.normal;
				eta		= curr_ior / mtr.indexOfRefraction;
			} else {
				normal	= -mtr.normal;
				eta		= mtr.indexOfRefraction / curr_ior;
			}

			vec3	direction = refract( gl_WorldRayDirectionNV, normal, eta );			// total internal reflection			if ( dot( direction, vec3(1.0f) ) == 0.0f )			{				direction = reflect( gl_WorldRayDirectionNV, normal );				
				// keep 'PrimaryRay.indexOfRefraction'
				PrimaryRay.depth += 1;
				TracePrimaryRay( 0, origin, clip_planes.x, direction, clip_planes.y );			}			else			// refraction			{
				PrimaryRay.indexOfRefraction = mtr.indexOfRefraction;
				PrimaryRay.depth			 += 1;
				TracePrimaryRay( 0, origin, clip_planes.x, direction, clip_planes.y );
				
				if ( !front_face )
				{
					ApplyOpticalDepth( INOUT PrimaryRay.color, mtr.albedo, gl_HitTNV, mtr.opticalDepth );
				}
				if ( front_face && PrimaryRay.objectID != mtr.objectID )
				{
					ApplyOpticalDepth( INOUT PrimaryRay.color, mtr.albedo, PrimaryRay.distance, mtr.opticalDepth );
				}
			}
		}
		else
			PrimaryRay.color = mtr.albedo;

		PrimaryRay.distance	= gl_HitTNV;
		PrimaryRay.objectID	= mtr.objectID;
	}
#endif	// RAYSHADER_TranslucentPrimaryHit
//-----------------------------------------------------------------------------


#ifdef RAYSHADER_OpaqueShadowHit
	layout(location = PRIMARY_RAY_LOC) rayPayloadInNV RayPayload  PrimaryRay;

	void main ()
	{
		// TODO: alpha test

		PrimaryRay.color	= vec3(0.0f); // vec3(gl_HitTNV / gl_RayTmaxNV);
		PrimaryRay.distance	= gl_HitTNV;
		PrimaryRay.objectID	= ~0u;
	}
#endif	// RAYSHADER_OpaqueShadowHit
//-----------------------------------------------------------------------------


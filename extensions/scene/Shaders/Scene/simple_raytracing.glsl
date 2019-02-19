
// @set 0 PerObject

layout(set=0, binding=0) uniform accelerationStructureNV   un_RtScene;

layout (constant_id = 0) const uint sbtRecordStride = 1;
	
void TracePrimaryRay (uint rayFlags, const vec3 origin, float Tmin, const vec3 direction, float Tmax)
{
	const uint cullMask = 0xFF;
	const uint sbtRecordOffset = 0;
	const uint missIndex = 0;
	traceNV( un_RtScene, rayFlags, cullMask, sbtRecordOffset, sbtRecordStride, missIndex, origin, Tmin, direction, Tmax, PRIMARY_RAY_LOC );
}

void TraceShadowRay (uint rayFlags, const vec3 origin, float Tmin, const vec3 direction, float Tmax)
{
	const uint cullMask = 0xFF;
	const uint sbtRecordOffset = 1;
	const uint missIndex = 1;
	traceNV( un_RtScene, rayFlags, cullMask, sbtRecordOffset, sbtRecordStride, missIndex, origin, Tmin, direction, Tmax, SHADOW_RAY_LOC );
}
//-----------------------------------------------------------------------------


#if defined(RAYSHADER_Main) || defined(RAYSHADER_PrimaryHit)
	struct PrimitiveData
	{
		uvec3	face;
		int		materialID;
	};
	layout(set=0, binding=1, std430) readonly buffer PrimitivesSSB {
		PrimitiveData	primitives[];
	};

	layout(set=0, binding=2, std430) readonly buffer VertexAttribsSSB {
		VertexAttrib	attribs[];
	};
	
	struct ObjectMaterial
	{
		vec3	albedoColor;
		uint	albedoMap;
		vec3	specularColor;
		uint	specularMap;
		uint	normalMap;
		float	roughtness;
		float	metallic;
	};
	layout(set=0, binding=3, std430) readonly buffer MaterialsSSB {
		ObjectMaterial	materials[];
	};

	#ifdef ALBEDO_MAP
		layout(set=0, binding=4) uniform sampler2D  un_AlbedoMaps[];
	#endif
	#ifdef NORMAL_MAP
		layout(set=0, binding=5) uniform sampler2D  un_NormalMaps[];
	#endif
	#ifdef SPECULAR_MAP
		layout(set=0, binding=6) uniform sampler2D  un_SpecularMaps[];
	#endif

	IntermMaterial  LoadMaterial (uint primitiveID, const vec3 barycentrics, float lod)
	{
		const PrimitiveData		prim	= primitives[ primitiveID ];
		const ObjectMaterial	mtr		= materials[ prim.materialID ];
		const VertexAttrib		attr0	= attribs[ prim.face.x ];
		const VertexAttrib		attr1	= attribs[ prim.face.y ];
		const VertexAttrib		attr2	= attribs[ prim.face.z ];
		const vec2				uv0		= BaryLerp( attr0.at_TextureUV, attr1.at_TextureUV, attr2.at_TextureUV, barycentrics );
		IntermMaterial			result;

		result.roughtness	= mtr.roughtness;
		result.metallic		= mtr.metallic;
		result.alpha		= 1.0f;
	
		#ifdef ALBEDO_MAP
			result.albedo	= textureLod( un_AlbedoMaps[nonuniformEXT(mtr.albedoMap)], uv0, lod ).rgb * mtr.albedoColor;
		#else
			result.albedo	= mtr.albedoColor;
		#endif

		#ifdef NORMAL_MAP
			// TODO
		#else
			result.normal	= BaryLerp( attr0.at_Normal, attr1.at_Normal, attr2.at_Normal, barycentrics );
		#endif

		#ifdef SPECULAR_MAP
			result.specular	= textureLod( un_AlbedoMaps[nonuniformEXT(mtr.specularMap)], uv0, lod ).rgb * mtr.specularColor;
		#else
			result.specular	= mtr.specularColor;
		#endif

		return result;
	}

#endif	// RAYSHADER_Main or RAYSHADER_PrimaryHit

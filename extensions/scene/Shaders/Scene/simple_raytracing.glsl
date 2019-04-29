
// @set 0 PerObject

layout(set=0, binding=0) uniform accelerationStructureNV   un_RtScene;

layout (constant_id = 0) const uint sbtRecordStride = 0;


void TracePrimaryRay (uint rayFlags, const vec3 origin, float Tmin, const vec3 direction, float Tmax)
{
	traceNV( un_RtScene, rayFlags, /*cullMask*/0xFF, /*sbtRecordOffset*/0, sbtRecordStride, /*missIndex*/0, origin, Tmin, direction, Tmax, PRIMARY_RAY_LOC );
}

void TraceShadowRay (uint rayFlags, const vec3 origin, float Tmin, const vec3 direction, float Tmax)
{
	traceNV( un_RtScene, rayFlags, /*cullMask*/0xFF, /*sbtRecordOffset*/1, sbtRecordStride, /*missIndex*/1, origin, Tmin, direction, Tmax, PRIMARY_RAY_LOC );
}
//-----------------------------------------------------------------------------


#if SHADER & SH_RAY_CLOSESTHIT

	struct PrimitiveData
	{
		uvec3	face;
		uint	materialAndObjectID;
	};
	layout(set=0, binding=1, std430) readonly buffer PrimitivesSSB {
		PrimitiveData	primitives[];
	} un_PerObjectPrimitives[];

	layout(set=0, binding=2, std430) readonly buffer VertexAttribsSSB {
		VertexAttrib	attribs[];
	} un_PerObjectAttribs[];
	
	struct ObjectMaterial
	{
		vec3	albedoColor;
		uint	albedoMap;
		vec3	specularColor;
		uint	specularMap;
		uint	normalMap;
		float	roughtness;
		float	metallic;
		float	indexOfRefraction;
		float	opticalDepth;
	};
	layout(set=0, binding=3, std430) readonly buffer MaterialsSSB {
		ObjectMaterial	materials[];
	};

	#if TEXTURE_BITS & ALBEDO_MAP
		layout(set=0, binding=4) uniform sampler2D  un_AlbedoMaps[];
	#endif
	#if TEXTURE_BITS & NORMAL_MAP
		layout(set=0, binding=5) uniform sampler2D  un_NormalMaps[];
	#endif
	#if TEXTURE_BITS & SPECULAR_MAP
		layout(set=0, binding=6) uniform sampler2D  un_SpecularMaps[];
	#endif


	IntermMaterial  LoadMaterial (uint instanceID, uint primitiveID, const vec3 barycentrics, float lod)
	{
		const PrimitiveData		prim	= un_PerObjectPrimitives[ nonuniformEXT(instanceID) ].primitives[ primitiveID ];
		const uint				mtr_id	= (prim.materialAndObjectID & 0xFFFF);
		const uint				obj_id	= (prim.materialAndObjectID >> 16);
		const ObjectMaterial	mtr		= materials[ mtr_id ];
		const VertexAttrib		attr0	= un_PerObjectAttribs[ nonuniformEXT(instanceID) ].attribs[ prim.face.x ];
		const VertexAttrib		attr1	= un_PerObjectAttribs[ nonuniformEXT(instanceID) ].attribs[ prim.face.y ];
		const VertexAttrib		attr2	= un_PerObjectAttribs[ nonuniformEXT(instanceID) ].attribs[ prim.face.z ];
		const vec2				uv0		= BaryLerp( attr0.at_TextureUV, attr1.at_TextureUV, attr2.at_TextureUV, barycentrics );
		IntermMaterial			result;

		result.roughtness		 = mtr.roughtness;
		result.metallic			 = mtr.metallic;
		result.indexOfRefraction = mtr.indexOfRefraction;
		result.opticalDepth		 = mtr.opticalDepth;
		result.objectID			 = obj_id;
	
		#if TEXTURE_BITS & ALBEDO_MAP
			result.albedo	= textureLod( un_AlbedoMaps[ nonuniformEXT(mtr.albedoMap) ], uv0, lod ).rgb * mtr.albedoColor;
		#else
			result.albedo	= mtr.albedoColor;
		#endif

		#if TEXTURE_BITS & NORMAL_MAP
			// TODO
		#else
			result.normal	= BaryLerp( attr0.at_Normal, attr1.at_Normal, attr2.at_Normal, barycentrics );
		#endif

		#if TEXTURE_BITS & SPECULAR_MAP
			result.specular	= textureLod( un_AlbedoMaps[nonuniformEXT(mtr.specularMap)], uv0, lod ).rgb * mtr.specularColor;
		#else
			result.specular	= mtr.specularColor;
		#endif

		return result;
	}

#endif	// SH_RAY_CLOSESTHIT

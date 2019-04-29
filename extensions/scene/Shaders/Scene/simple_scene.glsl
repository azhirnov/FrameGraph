
#if SHADER & SH_VERTEX
	struct ObjectTransform
	{
		vec4	orientation;
		vec3	position;
		float	scale;
	};

	layout(set=0, binding=0, std140) uniform PerInstanceUB {
		ObjectTransform		transforms [MAX_INSTANCE_COUNT];
	} perInstance;

	layout(push_constant, std140) uniform VSPushConst {
		layout(offset=0) int	materialID;
		layout(offset=4) int	instanceID;
	};

	vec3 GetWorldPosition ()
	{
		vec3	pos   = perInstance.transforms[instanceID].position;
		float	scale = perInstance.transforms[instanceID].scale;
		vec4	quat  = perInstance.transforms[instanceID].orientation;
		return Transform( at_Position.xyz, pos, quat, scale );
	}

	#ifdef ATTRIB_TextureUV
	vec2 GetTextureCoordinate0 ()  { return at_TextureUV.xy; }
	#else
	vec2 GetTextureCoordinate0 ()  { return vec2(1.0f); }
	#endif
#endif	// SH_VERTEX


#if SHADER & SH_FRAGMENT
	/*layout(set=0, binding=1, std140) uniform MaterialsUB {
		vec4	someData;
	} mtr;*/

# if TEXTURE_BITS & ALBEDO_MAP
	layout(set=0, binding=2) uniform sampler2D  un_AlbedoTex;
	vec4  SampleAlbedoLinear (const vec2 texcoord)    { return texture( un_AlbedoTex, texcoord ); }
	vec4  SampleAlbedoNonlinear (const vec2 texcoord) { return ToNonLinear(texture( un_AlbedoTex, texcoord )); }
# endif

# if TEXTURE_BITS & SPECULAR_MAP
	layout(set=0, binding=3) uniform sampler2D  un_SpecularTex;
	float SampleSpecular (const vec2 texcoord)  { return texture( un_SpecularTex, texcoord ).r; }
# endif

# if TEXTURE_BITS & ROUGHTNESS_MAP
	layout(set=0, binding=4) uniform sampler2D  un_RoughtnessTex;
	float SampleRoughtness (const vec2 texcoord)  { return texture( un_RoughtnessTex, texcoord ).r; }
# endif

# if TEXTURE_BITS & METALLIC_MAP
	layout(set=0, binding=5) uniform sampler2D  un_MetallicTex;
	float SampleMetallic (const vec2 texcoord)  { return texture( un_MetallicTex, texcoord ).r; }
# endif
#endif	// SH_FRAGMENT

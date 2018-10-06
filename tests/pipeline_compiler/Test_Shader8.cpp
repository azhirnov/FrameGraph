// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'
// ray tracing shaders was copied from glslang tests

#include "Utils.h"


extern void Test_Shader8 (VPipelineCompiler* compiler)
{
	RayTracingPipelineDesc	ppln;
	
	ppln.AddShader( EShader::RayGen,
				    EShaderLangFormat::Vulkan_110 | EShaderLangFormat::HighLevel,
				    "main", R"#(
#version 460
#extension GL_NVX_raytracing : enable
layout(binding = 0, set = 0) uniform accelerationStructureNVX accNV;
layout(location = 0) rayPayloadNVX vec4 payload;
layout(shaderRecordNVX) buffer block
{
	float arr[4];
	vec4 pad;
};
void main()
{
    uint lx = gl_LaunchIDNVX.x;
    uint ly = gl_LaunchIDNVX.y;
    uint sx = gl_LaunchSizeNVX.x;
    uint sy = gl_LaunchSizeNVX.y;
    traceNVX(accNV, lx, ly, sx, sy, 0u, vec3(0.0f), 0.5f, vec3(1.0f), 0.75f, 1);
    arr[3] = 1.0f;
    pad = payload;
}
)#" );

	ppln.AddShader( EShader::RayAnyHit,
				    EShaderLangFormat::Vulkan_110 | EShaderLangFormat::HighLevel,
				    "main", R"#(
#version 460
#extension GL_NVX_raytracing : enable
layout(location = 1) rayPayloadInNVX vec4 incomingPayload;
void main()
{
	uvec2 v0 = gl_LaunchIDNVX;
	uvec2 v1 = gl_LaunchSizeNVX;
	int v2 = gl_PrimitiveID;
	int v3 = gl_InstanceID;
	int v4 = gl_InstanceCustomIndexNVX;
	vec3 v5 = gl_WorldRayOriginNVX;
	vec3 v6 = gl_WorldRayDirectionNVX;
	vec3 v7 = gl_ObjectRayOriginNVX;
	vec3 v8 = gl_ObjectRayDirectionNVX;
	float v9 = gl_RayTminNVX;
	float v10 = gl_RayTmaxNVX;
	float v11 = gl_HitTNVX;
	uint v12 = gl_HitKindNVX;
	mat4x3 v13 = gl_ObjectToWorldNVX;
	mat4x3 v14 = gl_WorldToObjectNVX;
	incomingPayload = vec4(0.5f);
	if (v2 == 1)
	    ignoreIntersectionNVX();
	else
	    terminateRayNVX();
}
)#" );

	ppln.AddShader( EShader::RayClosestHit,
				    EShaderLangFormat::Vulkan_110 | EShaderLangFormat::HighLevel,
				    "main", R"#(
#version 460
#extension GL_NVX_raytracing : enable
layout(binding = 0, set = 0) uniform accelerationStructureNVX accNV;
layout(location = 0) rayPayloadNVX vec4 localPayload;
layout(location = 1) rayPayloadInNVX vec4 incomingPayload;
void main()
{
	uvec2 v0 = gl_LaunchIDNVX;
	uvec2 v1 = gl_LaunchSizeNVX;
	int v2 = gl_PrimitiveID;
	int v3 = gl_InstanceID;
	int v4 = gl_InstanceCustomIndexNVX;
	vec3 v5 = gl_WorldRayOriginNVX;
	vec3 v6 = gl_WorldRayDirectionNVX;
	vec3 v7 = gl_ObjectRayOriginNVX;
	vec3 v8 = gl_ObjectRayDirectionNVX;
	float v9 = gl_RayTminNVX;
	float v10 = gl_RayTmaxNVX;
	float v11 = gl_HitTNVX;
	uint v12 = gl_HitKindNVX;
	mat4x3 v13 = gl_ObjectToWorldNVX;
	mat4x3 v14 = gl_WorldToObjectNVX;
	traceNVX(accNV, 0u, 1u, 2u, 3u, 0u, vec3(0.5f), 0.5f, vec3(1.0f), 0.75f, 1);
}
)#" );

	ppln.AddShader( EShader::RayMiss,
				    EShaderLangFormat::Vulkan_110 | EShaderLangFormat::HighLevel,
				    "main", R"#(
#version 460
#extension GL_NVX_raytracing : enable
layout(binding = 0, set = 0) uniform accelerationStructureNVX accNV;
layout(location = 0) rayPayloadNVX vec4 localPayload;
layout(location = 1) rayPayloadInNVX vec4 incomingPayload;
void main()
{
	uvec2 v0 = gl_LaunchIDNVX;
	uvec2 v1 = gl_LaunchSizeNVX;
	vec3 v2 = gl_WorldRayOriginNVX;
	vec3 v3 = gl_WorldRayDirectionNVX;
	vec3 v4 = gl_ObjectRayOriginNVX;
	vec3 v5 = gl_ObjectRayDirectionNVX;
	float v6 = gl_RayTminNVX;
	float v7 = gl_RayTmaxNVX;
	traceNVX(accNV, 0u, 1u, 2u, 3u, 0u, vec3(0.5f), 0.5f, vec3(1.0f), 0.75f, 1);
}
)#" );

	ppln.AddShader( EShader::RayIntersection,
				    EShaderLangFormat::Vulkan_110 | EShaderLangFormat::HighLevel,
				    "main", R"#(
#version 460
#extension GL_NVX_raytracing : enable
hitAttributeNVX vec4 iAttr;
void main()
{
	uvec2 v0 = gl_LaunchIDNVX;
	uvec2 v1 = gl_LaunchSizeNVX;
	int v2 = gl_PrimitiveID;
	int v3 = gl_InstanceID;
	int v4 = gl_InstanceCustomIndexNVX;
	vec3 v5 = gl_WorldRayOriginNVX;
	vec3 v6 = gl_WorldRayDirectionNVX;
	vec3 v7 = gl_ObjectRayOriginNVX;
	vec3 v8 = gl_ObjectRayDirectionNVX;
	float v9 = gl_RayTminNVX;
	float v10 = gl_RayTmaxNVX;
	mat4x3 v11 = gl_ObjectToWorldNVX;
	mat4x3 v12 = gl_WorldToObjectNVX;
	iAttr = vec4(0.5f,0.5f,0.0f,1.0f);
	reportIntersectionNVX(0.5, 1U);
}
)#" );
	/*
	ppln.AddShader( EShader::RayCallable,
				    EShaderLangFormat::Vulkan_110 | EShaderLangFormat::HighLevel,
				    "main", R"#(
)#" );*/


	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::Vulkan_110 | EShaderLangFormat::SPIRV ) );
	
	// TODO

    FG_LOGI( "Test_Shader8 - passed" );
}

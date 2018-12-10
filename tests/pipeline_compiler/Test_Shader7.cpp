// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'
// task & mesh shaders was copied from glslang tests

#include "Utils.h"


extern void Test_Shader7 (VPipelineCompiler* compiler)
{
	MeshPipelineDesc	ppln;

	ppln.AddShader( EShader::MeshTask, EShaderLangFormat::VKSL_110, "main", R"#(
#version 450
#extension GL_NV_mesh_shader : enable

layout(local_size_x = 32) in;

layout(binding=0) writeonly uniform image2D uni_image;
uniform block0 {
	uint uni_value;
};
shared vec4 mem[10];

taskNV out Task {
	vec2 dummy;
	vec2 submesh[3];
} mytask;

void main()
{
	uint iid = gl_LocalInvocationID.x;
	uint gid = gl_WorkGroupID.x;

	for (uint i = 0; i < 10; ++i) {
		mem[i] = vec4(i + uni_value);
	}
	imageStore(uni_image, ivec2(iid), mem[gid]);
	imageStore(uni_image, ivec2(iid), mem[gid+1]);

	memoryBarrierShared();
	barrier();

	mytask.dummy	  = vec2(30.0, 31.0);
	mytask.submesh[0] = vec2(32.0, 33.0);
	mytask.submesh[1] = vec2(34.0, 35.0);
	mytask.submesh[2] = mytask.submesh[gid%2];

	memoryBarrierShared();
	barrier();

	gl_TaskCountNV = 3;
}
)#" );

	ppln.AddShader( EShader::Mesh, EShaderLangFormat::VKSL_110, "main", R"#(
#version 450
#extension GL_NV_mesh_shader : enable

#define MAX_VER  81
#define MAX_PRIM 32

layout(local_size_x_id = 0) in;

layout(max_vertices=MAX_VER) out;
layout(max_primitives=MAX_PRIM) out;
layout(triangles) out;

taskNV in taskBlock {
	float gid1[2];
	vec4 gid2;
} mytask;

buffer bufferBlock {
	float gid3[2];
	vec4 gid4;
} mybuf;

layout(location=0) out outBlock {
	float gid5;
	vec4 gid6;
} myblk[];

void main()
{
	uint iid = gl_LocalInvocationID.x;

	myblk[iid].gid5 = mytask.gid1[1] + mybuf.gid3[1];
	myblk[iid].gid6 = mytask.gid2	+ mybuf.gid4;
}
)#" );

	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_110 ));
	
	TEST( ppln._topology == EPrimitive::TriangleList );

	TEST( ppln._maxIndices == 32*3 );
	TEST( ppln._maxVertices == 81 );

	TEST(All( ppln._defaultTaskGroupSize == uint3{ 32, 1, 1 } ));
	TEST(All( ppln._defaultMeshGroupSize == uint3{ 1, 1, 1 } ));
	TEST(All( ppln._taskSizeSpec == uint3{ ~0u } ));
	TEST(All( ppln._meshSizeSpec == uint3{ 0, ~0u, ~0u } ));

	FG_LOGI( "Test_Shader7 - passed" );
}

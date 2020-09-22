// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"


extern void Test_Reflection5 (VPipelineCompiler* compiler)
{
	ComputePipelineDesc	ppln;

	ppln.AddShader( EShaderLangFormat::GLSL_450, "main", R"#(
#pragma shader_stage(compute)
#extension GL_ARB_separate_shader_objects : enable

layout (local_size_x=1, local_size_y=1, local_size_z=1) in;

layout(binding=0, rgba8) readonly  uniform image2D			un_Image2D_0;
layout(binding=1, r16ui) coherent  uniform uimage2D			un_Image2D_1;
layout(binding=2)        writeonly uniform image2DMS		un_Image2DMS_2;
layout(binding=3)                  uniform sampler3D		un_Image3D_3;
layout(binding=4)                  uniform sampler2DArray	un_Image2DA_4;
layout(binding=5)                  uniform isampler1D		un_Image1D_5;
layout(binding=6)                  uniform sampler2DShadow	un_Image2DS_6;

void main ()
{
}
)#" );


	TEST( compiler->Compile( INOUT ppln, EShaderLangFormat::SPIRV_100 ));

	auto ds = FindDescriptorSet( ppln, DescriptorSetID("0") );
	TEST( ds );

	TEST( TestImageUniform( *ds, UniformID("un_Image2D_0"), EImageSampler(EPixelFormat::RGBA8_UNorm) | EImageSampler::Float2D, EShaderAccess::ReadOnly, 0, EShaderStages::Compute ));
	TEST( TestImageUniform( *ds, UniformID("un_Image2D_1"), EImageSampler(EPixelFormat::R16U) | EImageSampler::Uint2D, EShaderAccess::ReadWrite, 1, EShaderStages::Compute ));
	TEST( TestImageUniform( *ds, UniformID("un_Image2DMS_2"), EImageSampler::Float2DMS, EShaderAccess::WriteOnly, 2, EShaderStages::Compute ));
	
	TEST( TestTextureUniform( *ds, UniformID("un_Image3D_3"), EImageSampler::Float3D, 3, EShaderStages::Compute ));
	TEST( TestTextureUniform( *ds, UniformID("un_Image2DA_4"), EImageSampler::Float2DArray, 4, EShaderStages::Compute ));
	TEST( TestTextureUniform( *ds, UniformID("un_Image1D_5"), EImageSampler::Int1D, 5, EShaderStages::Compute ));
	TEST( TestTextureUniform( *ds, UniformID("un_Image2DS_6"), EImageSampler::Float2D | EImageSampler::_Shadow, 6, EShaderStages::Compute ));
	
	TEST_PASSED();
}

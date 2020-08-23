// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"

extern void Test_Annotation1 (VPipelineCompiler* compiler);
extern void Test_Annotation2 (VPipelineCompiler* compiler);
extern void Test_Annotation3 (VPipelineCompiler* compiler);
extern void Test_Annotation4 (VPipelineCompiler* compiler);

extern void Test_ComputeLocalSize1 (VPipelineCompiler* compiler);

extern void Test_MeshShader1 (VPipelineCompiler* compiler);
extern void Test_MRT1 (VPipelineCompiler* compiler);

extern void Test_Optimization1 (VPipelineCompiler* compiler);

extern void Test_PushConst1 (VPipelineCompiler* compiler);
extern void Test_PushConst2 (VPipelineCompiler* compiler);
extern void Test_PushConst3 (VPipelineCompiler* compiler);

extern void Test_RayTracing1 (VPipelineCompiler* compiler);

extern void Test_Reflection1 (VPipelineCompiler* compiler);
extern void Test_Reflection2 (VPipelineCompiler* compiler);
extern void Test_Reflection3 (VPipelineCompiler* compiler);
extern void Test_Reflection4 (VPipelineCompiler* compiler);

extern void Test_ShaderTrace1 (VPipelineCompiler* compiler);

extern void Test_UniformArrays1 (VPipelineCompiler* compiler);
extern void Test_UniformArrays2 (VPipelineCompiler* compiler);

extern void Test_VersionSelector1 (VPipelineCompiler* compiler);


int main ()
{
	{
		VPipelineCompiler	compiler;
		compiler.SetCompilationFlags( EShaderCompilationFlags::Unknown );
	
		Test_Annotation1( &compiler );
		Test_Annotation2( &compiler );
		Test_Annotation3( &compiler );
		Test_Annotation4( &compiler );

		Test_ComputeLocalSize1( &compiler );

		Test_MeshShader1( &compiler );
		Test_MRT1( &compiler );
		
		Test_Optimization1( &compiler );
		
		Test_PushConst1( &compiler );
		Test_PushConst2( &compiler );
		Test_PushConst3( &compiler );
		
		Test_RayTracing1( &compiler );
		
		Test_Reflection1( &compiler );
		Test_Reflection2( &compiler );
		Test_Reflection3( &compiler );
		Test_Reflection4( &compiler );
		
		Test_ShaderTrace1( &compiler );
		
		Test_UniformArrays1( &compiler );
		Test_UniformArrays2( &compiler );
		
		Test_VersionSelector1( &compiler );
	}

	CHECK_FATAL( FG_DUMP_MEMLEAKS() );

	FG_LOGI( "Tests.PipelineCompiler finished" );
	return 0;
}

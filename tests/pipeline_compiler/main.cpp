// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"

extern void Test_Shader1 (VPipelineCompiler* compiler);
extern void Test_Shader2 (VPipelineCompiler* compiler);
extern void Test_Shader3 (VPipelineCompiler* compiler);
extern void Test_Shader4 (VPipelineCompiler* compiler);
extern void Test_Shader5 (VPipelineCompiler* compiler);
extern void Test_Shader6 (VPipelineCompiler* compiler);
extern void Test_Shader7 (VPipelineCompiler* compiler);
extern void Test_Shader8 (VPipelineCompiler* compiler);
extern void Test_Shader9 (VPipelineCompiler* compiler);
extern void Test_Shader10 (VPipelineCompiler* compiler);
extern void Test_Shader11 (VPipelineCompiler* compiler);
extern void Test_Shader12 (VPipelineCompiler* compiler);
extern void Test_Shader13 (VPipelineCompiler* compiler);
extern void Test_Shader14 (VPipelineCompiler* compiler);
extern void Test_Shader15 (VPipelineCompiler* compiler);
extern void Test_Shader16 (VPipelineCompiler* compiler);
extern void Test_Shader17 (VPipelineCompiler* compiler);


int main ()
{
	{
		VPipelineCompiler	compiler;
		compiler.SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations );
	
		Test_Shader1( &compiler );
		Test_Shader2( &compiler );
		Test_Shader3( &compiler );
		Test_Shader4( &compiler );
		Test_Shader5( &compiler );
		Test_Shader6( &compiler );
		Test_Shader7( &compiler );
		Test_Shader8( &compiler );
		Test_Shader9( &compiler );
		Test_Shader10( &compiler );
		Test_Shader11( &compiler );
		Test_Shader12( &compiler );
		Test_Shader13( &compiler );
		Test_Shader14( &compiler );
		Test_Shader15( &compiler );
		Test_Shader16( &compiler );
		Test_Shader17( &compiler );
	}

	CHECK_FATAL( FG_DUMP_MEMLEAKS() );

	FG_LOGI( "Tests.PipelineCompiler finished" );
	return 0;
}

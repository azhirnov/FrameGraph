// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"
#include <iostream>

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
extern void Test_Serializer1 (VPipelineCompiler* compiler);
extern void Test_Serializer2 (VPipelineCompiler* compiler);


int main ()
{
	VPipelineCompiler	compiler;
	compiler.SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations /*|
								  EShaderCompilationFlags::UseCurrentDeviceLimits*/ );
	
	Test_Serializer1( &compiler );
	Test_Serializer2( &compiler );
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
	
	FG_LOGI( "Tests.PipelineCompiler finished" );
	
	std::cin.ignore();
	return 0;
}

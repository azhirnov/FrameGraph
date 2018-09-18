// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Utils.h"
#include <iostream>

extern void Test_Shader1 (VPipelineCompiler* compiler);
extern void Test_Shader2 (VPipelineCompiler* compiler);
extern void Test_Shader3 (VPipelineCompiler* compiler);
extern void Test_Shader4 (VPipelineCompiler* compiler);
extern void Test_Shader5 (VPipelineCompiler* compiler);
extern void Test_Shader6 (VPipelineCompiler* compiler);

extern void Test_Serializer1 (VPipelineCompiler* compiler);


int main ()
{
	VPipelineCompiler	compiler;
    compiler.SetCompilationFlags( EShaderCompilationFlags::AutoMapLocations | EShaderCompilationFlags::GenerateDebugInfo );

	Test_Serializer1( &compiler );
	Test_Shader1( &compiler );
	Test_Shader2( &compiler );
	Test_Shader3( &compiler );
	Test_Shader4( &compiler );
	Test_Shader5( &compiler );
	Test_Shader6( &compiler );

    FG_LOGI( "Tests.PipelineCompiler finished" );
	
    DEBUG_ONLY( std::cin.ignore() );
	return 0;
}

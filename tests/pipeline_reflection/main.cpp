// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "tests/pipeline_compiler/Utils.h"
#include "pipeline_reflection/VPipelineReflection.h"

extern void Test1 (VPipelineCompiler* compiler);


int main ()
{
	{
		VPipelineCompiler	compiler;
		compiler.SetCompilationFlags( EShaderCompilationFlags::Optimize );
	
		Test1( &compiler );
	}
	
	CHECK_FATAL( FG_DUMP_MEMLEAKS() );

	FG_LOGI( "Tests.PipelineReflection finished" );
	return 0;
}

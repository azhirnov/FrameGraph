// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include <android_native_app_glue.h>
#include "stl/Common.h"

#ifdef ENABLE_STL_TESTS
extern void Tests_STL_main ();
#endif
#ifdef ENABLE_PIPELINE_COMPILER_TESTS
extern int Tests_PipelineCompiler_main ();
#endif
#ifdef ENABLE_FRAMEGRAPH_TESTS
extern int Tests_FrameGraph_main (void* nativeHandle);
#endif


void android_main (struct android_app* state)
{
	#ifdef ENABLE_STL_TESTS
	Tests_STL_main();
	#endif
	#ifdef ENABLE_PIPELINE_COMPILER_TESTS
	Tests_PipelineCompiler_main();
	#endif
	#ifdef ENABLE_FRAMEGRAPH_TESTS
	Tests_FrameGraph_main( state );
	#endif
	
	FG_LOGI( "All tests are finished" );
	std::abort();
}

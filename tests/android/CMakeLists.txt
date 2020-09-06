if (DEFINED ANDROID)
    add_library( native_app_glue STATIC "${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c" )
    target_include_directories( native_app_glue PUBLIC "${ANDROID_NDK}/sources/android/native_app_glue" )
    target_link_libraries( native_app_glue PUBLIC android log )
	
	file( GLOB_RECURSE SOURCES "*.*" )
	add_library( "FGTestLauncher" SHARED ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	set_property( TARGET "FGTestLauncher" PROPERTY FOLDER "Tests" )

	if (TARGET "Tests.STL")
		target_compile_definitions( "FGTestLauncher" PUBLIC "ENABLE_STL_TESTS" )
		target_link_libraries( "FGTestLauncher" PUBLIC "Tests.STL" )
	endif ()
	
	if (TARGET "Tests.PipelineCompiler")
		target_compile_definitions( "FGTestLauncher" PUBLIC "ENABLE_PIPELINE_COMPILER_TESTS" )
		target_link_libraries( "FGTestLauncher" PUBLIC "Tests.PipelineCompiler" )
	endif ()
	
	if (TARGET "Tests.FrameGraph")
		target_compile_definitions( "FGTestLauncher" PUBLIC "ENABLE_FRAMEGRAPH_TESTS" )
		target_link_libraries( "FGTestLauncher" PUBLIC "Tests.FrameGraph" )
	endif ()
	
	
	target_link_libraries( "FGTestLauncher" PUBLIC native_app_glue )
	set_target_properties( "FGTestLauncher" PROPERTIES LINK_FLAGS "-u ANativeActivity_onCreate" )
endif ()

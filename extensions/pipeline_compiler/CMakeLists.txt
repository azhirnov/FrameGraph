if (TARGET "GLSLang-lib")
	file( GLOB_RECURSE SOURCES "*.*" )
	add_library( "PipelineCompiler" STATIC ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	target_include_directories( "PipelineCompiler" PUBLIC "." )
	set_property( TARGET "PipelineCompiler" PROPERTY FOLDER "Extensions" )
	
	target_link_libraries( "PipelineCompiler" PUBLIC "VulkanLoader" )
	target_link_libraries( "PipelineCompiler" PUBLIC "FrameGraph" )
	target_link_libraries( "PipelineCompiler" PUBLIC "GLSLang-lib" )

	if (TARGET "GLSL-Trace-lib")
		target_link_libraries( "PipelineCompiler" PUBLIC "GLSL-Trace-lib" )
	endif ()

	install( TARGETS "PipelineCompiler" ARCHIVE DESTINATION "libs/$<CONFIG>" )
	install( FILES "EShaderCompilationFlags.h" "VPipelineCompiler.h" DESTINATION "include/PipelineCompiler" )
endif ()

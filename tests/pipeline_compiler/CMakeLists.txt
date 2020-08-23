if (${FG_ENABLE_GLSLANG})
	file( GLOB_RECURSE SOURCES "*.*" )
	add_executable( "Tests.PipelineCompiler" ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	set_property( TARGET "Tests.PipelineCompiler" PROPERTY FOLDER "Tests" )
	target_link_libraries( "Tests.PipelineCompiler" "PipelineCompiler" )
	
	add_test( NAME "Tests.PipelineCompiler" COMMAND "Tests.PipelineCompiler" )
endif ()
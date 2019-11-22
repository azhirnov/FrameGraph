cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

if (${FG_ENABLE_GLM})
	file( GLOB_RECURSE SOURCES "*.*" )
	add_library( "Scene" STATIC ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	target_include_directories( "Scene" PUBLIC ".." )
	set_property( TARGET "Scene" PROPERTY FOLDER "Extensions" )

	target_link_libraries( "Scene" "FrameGraph" )
	target_link_libraries( "Scene" "Framework" )
	target_link_libraries( "Scene" "GLM-lib" )

	if (${FG_ENABLE_DEVIL})
		target_link_libraries( "Scene" "DevIL-lib" )
	endif()
	if (${FG_ENABLE_ASSIMP})
		target_link_libraries( "Scene" "Assimp-lib" )
	endif()
	if (${FG_ENABLE_GLSLANG})
		target_link_libraries( "Scene" "PipelineCompiler" )
	endif()

	target_compile_definitions( "Scene" PUBLIC "FG_SHADER_PATH=R\"(${CMAKE_CURRENT_SOURCE_DIR}/Shaders/)\"" )

	install( TARGETS "Scene" ARCHIVE DESTINATION "lib" )
endif ()

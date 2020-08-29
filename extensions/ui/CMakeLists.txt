if (TARGET "imgui-lib")
	file( GLOB_RECURSE SOURCES "*.*" )
	add_library( "UI" STATIC ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	target_include_directories( "UI" PUBLIC ".." )
	set_property( TARGET "UI" PROPERTY FOLDER "Extensions" )

	target_link_libraries( "UI" "FrameGraph" )
	target_link_libraries( "UI" "PipelineCompiler" )
	target_link_libraries( "UI" "Framework" )
	target_link_libraries( "UI" "imgui-lib" )

	if (TARGET "GLM-lib")
		target_link_libraries( "UI" "GLM-lib" )
	endif ()

	install( TARGETS "UI" ARCHIVE DESTINATION "lib" )
endif ()

if (TARGET "imgui-lib")
	file( GLOB_RECURSE SOURCES "*.*" )
	add_library( "UI" STATIC ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	target_include_directories( "UI" PUBLIC ".." )
	set_property( TARGET "UI" PROPERTY FOLDER "Extensions" )

	target_link_libraries( "UI" PUBLIC "FrameGraph" )
	target_link_libraries( "UI" PUBLIC "PipelineCompiler" )
	target_link_libraries( "UI" PUBLIC "Framework" )
	target_link_libraries( "UI" PUBLIC "imgui-lib" )

	if (TARGET "GLM-lib")
		target_link_libraries( "UI" PUBLIC "GLM-lib" )
	endif ()

	install( TARGETS "UI" ARCHIVE DESTINATION "lib" )
endif ()

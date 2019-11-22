cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

if (${FG_ENABLE_GRAPHVIZ})
	file( GLOB_RECURSE SOURCES "*.*" )
	add_library( "GraphViz" STATIC ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	target_include_directories( "GraphViz" PUBLIC ".." )
	set_property( TARGET "GraphViz" PROPERTY FOLDER "Extensions" )
	
	target_link_libraries( "GraphViz" "GraphViz-lib" )
	target_link_libraries( "GraphViz" "STL" )

	install( TARGETS "GraphViz" ARCHIVE DESTINATION "lib" )
endif ()

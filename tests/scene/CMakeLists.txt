if (TARGET "Scene")
	file( GLOB_RECURSE SOURCES "*.*" )
	add_executable( "Tests.Scene" ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	set_property( TARGET "Tests.Scene" PROPERTY FOLDER "Tests" )
	target_link_libraries( "Tests.Scene" "Scene" )
	
	add_test( NAME "Tests.Scene" COMMAND "Tests.Scene" )
endif ()

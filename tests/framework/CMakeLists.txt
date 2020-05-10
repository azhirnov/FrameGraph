file( GLOB_RECURSE SOURCES "*.*" )
add_executable( "Tests.Framework" ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.Framework" PROPERTY FOLDER "Tests" )
target_link_libraries( "Tests.Framework" "Framework" )

if (NOT FG_CI_BUILD)
	add_test( NAME "Tests.Framework" COMMAND "Tests.Framework" )
endif ()

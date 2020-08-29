file( GLOB_RECURSE SOURCES "*.*" )

if (DEFINED ANDROID)
	add_library( "Tests.STL" STATIC ${SOURCES} )
else ()
	add_executable( "Tests.STL" ${SOURCES} )
endif ()

source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.STL" PROPERTY FOLDER "Tests" )
target_link_libraries( "Tests.STL" "STL" )

if (NOT DEFINED ANDROID)
	add_test( NAME "Tests.STL" COMMAND "Tests.STL" )
endif ()

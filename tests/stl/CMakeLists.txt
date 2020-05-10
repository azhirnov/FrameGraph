file( GLOB_RECURSE SOURCES "*.*" )
add_executable( "Tests.STL" ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "Tests.STL" PROPERTY FOLDER "Tests" )
target_link_libraries( "Tests.STL" "STL" )

add_test( NAME "Tests.STL" COMMAND "Tests.STL" )

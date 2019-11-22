cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "STL" STATIC ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
target_link_libraries( "STL" "ProjectTemplate" )
target_include_directories( "STL" PUBLIC ".." )
set_property( TARGET "STL" PROPERTY FOLDER "" )

if (UNIX)
	target_link_libraries( "STL" "dl;pthread" )
	target_compile_definitions( "STL" PUBLIC _LARGEFILE_SOURCE )
	target_compile_definitions( "STL" PUBLIC _LARGE_FILES )
	target_compile_definitions( "STL" PUBLIC _FILE_OFFSET_BITS=64 )
endif()

install( TARGETS "STL" ARCHIVE DESTINATION "lib" )

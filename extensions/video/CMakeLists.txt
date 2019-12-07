cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "Video" STATIC ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
target_include_directories( "Video" PUBLIC ".." )
set_property( TARGET "Video" PROPERTY FOLDER "Extensions" )
target_link_libraries( "Video" PUBLIC "STL" )

if (${FG_ENABLE_FFMPEG})
	target_link_libraries( "Video" PUBLIC "ffmpeg-lib" )
endif ()

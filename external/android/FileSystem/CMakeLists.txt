# attaches filesystem (MIT license)

cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

set( GFS_DST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src" )

add_library( "FileSystem-lib" INTERFACE )
target_include_directories( "FileSystem-lib" INTERFACE "${GFS_DST_PATH}/include" )
target_compile_definitions( "FileSystem-lib" INTERFACE "FG_ENABLE_FILESYSTEM" )

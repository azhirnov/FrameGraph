cmake_minimum_required (VERSION 3.6.0)
include(FetchContent)

set( FG_EXTERNALS_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Path to external projects sources" )
message( STATUS "FG_EXTERNALS_PATH: ${FG_EXTERNALS_PATH}" )

set( FG_SKIP_DOWNLOAD_EXTERNALS OFF CACHE BOOL "" )
set( FG_DOWNLOAD_PATH "${FG_EXTERNALS_PATH}" CACHE PATH "Path for downloadable externals" )


set( FG_GLOBAL_DEFINITIONS "" )

include( "${CMAKE_SOURCE_DIR}/cmake/download_vk.cmake" )

if (${FG_SKIP_DOWNLOAD_EXTERNALS} OR CMAKE_VERSION VERSION_LESS 3.11.0)
	include( "${CMAKE_SOURCE_DIR}/cmake/attach.cmake" )
else ()
	include( "${CMAKE_SOURCE_DIR}/cmake/download.cmake" )
endif ()

set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" CACHE INTERNAL "" FORCE )

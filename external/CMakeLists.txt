cmake_minimum_required (VERSION 3.6.0)

set( FG_GLOBAL_DEFINITIONS "" )
set( FG_EXTERNALS_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Path to external projects sources" )
message( STATUS "FG_EXTERNALS_PATH: ${FG_EXTERNALS_PATH}" )

if (NOT CMAKE_VERSION VERSION_LESS 3.11.0)
	include(FetchContent)
	#set( FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "don't download externals" )
	set( FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "don't update externals" )
endif ()


include( "${CMAKE_SOURCE_DIR}/cmake/download_vk.cmake" )
include( "${CMAKE_SOURCE_DIR}/cmake/download_glslang.cmake" )
include( "${CMAKE_SOURCE_DIR}/cmake/download_vma.cmake" )
include( "${CMAKE_SOURCE_DIR}/cmake/download_sdl2.cmake" )
include( "${CMAKE_SOURCE_DIR}/cmake/download_glfw.cmake" )
include( "${CMAKE_SOURCE_DIR}/cmake/download_sfml.cmake" )
include( "${CMAKE_SOURCE_DIR}/cmake/download_lodepng.cmake" )


set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" CACHE INTERNAL "" FORCE )

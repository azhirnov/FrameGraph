# auto generated file
cmake_minimum_required (VERSION 3.6.0)

message( STATUS "==========================================================================\n" )
message( STATUS "project 'FrameGraph' generation started" )

project( "FrameGraph" LANGUAGES C CXX )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )
message( STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER_VERSION})" )
message( STATUS "Compiler name: ${DETECTED_COMPILER}" )
message( STATUS "target system: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_VERSION})" )
message( STATUS "host system: ${CMAKE_HOST_SYSTEM_NAME} (${CMAKE_HOST_SYSTEM_VERSION})" )

include( "cmake/compilers.cmake" )

set( FG_ENABLE_GLSLANG OFF CACHE BOOL "use glslang (optional, required for glsl compilation)" )
set( FG_ENABLE_GLFW ON CACHE BOOL "use glfw (optional, required for tests)" )
set( FG_ENABLE_SDL2 OFF CACHE BOOL "use SDL2 (optional, required for tests)" )
set( FG_ENABLE_VMA OFF CACHE BOOL "use Vulkan Memory Allocator (required)" )
set( FG_ENABLE_LODEPNG OFF CACHE BOOL "use lodepng (optional, may be used in tests)" )
set( FG_ENABLE_SAMPLES OFF CACHE BOOL "" )
set( FG_ENABLE_TESTS ON CACHE BOOL "" )
set( CMAKE_DEBUG_POSTFIX "" )
set( CMAKE_RELEASE_POSTFIX "" )
set( CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "installation directory" )

set( MAIN_BINARY_DIR "${CMAKE_BINARY_DIR}/bin" CACHE INTERNAL "" FORCE )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
add_subdirectory( "external" "external" )

add_subdirectory( "stl" "stl" )

#add_subdirectory( "framegraph" "framegraph" )

add_subdirectory( "extensions" "extensions" )


message( STATUS "project 'FrameGraph' generation ended" )
message( STATUS "\n==========================================================================" )


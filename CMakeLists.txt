# auto generated file
cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

message( STATUS "==========================================================================\n" )
message( STATUS "project 'FrameGraph' generation started" )

project( "FrameGraph" LANGUAGES C CXX )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )
message( STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER_VERSION})" )
message( STATUS "target system: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_VERSION})" )
message( STATUS "host system: ${CMAKE_HOST_SYSTEM_NAME} (${CMAKE_HOST_SYSTEM_VERSION})" )

include( "cmake/ci_build.cmake" )

set( FG_ENABLE_STDALLOC OFF CACHE BOOL "custom std allocators (optional)" )
set( FG_ENABLE_GLSLANG ON CACHE BOOL "use glslang (optional, required for glsl compilation)" )
set( FG_ENABLE_VMA ON CACHE BOOL "use Vulkan Memory Allocator (required)" )
set( FG_ENABLE_OPENVR ON CACHE BOOL "use OpenVR (optional)" )
set( FG_ENABLE_SIMPLE_COMPILER_OPTIONS OFF CACHE BOOL "use simplified compiler settings if you have problem with it" )
set( FG_EXTERNALS_USE_STABLE_VERSIONS ON CACHE BOOL "use last stable version instead of master branch" )
set( FG_EXTERNALS_USE_PREBUILD OFF CACHE BOOL "use stable prebuild libraries" )

# test & samples dependencies
set( FG_ENABLE_TESTS ON CACHE BOOL "enable tests" )
set( FG_ENABLE_GLFW ON CACHE BOOL "use glfw (optional, required for tests)" )
set( FG_ENABLE_SDL2 OFF CACHE BOOL "use SDL2 (optional, required for tests)" )
set( FG_ENABLE_LODEPNG OFF CACHE BOOL "use lodepng (optional, may be used in tests)" )
set( FG_ENABLE_GRAPHVIZ OFF CACHE BOOL "use graphviz (optional)" )

# UI dependencies
set( FG_ENABLE_IMGUI OFF CACHE BOOL "use imgui (optional, required for UI extension)" )

# Scene dependencies
set( FG_ENABLE_ASSIMP ON CACHE BOOL "use Assimp (optional, required for Scene extension)" )
set( FG_ENABLE_GLM ON CACHE BOOL "use glm (optional, required for Scene extension)" )
set( FG_ENABLE_DEVIL ON CACHE BOOL "use DevIL (optional, required for Scene extension)" )
set( FG_ENABLE_FREEIMAGE OFF CACHE BOOL "use FreeImage (optional, required for Scene extension)" )


if (${FG_ENABLE_SIMPLE_COMPILER_OPTIONS})
	include( "cmake/compilers_minimal.cmake" )
else()
	include( "cmake/compilers.cmake" )
endif()

include( "cmake/compiler_tests.cmake" )

set( CMAKE_DEBUG_POSTFIX "d" )
set( CMAKE_RELEASE_POSTFIX "" )
set( CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "installation directory" )
set( MAIN_BINARY_DIR "${CMAKE_BINARY_DIR}/bin" CACHE INTERNAL "" FORCE )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
add_subdirectory( "external" "external" )

add_subdirectory( "stl" "stl" )

add_subdirectory( "framegraph" "framegraph" )

add_subdirectory( "extensions" "extensions" )

message( STATUS "project 'FrameGraph' generation ended" )
message( STATUS "\n==========================================================================" )


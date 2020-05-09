cmake_minimum_required( VERSION 3.11 FATAL_ERROR )

message( STATUS "==========================================================================\n" )
message( STATUS "project 'FrameGraph' generation started" )

project( "FrameGraph"
		 VERSION 2020.1.0
		 LANGUAGES C CXX)

set_property( GLOBAL PROPERTY USE_FOLDERS ON )
message( STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER_VERSION})" )
message( STATUS "target system: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_VERSION})" )
message( STATUS "host system: ${CMAKE_HOST_SYSTEM_NAME} (${CMAKE_HOST_SYSTEM_VERSION})" )

set( FG_ENABLE_STDALLOC OFF CACHE BOOL "custom std allocators (optional)" )
set( FG_ENABLE_GLSLANG ON CACHE BOOL "use glslang (optional, required for glsl compilation)" )
set( FG_ENABLE_VMA ON CACHE BOOL "use Vulkan Memory Allocator (required)" )
set( FG_ENABLE_OPENVR ON CACHE BOOL "use OpenVR (optional)" )
set( FG_ENABLE_SIMPLE_COMPILER_OPTIONS OFF CACHE BOOL "use simplified compiler settings if you have problem with it" )
set( FG_EXTERNALS_USE_STABLE_VERSIONS ON CACHE BOOL "use last stable version instead of master branch" )
set( FG_EXTERNALS_USE_PREBUILD OFF CACHE BOOL "use stable prebuild libraries" )
set( FG_ENABLE_GLSL_TRACE ON CACHE BOOL "used for shader debugging and profiling" )
set( FG_USE_VULKAN_SDK OFF CACHE BOOL "use vulkan headers and glslang source from VulkanSDK" )


# test & samples dependencies
set( FG_ENABLE_TESTS ON CACHE BOOL "enable tests" )
set( FG_ENABLE_GLFW ON CACHE BOOL "use glfw (optional, required for tests)" )
set( FG_ENABLE_SDL2 OFF CACHE BOOL "use SDL2 (optional, required for tests)" )
set( FG_ENABLE_LODEPNG OFF CACHE BOOL "use lodepng (optional, may be used in tests)" )
set( FG_ENABLE_GRAPHVIZ OFF CACHE BOOL "use graphviz (optional)" )
set( FG_ENABLE_SPIRVREFLECT ON CACHE BOOL "for runtime spirv shader reflection (optional)" )

# UI dependencies
set( FG_ENABLE_IMGUI ON CACHE BOOL "use imgui (optional, required for UI extension)" )

# Scene dependencies
set( FG_ENABLE_ASSIMP ON CACHE BOOL "use Assimp (optional, required for Scene extension)" )
set( FG_ENABLE_GLM ON CACHE BOOL "use glm (optional, required for Scene extension)" )
set( FG_ENABLE_DEVIL ON CACHE BOOL "use DevIL (optional, required for Scene extension)" )
set( FG_ENABLE_FREEIMAGE OFF CACHE BOOL "use FreeImage (optional, GPL license)" )
set( FG_ENABLE_FFMPEG OFF CACHE BOOL "use FFmpeg (optional, LGPL license)" )
set( FG_ALLOW_GPL OFF CACHE BOOL "allow GNU GPL code and libraries" )

# Samples dependencies
set( FG_ENABLE_STB OFF CACHE BOOL "" )
set( FG_ENABLE_TINYOBJ OFF CACHE BOOL "" )


set( CMAKE_DEBUG_POSTFIX "d" )
set( CMAKE_RELEASE_POSTFIX "" )
set( CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "installation directory" FORCE )
set( MAIN_BINARY_DIR "${CMAKE_BINARY_DIR}/bin" CACHE INTERNAL "" FORCE )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )

if (${FG_ENABLE_SIMPLE_COMPILER_OPTIONS})
	include( "cmake/compilers_minimal.cmake" )
else()
	include( "cmake/compilers.cmake" )
endif()
include( "cmake/compiler_tests.cmake" )
include( "cmake/ci_build.cmake" )
include( "cmake/version.cmake" )

add_subdirectory( "external" )
include( "cmake/project_template.cmake" )
add_subdirectory( "stl" )
add_subdirectory( "extensions/vulkan_loader" )
add_subdirectory( "extensions/framework" )
add_subdirectory( "framegraph" )
add_subdirectory( "extensions/pipeline_compiler" )
add_subdirectory( "extensions/pipeline_reflection" )
add_subdirectory( "extensions/scene" )
add_subdirectory( "extensions/ui" )
add_subdirectory( "extensions/graphviz" )
add_subdirectory( "extensions/video" )

if (${FG_ENABLE_TESTS})
	enable_testing()
	add_subdirectory( "tests/stl" )
	add_subdirectory( "tests/framegraph" )
	add_subdirectory( "tests/framework" )
	add_subdirectory( "tests/pipeline_compiler" )
	add_subdirectory( "tests/pipeline_reflection" )
	add_subdirectory( "tests/scene" )
	add_subdirectory( "tests/ui" )
endif ()

message( STATUS "project 'FrameGraph' generation ended" )
message( STATUS "\n==========================================================================" )


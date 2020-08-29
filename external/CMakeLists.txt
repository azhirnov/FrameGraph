cmake_minimum_required( VERSION 3.11 FATAL_ERROR )

set( FG_EXTERNALS_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Path to external projects sources" )
set( FG_EXTERNALS_INSTALL_PATH "${CMAKE_BINARY_DIR}/external_libs" CACHE PATH "Path to install external projects libraries" )
#message( STATUS "FG_EXTERNALS_PATH: ${FG_EXTERNALS_PATH}" )
#message( STATUS "FG_EXTERNALS_INSTALL_PATH: ${FG_EXTERNALS_INSTALL_PATH}" )
mark_as_advanced( FG_EXTERNALS_PATH FG_EXTERNALS_INSTALL_PATH )
mark_as_advanced( BUILD_SHARED_LIBS LIB_SUFFIX )

# prepare for FetchContent
if (TRUE)
	include(FetchContent)
	#set( FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "don't download externals" )
	set( FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "don't update externals" )
	mark_as_advanced( FETCHCONTENT_BASE_DIR FETCHCONTENT_FULLY_DISCONNECTED )
	mark_as_advanced( FETCHCONTENT_QUIET FETCHCONTENT_UPDATES_DISCONNECTED )
endif ()


# prepare for external projects
if (TRUE)
	include( ExternalProject )

	set( FG_LIST_SEPARATOR "|" )
	string( REPLACE ";" "${FG_LIST_SEPARATOR}" FG_EXTERNAL_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" )
	set( FG_BUILD_TARGET_FLAGS 	"-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
								"-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
								"-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}"
								"-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
								"-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}"
								"-DCMAKE_STATIC_LINKER_FLAGS=${CMAKE_STATIC_LINKER_FLAGS}"
								"-DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}" )

	foreach ( CONFIG ${CMAKE_CONFIGURATION_TYPES} )
		string( TOUPPER ${CONFIG} OUT_CONFIG )

		set( FG_BUILD_TARGET_FLAGS
			"${FG_BUILD_TARGET_FLAGS}"
			"-DCMAKE_C_FLAGS_${OUT_CONFIG}=${CMAKE_C_FLAGS_${OUT_CONFIG}}"
			"-DCMAKE_CXX_FLAGS_${OUT_CONFIG}=${CMAKE_CXX_FLAGS_${OUT_CONFIG}}"
			"-DCMAKE_EXE_LINKER_FLAGS_${OUT_CONFIG}=${CMAKE_EXE_LINKER_FLAGS_${OUT_CONFIG}}"
			"-DCMAKE_STATIC_LINKER_FLAGS_${OUT_CONFIG}=${CMAKE_STATIC_LINKER_FLAGS_${OUT_CONFIG}}"
			"-DCMAKE_SHARED_LINKER_FLAGS_${OUT_CONFIG}=${CMAKE_SHARED_LINKER_FLAGS_${OUT_CONFIG}}" )
	endforeach ()
endif ()


set( CMAKE_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../cmake" )

include( "${CMAKE_FOLDER}/download_prebuild.cmake" )
include( "${CMAKE_FOLDER}/download_stdoptional.cmake" )
include( "${CMAKE_FOLDER}/download_stdvariant.cmake" )
include( "${CMAKE_FOLDER}/download_stdstringview.cmake" )
include( "${CMAKE_FOLDER}/download_filesystem.cmake" )
#include( "${CMAKE_FOLDER}/download_stdalloc.cmake" )
include( "${CMAKE_FOLDER}/download_glslang.cmake" )
include( "${CMAKE_FOLDER}/download_glsltrace.cmake" )
include( "${CMAKE_FOLDER}/download_vma.cmake" )
include( "${CMAKE_FOLDER}/download_sdl2.cmake" )
include( "${CMAKE_FOLDER}/download_glfw.cmake" )
include( "${CMAKE_FOLDER}/download_lodepng.cmake" )
include( "${CMAKE_FOLDER}/download_assimp.cmake" )
include( "${CMAKE_FOLDER}/download_imgui.cmake" )
include( "${CMAKE_FOLDER}/download_glm.cmake" )
include( "${CMAKE_FOLDER}/download_devil.cmake" )
include( "${CMAKE_FOLDER}/download_freeimage.cmake" )
include( "${CMAKE_FOLDER}/graphviz.cmake" )
include( "${CMAKE_FOLDER}/download_openvr.cmake" )
include( "${CMAKE_FOLDER}/download_ffmpeg.cmake" )
include( "${CMAKE_FOLDER}/download_spirv-cross.cmake" )
include( "${CMAKE_FOLDER}/download_spirv-reflect.cmake" )

install( DIRECTORY "${FG_EXTERNALS_INSTALL_PATH}" DESTINATION "${CMAKE_INSTALL_PREFIX}" )

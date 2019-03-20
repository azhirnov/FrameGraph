cmake_minimum_required (VERSION 3.6.0)

set( FG_GLOBAL_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" )
set( FG_EXTERNALS_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Path to external projects sources" )
set( FG_EXTERNALS_INSTALL_PATH "${CMAKE_BINARY_DIR}/lib" CACHE PATH "Path to install external projects libraries" )
#message( STATUS "FG_EXTERNALS_PATH: ${FG_EXTERNALS_PATH}" )
#message( STATUS "FG_EXTERNALS_INSTALL_PATH: ${FG_EXTERNALS_INSTALL_PATH}" )


# prepare for FetchContent
if (NOT CMAKE_VERSION VERSION_LESS 3.11.0)
	include(FetchContent)
	#set( FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "don't download externals" )
	set( FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "don't update externals" )
endif ()


# prepare for external projects
if (TRUE)
	include( ExternalProject )

	set( FG_LIST_SEPARATOR "|" )
	string( REPLACE ";" "${FG_LIST_SEPARATOR}" FG_EXTERNAL_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" )
	set( FG_BUILD_TARGET_FLAGS 	"-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
								"-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}" )

	foreach ( CONFIG ${CMAKE_CONFIGURATION_TYPES} )
		string( TOUPPER ${CONFIG} OUT_CONFIG )

		set( FG_BUILD_TARGET_FLAGS
			"${FG_BUILD_TARGET_FLAGS}"
			"-DCMAKE_C_FLAGS_${OUT_CONFIG}=${CMAKE_C_FLAGS_${OUT_CONFIG}}"
			"-DCMAKE_CXX_FLAGS_${OUT_CONFIG}=${CMAKE_CXX_FLAGS_${OUT_CONFIG}}"
			"-DCMAKE_EXE_LINKER_FLAGS_${OUT_CONFIG}=${CMAKE_EXE_LINKER_FLAGS_${OUT_CONFIG}}"
			"-DCMAKE_STATIC_LINKER_FLAGS_${OUT_CONFIG}=${CMAKE_STATIC_LINKER_FLAGS_${OUT_CONFIG}}"
			"-DCMAKE_SHARED_LINKER_FLAGS_${OUT_CONFIG}=${CMAKE_SHARED_LINKER_FLAGS_${OUT_CONFIG}}"
		)
	endforeach ()
endif ()


set( CMAKE_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../cmake" )

include( "${CMAKE_FOLDER}/download_stdoptional.cmake" )
include( "${CMAKE_FOLDER}/download_stdvariant.cmake" )
include( "${CMAKE_FOLDER}/download_mem.cmake" )
include( "${CMAKE_FOLDER}/download_vk.cmake" )
include( "${CMAKE_FOLDER}/download_glslang.cmake" )
include( "${CMAKE_FOLDER}/download_vma.cmake" )
include( "${CMAKE_FOLDER}/download_sdl2.cmake" )
include( "${CMAKE_FOLDER}/download_glfw.cmake" )
include( "${CMAKE_FOLDER}/download_sfml.cmake" )
include( "${CMAKE_FOLDER}/download_lodepng.cmake" )
include( "${CMAKE_FOLDER}/download_assimp.cmake" )
include( "${CMAKE_FOLDER}/download_imgui.cmake" )
include( "${CMAKE_FOLDER}/download_glm.cmake" )
include( "${CMAKE_FOLDER}/download_devil.cmake" )
include( "${CMAKE_FOLDER}/download_freeimage.cmake" )
include( "${CMAKE_FOLDER}/graphviz.cmake" )
include( "${CMAKE_FOLDER}/download_angelscript.cmake" )


set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" CACHE INTERNAL "" FORCE )

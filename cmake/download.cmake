# Download externals using cmake 3.11 feature - FetchContent

include( FetchContent )

#set( FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "don't download externals" )
set( FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "don't update externals" )


# Vulkan Memory Allocator
if (${FG_ENABLE_VMA})
	FetchContent_Declare( ExternalDownloadVulkanMemoryAllocator
		GIT_REPOSITORY		https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
		SOURCE_DIR			"${FG_DOWNLOAD_PATH}/VulkanMemoryAllocator"
		GIT_TAG				master
	)
	
	FetchContent_GetProperties( ExternalDownloadVulkanMemoryAllocator )
	if (NOT ExternalDownloadVulkanMemoryAllocator_POPULATED)
		FetchContent_Populate( ExternalDownloadVulkanMemoryAllocator )
	endif ()

	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DFG_ENABLE_VULKAN_MEMORY_ALLOCATOR" )
endif ()


# GLSLANG
if (${FG_ENABLE_GLSLANG})
	# SPIRV-Tools require Python 2.7 for building
	find_package( PythonInterp 2.7 REQUIRED )
	find_package( PythonLibs 2.7 REQUIRED )
	
	FetchContent_Declare( ExternalDownloadGLSLang
		GIT_REPOSITORY		https://github.com/KhronosGroup/glslang.git
		GIT_TAG				master
		SOURCE_DIR			"${FG_DOWNLOAD_PATH}/glslang"
	)
	FetchContent_GetProperties( ExternalDownloadGLSLang )
	if (NOT ExternalDownloadGLSLang_POPULATED)
		FetchContent_Populate( ExternalDownloadGLSLang )
	endif ()
	
	FetchContent_Declare( ExternalDownloadSPIRVTools
		GIT_REPOSITORY		https://github.com/KhronosGroup/SPIRV-Tools.git
		GIT_TAG				master
		SOURCE_DIR			"${FG_DOWNLOAD_PATH}/glslang/External/SPIRV-Tools"
	)
	FetchContent_GetProperties( ExternalDownloadSPIRVTools )
	if (NOT ExternalDownloadSPIRVTools_POPULATED)
		FetchContent_Populate( ExternalDownloadSPIRVTools )
	endif ()
	
	FetchContent_Declare( ExternalDownloadSPIRVHeaders
		GIT_REPOSITORY		https://github.com/KhronosGroup/SPIRV-Headers.git
		GIT_TAG				master
		SOURCE_DIR			"${FG_DOWNLOAD_PATH}/glslang/External/SPIRV-Tools/external/SPIRV-Headers"
	)
	FetchContent_GetProperties( ExternalDownloadSPIRVHeaders )
	if (NOT ExternalDownloadSPIRVHeaders_POPULATED)
		FetchContent_Populate( ExternalDownloadSPIRVHeaders )
	endif ()
	
	set( BUILD_TESTING OFF CACHE BOOL "glslang option" )
	set( ENABLE_AMD_EXTENSIONS ON CACHE BOOL "glslang option" )
	set( ENABLE_NV_EXTENSIONS ON CACHE BOOL "glslang option" )
	set( ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "glslang option" )
	set( ENABLE_HLSL ON CACHE BOOL "glslang option" )
	set( ENABLE_OPT ON CACHE BOOL "glslang option" )
	set( ENABLE_SPVREMAPPER OFF CACHE BOOL "glslang option" )
	set( SKIP_GLSLANG_INSTALL ON CACHE BOOL "glslang option" )
	set( SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "glslang option" )
	set( SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "glslang option" )
	set( SPIRV_SKIP_TESTS ON CACHE BOOL "glslang option" )
	set( SPIRV_TOOLS_INSTALL_EMACS_HELPERS OFF CACHE BOOL "glslang option" )
	set( SPIRV_WERROR ON CACHE BOOL "glslang option" )
	
	add_subdirectory( "${FG_DOWNLOAD_PATH}/glslang" "glslang" )
	set_property( TARGET "glslang" PROPERTY FOLDER "External" )
	set_property( TARGET "OGLCompiler" PROPERTY FOLDER "External" )
	set_property( TARGET "OSDependent" PROPERTY FOLDER "External" )
	set_property( TARGET "SPIRV" PROPERTY FOLDER "External" )
	
	if (${ENABLE_HLSL})
		set_property( TARGET "HLSL" PROPERTY FOLDER "External" )
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DENABLE_HLSL" )
	endif ()
	
	if (${ENABLE_OPT})
		set_property( TARGET "spirv-tools-pkg-config" PROPERTY FOLDER "External" )
		set_property( TARGET "spirv-tools-shared-pkg-config" PROPERTY FOLDER "External" )
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DENABLE_OPT" )
	endif ()

	if (${ENABLE_AMD_EXTENSIONS})
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DAMD_EXTENSIONS" )
	endif ()

	if (${ENABLE_NV_EXTENSIONS})
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DNV_EXTENSIONS" )
	endif ()

	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DFG_ENABLE_GLSLANG" )
endif ()


# GLFW
if (${FG_ENABLE_GLFW})
	FetchContent_Declare( ExternalDownloadGLFW
		GIT_REPOSITORY		https://github.com/glfw/glfw.git
		GIT_TAG				master
		SOURCE_DIR			"${FG_DOWNLOAD_PATH}/glfw"
	)
	
	FetchContent_GetProperties( ExternalDownloadGLFW )
	if (NOT ExternalDownloadGLFW_POPULATED)
		FetchContent_Populate( ExternalDownloadGLFW )
	endif ()
		
	set( GLFW_INSTALL OFF CACHE BOOL "glfw option" )
	set( GLFW_BUILD_TESTS OFF CACHE BOOL "glfw option" )
	set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "glfw option" )
	set( GLFW_BUILD_DOCS OFF CACHE BOOL "glfw option" )
	
	add_subdirectory( "${FG_DOWNLOAD_PATH}/glfw" "glfw" )
	set_property( TARGET "glfw" PROPERTY FOLDER "External" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DFG_ENABLE_GLFW" )
endif ()


# SDL2
if (${FG_ENABLE_SDL2})
	FetchContent_Declare( ExternalDownloadSDL2
		URL					https://www.libsdl.org/release/SDL2-2.0.8.zip
		SOURCE_DIR			"${FG_DOWNLOAD_PATH}/SDL2"
	)
	
	FetchContent_GetProperties( ExternalDownloadSDL2 )
	if (NOT ExternalDownloadSDL2_POPULATED)
		FetchContent_Populate( ExternalDownloadSDL2 )
	endif ()
	
	set( SDL_SHARED OFF CACHE BOOL "SDL2 option" )
	set( SDL_STATIC ON CACHE BOOL "SDL2 option" )

	add_subdirectory( "${FG_DOWNLOAD_PATH}/SDL2" "SDL2" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DFG_ENABLE_SDL2" )

	if (${SDL_SHARED})
		set_property( TARGET "SDL2" PROPERTY FOLDER "External" )
	endif ()

	set_property( TARGET "SDL2main" PROPERTY FOLDER "External" )
	set_property( TARGET "uninstall" PROPERTY FOLDER "External" )

	if (${SDL_STATIC})
		set_property( TARGET "SDL2-static" PROPERTY FOLDER "External" )
	endif ()
endif ()


# LodePNG
if (${FG_ENABLE_LODEPNG})
	FetchContent_Declare( ExternalDownloadLodePNG
		GIT_REPOSITORY		https://github.com/lvandeve/lodepng.git
		GIT_TAG				master
		SOURCE_DIR			"${FG_DOWNLOAD_PATH}/lodepng"
		PATCH_COMMAND		${CMAKE_COMMAND} -E copy
							${CMAKE_SOURCE_DIR}/cmake/lodepng_CMakeLists.txt
							${FG_DOWNLOAD_PATH}/lodepng/CMakeLists.txt
	)
	
	FetchContent_GetProperties( ExternalDownloadLodePNG )
	if (NOT ExternalDownloadLodePNG_POPULATED)
		FetchContent_Populate( ExternalDownloadLodePNG )
	endif ()
	
	add_subdirectory( "${FG_DOWNLOAD_PATH}/lodepng" "lodepng" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DFG_ENABLE_LODEPNG" )
endif ()


set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" CACHE INTERNAL "" FORCE )

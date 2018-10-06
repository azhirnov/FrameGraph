# find or download GLSLANG

if (${FG_ENABLE_GLSLANG})
	set( FG_EXTERNAL_GLSLANG_PATH "" CACHE PATH "path to glslang source" )

	# SPIRV-Tools require Python 2.7 for building
	find_package( PythonInterp 2.7 REQUIRED )
	find_package( PythonLibs 2.7 REQUIRED )
	
	# reset to default
	if (NOT EXISTS ${FG_EXTERNAL_GLSLANG_PATH})
		message( STATUS "glslang is not found in ${FG_EXTERNAL_GLSLANG_PATH}" )
		set( FG_EXTERNAL_GLSLANG_PATH "${FG_EXTERNALS_PATH}/glslang" CACHE PATH "" FORCE )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_GLSLANG_PATH}" AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
		FetchContent_Declare( ExternalDownloadGLSLang
			GIT_REPOSITORY		https://github.com/KhronosGroup/glslang.git
			GIT_TAG				master
			SOURCE_DIR			"${FG_EXTERNAL_GLSLANG_PATH}"
		)
		FetchContent_GetProperties( ExternalDownloadGLSLang )
		if (NOT ExternalDownloadGLSLang_POPULATED)
			message( STATUS "downloading glslang" )
			FetchContent_Populate( ExternalDownloadGLSLang )
		endif ()
		
		FetchContent_Declare( ExternalDownloadSPIRVTools
			GIT_REPOSITORY		https://github.com/KhronosGroup/SPIRV-Tools.git
			GIT_TAG				master
			SOURCE_DIR			"${FG_EXTERNAL_GLSLANG_PATH}/External/SPIRV-Tools"
		)
		FetchContent_GetProperties( ExternalDownloadSPIRVTools )
		if (NOT ExternalDownloadSPIRVTools_POPULATED)
			message( STATUS "downloading SPIRV-Tools" )
			FetchContent_Populate( ExternalDownloadSPIRVTools )
		endif ()
		
		FetchContent_Declare( ExternalDownloadSPIRVHeaders
			GIT_REPOSITORY		https://github.com/KhronosGroup/SPIRV-Headers.git
			GIT_TAG				master
			SOURCE_DIR			"${FG_EXTERNAL_GLSLANG_PATH}/External/SPIRV-Tools/external/SPIRV-Headers"
		)
		FetchContent_GetProperties( ExternalDownloadSPIRVHeaders )
		if (NOT ExternalDownloadSPIRVHeaders_POPULATED)
			message( STATUS "downloading SPIRV-Headers" )
			FetchContent_Populate( ExternalDownloadSPIRVHeaders )
		endif ()
	endif ()
	
	set( BUILD_TESTING OFF CACHE BOOL "glslang option" FORCE )
	set( ENABLE_AMD_EXTENSIONS ON CACHE BOOL "glslang option" )
	set( ENABLE_NV_EXTENSIONS ON CACHE BOOL "glslang option" )
	set( ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "glslang option" FORCE )
	set( ENABLE_HLSL ON CACHE BOOL "glslang option" )
	set( ENABLE_OPT ON CACHE BOOL "glslang option" )
	set( ENABLE_SPVREMAPPER OFF CACHE BOOL "glslang option" FORCE )
	set( SKIP_GLSLANG_INSTALL ON CACHE BOOL "glslang option" )
	set( SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "glslang option" )
	set( SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "glslang option" FORCE )
	set( SPIRV_SKIP_TESTS ON CACHE BOOL "glslang option" FORCE )
	set( SPIRV_TOOLS_INSTALL_EMACS_HELPERS OFF CACHE BOOL "glslang option" FORCE )
	set( SPIRV_WERROR ON CACHE BOOL "glslang option" )
	
	add_subdirectory( "${FG_EXTERNAL_GLSLANG_PATH}" "glslang" )
	set_property( TARGET "glslang" PROPERTY FOLDER "External" )
	set_property( TARGET "OGLCompiler" PROPERTY FOLDER "External" )
	set_property( TARGET "OSDependent" PROPERTY FOLDER "External" )
	set_property( TARGET "SPIRV" PROPERTY FOLDER "External" )
	
	if (${ENABLE_HLSL})
		set_property( TARGET "HLSL" PROPERTY FOLDER "External" )
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "ENABLE_HLSL" )
	endif ()
	
	if (NOT UNIX AND ${ENABLE_OPT})  # TODO: fix fir linux
		set_property( TARGET "spirv-tools-pkg-config" PROPERTY FOLDER "External" )
		set_property( TARGET "spirv-tools-shared-pkg-config" PROPERTY FOLDER "External" )
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "ENABLE_OPT" )
	endif ()

	if (${ENABLE_AMD_EXTENSIONS})
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "AMD_EXTENSIONS" )
	endif ()

	if (${ENABLE_NV_EXTENSIONS})
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "NV_EXTENSIONS" )
	endif ()

	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_GLSLANG" )
endif ()

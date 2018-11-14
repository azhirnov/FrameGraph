# find or download GLSLANG
#[[
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
			GIT_PROGRESS		1
			GIT_SHALLOW			1
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
			GIT_PROGRESS		1
			GIT_SHALLOW			1
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
			GIT_PROGRESS		1
			GIT_SHALLOW			1
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
]]


# GLSLANG
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

	ExternalProject_Add( "External.glslang"
		# download
		GIT_REPOSITORY		https://github.com/KhronosGroup/glslang.git
		GIT_TAG				master
		GIT_PROGRESS		1
		EXCLUDE_FROM_ALL	1
		LOG_DOWNLOAD		1
		# update
		PATCH_COMMAND		""
		UPDATE_DISCONNECTED	1
		LOG_UPDATE			1
		# configure
        SOURCE_DIR			"${FG_EXTERNAL_GLSLANG_PATH}"
		CONFIGURE_COMMAND	""
		# build
		BINARY_DIR			""
		BUILD_COMMAND		""
		INSTALL_COMMAND		""
		TEST_COMMAND		""
	)
	
	ExternalProject_Add( "External.SPIRV-Tools"
		DEPENDS				"External.glslang"
		# download
		GIT_REPOSITORY		https://github.com/KhronosGroup/SPIRV-Tools.git
		GIT_TAG				master
		GIT_PROGRESS		1
		EXCLUDE_FROM_ALL	1
		LOG_DOWNLOAD		1
		# update
		PATCH_COMMAND		""
		UPDATE_DISCONNECTED	1
		LOG_UPDATE			1
		# configure
        SOURCE_DIR			"${FG_EXTERNAL_GLSLANG_PATH}/External/SPIRV-Tools"
		CONFIGURE_COMMAND	""
		# build
		BINARY_DIR			""
		BUILD_COMMAND		""
		INSTALL_COMMAND		""
		TEST_COMMAND		""
	)
	
	ExternalProject_Add( "External.SPIRV-Headers"
		DEPENDS				"External.glslang"
							"External.SPIRV-Tools"
		# download
		GIT_REPOSITORY		https://github.com/KhronosGroup/SPIRV-Headers.git
		GIT_TAG				master
		GIT_PROGRESS		1
		EXCLUDE_FROM_ALL	1
		LOG_DOWNLOAD		1
		# update
		PATCH_COMMAND		""
		UPDATE_DISCONNECTED	1
		LOG_UPDATE			1
		# configure
        SOURCE_DIR			"${FG_EXTERNAL_GLSLANG_PATH}/External/SPIRV-Tools/external/SPIRV-Headers"
		CONFIGURE_COMMAND	""
		# build
		BINARY_DIR			""
		BUILD_COMMAND		""
		INSTALL_COMMAND		""
		TEST_COMMAND		""
	)
	
    set( FG_GLSLANG_INSTALL_DIR "${FG_EXTERNALS_INSTALL_PATH}/glslang" CACHE INTERNAL "" FORCE )

	ExternalProject_Add( "External.glslang-main"
        LIST_SEPARATOR      "${FG_LIST_SEPARATOR}"
		DEPENDS				"External.glslang"
							"External.SPIRV-Tools"
							"External.SPIRV-Headers"
		# configure
        SOURCE_DIR          "${FG_EXTERNAL_GLSLANG_PATH}"
		CMAKE_GENERATOR		"${CMAKE_GENERATOR}"
		CMAKE_GENERATOR_TOOLSET	"${CMAKE_GENERATOR_TOOLSET}"
        CMAKE_ARGS			"-DCMAKE_CONFIGURATION_TYPES=${FG_EXTERNAL_CONFIGURATION_TYPES}"
							"-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
							"-DCMAKE_DEBUG_POSTFIX="
							"-DCMAKE_RELEASE_POSTFIX="
							"-DCMAKE_INSTALL_PREFIX=${FG_GLSLANG_INSTALL_DIR}"
							"-DENABLE_AMD_EXTENSIONS=ON"
							"-DENABLE_NV_EXTENSIONS=ON"
							"-DENABLE_HLSL=ON"
							"-DENABLE_OPT=ON"
							"-DENABLE_SPVREMAPPER=ON"
							"-DENABLE_GLSLANG_BINARIES=ON"
							"-DSKIP_GLSLANG_INSTALL=OFF"
							"-DSKIP_SPIRV_TOOLS_INSTALL=OFF"
							"-DSPIRV_SKIP_EXECUTABLES=OFF"
							"-DSPIRV_SKIP_TESTS=ON"
							"-DBUILD_TESTING=OFF"
                            ${FG_BUILD_TARGET_FLAGS}
		LOG_CONFIGURE 		1
		# build
		BINARY_DIR			"${CMAKE_BINARY_DIR}/build-glslang"
		BUILD_COMMAND		"${CMAKE_COMMAND}"
							--build .
							--target ALL_BUILD
							--config $<CONFIG>
		LOG_BUILD 			1
		# install
		INSTALL_DIR 		"${FG_GLSLANG_INSTALL_DIR}"
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)

	set_property( TARGET "External.SPIRV-Headers" PROPERTY FOLDER "External" )
	set_property( TARGET "External.SPIRV-Tools" PROPERTY FOLDER "External" )
	set_property( TARGET "External.glslang" PROPERTY FOLDER "External" )
	set_property( TARGET "External.glslang-main" PROPERTY FOLDER "External" )
	
	if (NOT UNIX AND ${ENABLE_OPT})  # TODO: fix fir linux
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "ENABLE_OPT" )
	endif ()

	if (${ENABLE_AMD_EXTENSIONS})
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "AMD_EXTENSIONS" )
	endif ()

	if (${ENABLE_NV_EXTENSIONS})
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "NV_EXTENSIONS" )
	endif ()

	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_GLSLANG" )
	set( FG_GLSLANG_INCLUDE_DIR "${FG_GLSLANG_INSTALL_DIR}/include" CACHE INTERNAL "" FORCE )
	set( FG_GLSLANG_LIBRARY_DIR "${FG_GLSLANG_INSTALL_DIR}/lib" CACHE INTERNAL "" FORCE )
endif ()

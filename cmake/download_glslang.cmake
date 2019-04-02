# find or download GLSLANG

# GLSLANG
if (${FG_ENABLE_GLSLANG})
	set( FG_EXTERNAL_GLSLANG_PATH "" CACHE PATH "path to glslang source" )

	# SPIRV-Tools require Python 2.7 for building
	find_package( PythonInterp 2.7 REQUIRED )
	find_package( PythonLibs 2.7 REQUIRED )
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_GLSLANG_PATH}/CMakeLists.txt")
		message( STATUS "glslang is not found in \"${FG_EXTERNAL_GLSLANG_PATH}\"" )
		set( FG_EXTERNAL_GLSLANG_PATH "${FG_EXTERNALS_PATH}/glslang" CACHE PATH "" FORCE )
		set( FG_GLSLANG_REPOSITORY "https://github.com/KhronosGroup/glslang.git" )
	else ()
		set( FG_GLSLANG_REPOSITORY "" )
	endif ()
	
	if (NOT EXISTS "${FG_EXTERNAL_GLSLANG_PATH}/External/SPIRV-Tools/include")
		set( FG_SPIRVTOOLS_REPOSITORY "https://github.com/KhronosGroup/SPIRV-Tools.git" )
	else ()
		set( FG_SPIRVTOOLS_REPOSITORY "" )
	endif ()
	
	if (NOT EXISTS "${FG_EXTERNAL_GLSLANG_PATH}/External/SPIRV-Tools/external/SPIRV-Headers/include")
		set( FG_SPIRVHEADERS_REPOSITORY "https://github.com/KhronosGroup/SPIRV-Headers.git" )
	else ()
		set( FG_SPIRVHEADERS_REPOSITORY "" )
	endif ()
	
	set( ENABLE_HLSL ON CACHE BOOL "glslang option" )
	set( ENABLE_OPT ON CACHE BOOL "glslang option" )
	set( ENABLE_AMD_EXTENSIONS ON CACHE BOOL "glslang option" )
	set( ENABLE_NV_EXTENSIONS ON CACHE BOOL "glslang option" )

	if (${FG_EXTERNALS_USE_STABLE_VERSIONS})
		# stable release February 8, 2019
		set( GLSLANG_TAG "7.11.3113" )
		set( SPIRV_TOOLS_TAG "v2019.1" )
		set( SPIRV_HEADERS_TAG "1.3.7" )
	else ()
		set( GLSLANG_TAG "master" )
		set( SPIRV_TOOLS_TAG "master" )
		set( SPIRV_HEADERS_TAG "master" )
	endif ()

	ExternalProject_Add( "External.glslang"
		# download
		GIT_REPOSITORY		${FG_GLSLANG_REPOSITORY}
		GIT_TAG				${GLSLANG_TAG}
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
		GIT_REPOSITORY		${FG_SPIRVTOOLS_REPOSITORY}
		GIT_TAG				${SPIRV_TOOLS_TAG}
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
		GIT_REPOSITORY		${FG_SPIRVHEADERS_REPOSITORY}
		GIT_TAG				${SPIRV_HEADERS_TAG}
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
		LIST_SEPARATOR		"${FG_LIST_SEPARATOR}"
		DEPENDS				"External.glslang"
							"External.SPIRV-Tools"
							"External.SPIRV-Headers"
		EXCLUDE_FROM_ALL	1
		# configure
		SOURCE_DIR			"${FG_EXTERNAL_GLSLANG_PATH}"
		CMAKE_GENERATOR		"${CMAKE_GENERATOR}"
		CMAKE_GENERATOR_TOOLSET	"${CMAKE_GENERATOR_TOOLSET}"
		CMAKE_ARGS			"-DCMAKE_CONFIGURATION_TYPES=${FG_EXTERNAL_CONFIGURATION_TYPES}"
							"-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
							"-DCMAKE_INSTALL_PREFIX=${FG_GLSLANG_INSTALL_DIR}"
							"-DENABLE_AMD_EXTENSIONS=${ENABLE_AMD_EXTENSIONS}"
							"-DENABLE_NV_EXTENSIONS=${ENABLE_NV_EXTENSIONS}"
							"-DENABLE_HLSL=${ENABLE_HLSL}"
							"-DENABLE_OPT=${ENABLE_OPT}"
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
							--target glslang
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
	
	if (NOT UNIX AND ${ENABLE_OPT})
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "ENABLE_OPT" )
	endif ()

	if (${ENABLE_HLSL})
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "ENABLE_HLSL" )
	endif ()

	if (${ENABLE_AMD_EXTENSIONS})
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "AMD_EXTENSIONS" )
	endif ()

	if (${ENABLE_NV_EXTENSIONS})
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "NV_EXTENSIONS" )
	endif ()

	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_GLSLANG" )
	set( FG_GLSLANG_INCLUDE_DIR "${FG_GLSLANG_INSTALL_DIR}/include" CACHE INTERNAL "" FORCE )
	

	# glslang libraries
	set( FG_GLSLANG_LIBNAMES "SPIRV" "glslang" "OSDependent" "HLSL" )
	
	if (UNIX)
		set( FG_GLSLANG_LIBNAMES "${FG_GLSLANG_LIBNAMES}" "pthread" )
	endif ()

	set( FG_GLSLANG_LIBNAMES "${FG_GLSLANG_LIBNAMES}" "OGLCompiler" )
								
	if (NOT UNIX AND ${ENABLE_OPT})
		set( FG_GLSLANG_LIBNAMES "${FG_GLSLANG_LIBNAMES}" "SPIRV-Tools" "SPIRV-Tools-opt" )
	endif ()

	if (MSVC)
		set( DBG_POSTFIX "${CMAKE_DEBUG_POSTFIX}" )
	else ()
		set( DBG_POSTFIX "" )
	endif ()

	set( FG_GLSLANG_LIBRARIES "" )
	foreach ( LIBNAME ${FG_GLSLANG_LIBNAMES} )
		if ( ${LIBNAME} STREQUAL "pthread" )
			set( FG_GLSLANG_LIBRARIES	"${FG_GLSLANG_LIBRARIES}" "${LIBNAME}" )
		else ()
			set( FG_GLSLANG_LIBRARIES	"${FG_GLSLANG_LIBRARIES}"
										"$<$<CONFIG:Debug>:${FG_GLSLANG_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${LIBNAME}${DBG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}>"
										"$<$<CONFIG:Profile>:${FG_GLSLANG_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}>"
										"$<$<CONFIG:Release>:${FG_GLSLANG_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${LIBNAME}${CMAKE_STATIC_LIBRARY_SUFFIX}>" )
		endif ()
	endforeach ()

	set( FG_GLSLANG_LIBRARIES "${FG_GLSLANG_LIBRARIES}" CACHE INTERNAL "" FORCE )

endif ()

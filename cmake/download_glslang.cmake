# find or download GLSLANG

if (${FG_EXTERNALS_USE_PREBUILD} AND ${FG_ENABLE_GLSLANG})
	add_library( "GLSLang-lib" INTERFACE )
	target_include_directories( "GLSLang-lib" INTERFACE "${FG_EXTERNAL_PREBUILD_PATH}/glslang/include" )
	target_compile_definitions( "GLSLang-lib" INTERFACE FG_ENABLE_GLSLANG ENABLE_HLSL )
	set( FG_GLSLANG_LIBRARIES
		"${FG_EXTERNAL_PREBUILD_PATH}/glslang/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SPIRV${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FG_EXTERNAL_PREBUILD_PATH}/glslang/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glslang${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FG_EXTERNAL_PREBUILD_PATH}/glslang/lib/${CMAKE_STATIC_LIBRARY_PREFIX}OSDependent${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FG_EXTERNAL_PREBUILD_PATH}/glslang/lib/${CMAKE_STATIC_LIBRARY_PREFIX}HLSL${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	if (UNIX)
		set( FG_GLSLANG_LIBNAMES "${FG_GLSLANG_LIBNAMES}" "pthread" )
	endif ()
	set( FG_GLSLANG_LIBRARIES ${FG_GLSLANG_LIBRARIES}
		"${FG_EXTERNAL_PREBUILD_PATH}/glslang/lib/${CMAKE_STATIC_LIBRARY_PREFIX}OGLCompiler${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FG_EXTERNAL_PREBUILD_PATH}/glslang/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SPIRV-Tools${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FG_EXTERNAL_PREBUILD_PATH}/glslang/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SPIRV-Tools-opt${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	set_property( TARGET "GLSLang-lib" PROPERTY INTERFACE_LINK_LIBRARIES "${FG_GLSLANG_LIBRARIES}" )


elseif (${FG_ENABLE_GLSLANG})
	set( FG_EXTERNAL_GLSLANG_PATH "" CACHE PATH "path to glslang source" )
	mark_as_advanced( FG_EXTERNAL_GLSLANG_PATH )

	# SPIRV-Tools require Python 3 for building
	find_package( PythonInterp 3.7 REQUIRED )
	find_package( PythonLibs 3.7 REQUIRED )
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_GLSLANG_PATH}/CMakeLists.txt")
		message( STATUS "glslang is not found in \"${FG_EXTERNAL_GLSLANG_PATH}\"" )
		set( FG_EXTERNAL_GLSLANG_PATH "${FG_EXTERNALS_PATH}/glslang" CACHE PATH "" FORCE )
	else ()
		message( STATUS "glslang found in \"${FG_EXTERNAL_GLSLANG_PATH}\"" )
	endif ()
	
	if (${FG_USE_VULKAN_SDK})
		find_package( Vulkan )
		if (Vulkan_FOUND)
			message( STATUS "used glslang from VulkanSDK" )
			set( FG_EXTERNAL_GLSLANG_PATH "${Vulkan_INCLUDE_DIR}/../glslang" CACHE PATH "" FORCE )
		endif ()
	endif ()

	if (NOT EXISTS "${FG_EXTERNAL_GLSLANG_PATH}/CMakeLists.txt")
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
	set( ENABLE_OPT OFF CACHE BOOL "glslang option" FORCE )
	mark_as_advanced( ENABLE_HLSL ENABLE_OPT )

	if (${FG_EXTERNALS_USE_STABLE_VERSIONS})
		# stable release January 22, 2020
		set( GLSLANG_TAG "8.13.3559" )
		set( SPIRV_TOOLS_TAG "v2019.4" )
		set( SPIRV_HEADERS_TAG "1.5.1.corrected" )
	else ()
		set( GLSLANG_TAG "master" )
		set( SPIRV_TOOLS_TAG "master" )
		set( SPIRV_HEADERS_TAG "master" )
	endif ()

	ExternalProject_Add( "External.glslang"
		LOG_OUTPUT_ON_FAILURE 1
		# download
		GIT_REPOSITORY		${FG_GLSLANG_REPOSITORY}
		GIT_TAG				${GLSLANG_TAG}
		GIT_PROGRESS		1
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
		LOG_OUTPUT_ON_FAILURE 1
		DEPENDS				"External.glslang"
		# download
		GIT_REPOSITORY		${FG_SPIRVTOOLS_REPOSITORY}
		GIT_TAG				${SPIRV_TOOLS_TAG}
		GIT_PROGRESS		1
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
		LOG_OUTPUT_ON_FAILURE 1
		DEPENDS				"External.glslang"
							"External.SPIRV-Tools"
		# download
		GIT_REPOSITORY		${FG_SPIRVHEADERS_REPOSITORY}
		GIT_TAG				${SPIRV_HEADERS_TAG}
		GIT_PROGRESS		1
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
	
	set( FG_GLSLANG_INSTALL_DIR "${FG_EXTERNALS_INSTALL_PATH}/glslang" )

	ExternalProject_Add( "External.glslang-main"
		LIST_SEPARATOR		"${FG_LIST_SEPARATOR}"
		LOG_OUTPUT_ON_FAILURE 1
		DEPENDS				"External.glslang"
							"External.SPIRV-Tools"
							"External.SPIRV-Headers"
		# configure
		SOURCE_DIR			"${FG_EXTERNAL_GLSLANG_PATH}"
		CMAKE_GENERATOR		"${CMAKE_GENERATOR}"
		CMAKE_GENERATOR_PLATFORM "${CMAKE_GENERATOR_PLATFORM}"
		CMAKE_GENERATOR_TOOLSET	"${CMAKE_GENERATOR_TOOLSET}"
		CMAKE_ARGS			"-DCMAKE_CONFIGURATION_TYPES=${FG_EXTERNAL_CONFIGURATION_TYPES}"
							"-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
							"-DCMAKE_INSTALL_PREFIX=${FG_GLSLANG_INSTALL_DIR}"
							"-DENABLE_HLSL=${ENABLE_HLSL}"
							"-DENABLE_OPT=${ENABLE_OPT}"
							"-DENABLE_SPVREMAPPER=ON"
							"-DENABLE_GLSLANG_BINARIES=OFF"
							"-DSKIP_GLSLANG_INSTALL=OFF"
							"-DSKIP_SPIRV_TOOLS_INSTALL=OFF"
							"-DSPIRV_SKIP_EXECUTABLES=OFF"
							"-DSPIRV_SKIP_TESTS=ON"
							"-DBUILD_TESTING=OFF"
							${FG_BUILD_TARGET_FLAGS}
		LOG_CONFIGURE 		1
		# build
		BINARY_DIR			"${CMAKE_BINARY_DIR}/build-glslang"
		BUILD_COMMAND		${CMAKE_COMMAND}
							--build .
							--config $<CONFIG>
							--target glslang
		LOG_BUILD 			1
		# install
		INSTALL_DIR 		"${FG_GLSLANG_INSTALL_DIR}"
		INSTALL_COMMAND		${CMAKE_COMMAND}
							--build .
							--config $<CONFIG>
							--target glslang
							install
							COMMAND ${CMAKE_COMMAND} -E copy_if_different
								"${FG_EXTERNAL_GLSLANG_PATH}/StandAlone/ResourceLimits.h"
								"${FG_GLSLANG_INSTALL_DIR}/include/StandAlone/ResourceLimits.h"
							COMMAND ${CMAKE_COMMAND} -E copy_if_different
								"${FG_EXTERNAL_GLSLANG_PATH}/StandAlone/ResourceLimits.cpp"
								"${FG_GLSLANG_INSTALL_DIR}/include/StandAlone/ResourceLimits.cpp"
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)

	set_property( TARGET "External.SPIRV-Headers" PROPERTY FOLDER "External" )
	set_property( TARGET "External.SPIRV-Tools" PROPERTY FOLDER "External" )
	set_property( TARGET "External.glslang" PROPERTY FOLDER "External" )
	set_property( TARGET "External.glslang-main" PROPERTY FOLDER "External" )

	set( FG_GLSLANG_DEFINITIONS "FG_ENABLE_GLSLANG" )
	
	if (${ENABLE_OPT})
		set( FG_GLSLANG_DEFINITIONS "${FG_GLSLANG_DEFINITIONS}" "ENABLE_OPT" )
	endif ()

	if (${ENABLE_HLSL})
		set( FG_GLSLANG_DEFINITIONS "${FG_GLSLANG_DEFINITIONS}" "ENABLE_HLSL" )
	endif ()

	# glslang libraries
	set( FG_GLSLANG_LIBNAMES "SPIRV" "glslang" "OSDependent" "HLSL" )
	
	if (UNIX)
		set( FG_GLSLANG_LIBNAMES "${FG_GLSLANG_LIBNAMES}" "pthread" )
	endif ()

	set( FG_GLSLANG_LIBNAMES "${FG_GLSLANG_LIBNAMES}" "OGLCompiler" )
								
	if (${ENABLE_OPT})
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
	
	add_library( "GLSLang-lib" INTERFACE )
	set_property( TARGET "GLSLang-lib" PROPERTY INTERFACE_LINK_LIBRARIES "${FG_GLSLANG_LIBRARIES}" )
	target_include_directories( "GLSLang-lib" INTERFACE "${FG_GLSLANG_INSTALL_DIR}/include" )
	target_compile_definitions( "GLSLang-lib" INTERFACE "${FG_GLSLANG_DEFINITIONS}" )
	add_dependencies( "GLSLang-lib" "External.glslang-main" )

endif ()

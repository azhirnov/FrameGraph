# download and install SPIRV-Reflect

if (${FG_ENABLE_SPIRVREFLECT})
	set( FG_EXTERNAL_SPIRVREFLECT_PATH "" CACHE PATH "path to SPIRV-Reflect source" )
	mark_as_advanced( FG_EXTERNAL_SPIRVREFLECT_PATH )
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_SPIRVREFLECT_PATH}/spirv_reflect.h")
		message( STATUS "SPIRV-Reflect is not found in \"${FG_EXTERNAL_SPIRVREFLECT_PATH}\"" )
		set( FG_EXTERNAL_SPIRVREFLECT_PATH "${FG_EXTERNALS_PATH}/SPIRV-Reflect" CACHE PATH "" FORCE )
		set( FG_SPIRVREFLECT_REPOSITORY "https://github.com/chaoticbob/SPIRV-Reflect.git" )
	else ()
		set( FG_SPIRVREFLECT_REPOSITORY "" )
	endif ()
	
	set( FG_SPIRVREFLECT_INSTALL_DIR "${FG_EXTERNALS_INSTALL_PATH}/SPIRV-Reflect" )

	ExternalProject_Add( "External.SPIRV-Reflect"
		LOG_OUTPUT_ON_FAILURE 1
        LIST_SEPARATOR		"${FG_LIST_SEPARATOR}"
		# download
		GIT_REPOSITORY		${FG_SPIRVREFLECT_REPOSITORY}
		GIT_TAG				master
		GIT_PROGRESS		1
		# update
		PATCH_COMMAND		""
		UPDATE_DISCONNECTED	1
		# configure
        SOURCE_DIR			"${FG_EXTERNAL_SPIRVREFLECT_PATH}"
		CMAKE_GENERATOR		"${CMAKE_GENERATOR}"
		CMAKE_GENERATOR_PLATFORM "${CMAKE_GENERATOR_PLATFORM}"
		CMAKE_GENERATOR_TOOLSET	"${CMAKE_GENERATOR_TOOLSET}"
        CMAKE_ARGS			"-DCMAKE_CONFIGURATION_TYPES=${FG_EXTERNAL_CONFIGURATION_TYPES}"
							"-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
							"-DCMAKE_DEBUG_POSTFIX="
							"-DCMAKE_RELEASE_POSTFIX="
                            ${FG_BUILD_TARGET_FLAGS}
		LOG_CONFIGURE 		1
		# build
		BINARY_DIR			"${CMAKE_BINARY_DIR}/build-SPIRV-Reflect"
		BUILD_COMMAND		""
		LOG_BUILD 			1
		# install
		INSTALL_DIR 		""
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)
	set_property( TARGET "External.SPIRV-Reflect" PROPERTY FOLDER "External" )

	add_library( "SPIRV-Reflect-lib" STATIC
		"${FG_EXTERNAL_SPIRVREFLECT_PATH}/spirv_reflect.c"
		"${FG_EXTERNAL_SPIRVREFLECT_PATH}/spirv_reflect.h" )

	target_include_directories( "SPIRV-Reflect-lib" PUBLIC "${FG_EXTERNAL_SPIRVREFLECT_PATH}" )
	target_compile_definitions( "SPIRV-Reflect-lib" PUBLIC FG_ENABLE_SPIRV_REFLECT )
	set_property( TARGET "SPIRV-Reflect-lib" PROPERTY FOLDER "External" )

	add_dependencies( "SPIRV-Reflect-lib" "External.SPIRV-Reflect" )
endif ()

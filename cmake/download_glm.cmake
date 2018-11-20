# find or download glm

if (${FG_ENABLE_GLM})
	set( FG_EXTERNAL_GLM_PATH "" CACHE PATH "path to glm source" )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_GLM_PATH}/glm/glm.hpp")
		message( STATUS "glm is not found in \"${FG_EXTERNAL_GLM_PATH}\"" )
		set( FG_EXTERNAL_GLM_PATH "${FG_EXTERNALS_PATH}/glm" CACHE PATH "" FORCE )
		set( FG_GLM_REPOSITORY "https://github.com/g-truc/glm.git" )
	else ()
		set( FG_GLM_REPOSITORY "" )
	endif ()

	ExternalProject_Add( "External.GLM"
        LIST_SEPARATOR		"${FG_LIST_SEPARATOR}"
		# download
		GIT_REPOSITORY		${FG_GLM_REPOSITORY}
		GIT_TAG				master
		GIT_PROGRESS		1
		EXCLUDE_FROM_ALL	1
		LOG_DOWNLOAD		1
		# update
		PATCH_COMMAND		""
		UPDATE_DISCONNECTED	1
		# configure
        SOURCE_DIR			"${FG_EXTERNAL_GLM_PATH}"
		CMAKE_GENERATOR		"${CMAKE_GENERATOR}"
		CMAKE_GENERATOR_TOOLSET	"${CMAKE_GENERATOR_TOOLSET}"
        CMAKE_ARGS			"-DCMAKE_CONFIGURATION_TYPES=${FG_EXTERNAL_CONFIGURATION_TYPES}"
							"-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
							"-DCMAKE_DEBUG_POSTFIX="
							"-DCMAKE_RELEASE_POSTFIX="
                            ${FG_BUILD_TARGET_FLAGS}
		LOG_CONFIGURE 		1
		# build
		BINARY_DIR			"${CMAKE_BINARY_DIR}/build-glm"
		BUILD_COMMAND		""
		LOG_BUILD 			1
		# install
		INSTALL_DIR 		""
		INSTALL_COMMAND		""
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)
	
	set_property( TARGET "External.GLM" PROPERTY FOLDER "External" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_GLM" )
	set( FG_GLM_INCLUDE_DIR "${FG_EXTERNAL_GLM_PATH}/glm" CACHE INTERNAL "" FORCE )
endif ()

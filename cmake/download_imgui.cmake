# find or download imgui

if (${FG_ENABLE_IMGUI})
	set( FG_EXTERNAL_IMGUI_PATH "" CACHE PATH "path to imgui source" )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_IMGUI_PATH}/imgui.h")
		message( STATUS "imgui is not found in \"${FG_EXTERNAL_IMGUI_PATH}\"" )
		set( FG_EXTERNAL_IMGUI_PATH "${FG_EXTERNALS_PATH}/imgui" CACHE PATH "" FORCE )
		set( FG_IMGUI_REPOSITORY "https://github.com/ocornut/imgui.git" )
	else ()
		set( FG_IMGUI_REPOSITORY "" )
	endif ()

	ExternalProject_Add( "External.imgui"
        LIST_SEPARATOR		"${FG_LIST_SEPARATOR}"
		# download
		GIT_REPOSITORY		${FG_IMGUI_REPOSITORY}
		GIT_TAG				master
		GIT_PROGRESS		1
		EXCLUDE_FROM_ALL	1
		LOG_DOWNLOAD		1
		# update
		PATCH_COMMAND		""
		UPDATE_DISCONNECTED	1
		# configure
        SOURCE_DIR			"${FG_EXTERNAL_IMGUI_PATH}"
		CMAKE_GENERATOR		"${CMAKE_GENERATOR}"
		CMAKE_GENERATOR_TOOLSET	"${CMAKE_GENERATOR_TOOLSET}"
        CMAKE_ARGS			"-DCMAKE_CONFIGURATION_TYPES=${FG_EXTERNAL_CONFIGURATION_TYPES}"
							"-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
							"-DCMAKE_DEBUG_POSTFIX="
							"-DCMAKE_RELEASE_POSTFIX="
                            ${FG_BUILD_TARGET_FLAGS}
		LOG_CONFIGURE 		1
		# build
		BINARY_DIR			"${CMAKE_BINARY_DIR}/build-imgui"
		BUILD_COMMAND		""
		LOG_BUILD 			1
		# install
		INSTALL_DIR 		""
		INSTALL_COMMAND		""
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)
	
	set_property( TARGET "External.imgui" PROPERTY FOLDER "External" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_IMGUI" )
endif ()

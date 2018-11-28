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
		DOWNLOAD_DIR		"${FG_EXTERNAL_IMGUI_PATH}"
		GIT_REPOSITORY		${FG_IMGUI_REPOSITORY}
		GIT_TAG				master
		GIT_PROGRESS		1
		EXCLUDE_FROM_ALL	1
		LOG_DOWNLOAD		1
		# update
		PATCH_COMMAND		""
		UPDATE_DISCONNECTED	1
		LOG_UPDATE			1
		# configure
		SOURCE_DIR			"${FG_EXTERNAL_IMGUI_PATH}"
		CONFIGURE_COMMAND	""
		# build
		BINARY_DIR			""
		BUILD_COMMAND		""
		INSTALL_COMMAND		""
		TEST_COMMAND		""
	)
	
	set_property( TARGET "External.imgui" PROPERTY FOLDER "External" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_IMGUI" )
	set( FG_IMGUI_SOURCE_DIR "${FG_EXTERNAL_IMGUI_PATH}" CACHE INTERNAL "" FORCE)
endif ()

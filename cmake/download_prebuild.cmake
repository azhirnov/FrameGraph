# clone repository with precompiled external dependencies

if (${FG_EXTERNALS_USE_PREBUILD})

	set( FG_EXTERNAL_PREBUILD_PATH "" CACHE PATH "path to prebuilded external libraries" )
	mark_as_advanced( FG_EXTERNAL_PREBUILD_PATH )

	# set tag
	set( FGEXTERNAL_TAG "master" )

	if (MSVC AND ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
		if (NOT ${CMAKE_GENERATOR_TOOLSET} STREQUAL "v141")
			message( FATAL_ERROR "Toolset ${CMAKE_GENERATOR_TOOLSET} is not supported, required v141 toolset" )
		endif ()
		if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
			set( FGEXTERNAL_TAG "win64-msvc-v141" )
		elseif (${CMAKE_SIZEOF_VOID_P} EQUAL 4)
			set( FGEXTERNAL_TAG "win32-msvc-v141" )
		endif ()
	endif ()
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_PREBUILD_PATH}")
		message( STATUS "prebuild is not found in \"${FG_EXTERNAL_PREBUILD_PATH}\"" )
		set( FG_EXTERNAL_PREBUILD_PATH "${FG_EXTERNALS_PATH}/prebuild/${FGEXTERNAL_TAG}" CACHE PATH "" FORCE )
	else ()
		message( STATUS "prebuild found in \"${FG_EXTERNAL_PREBUILD_PATH}\"" )
	endif ()

	ExternalProject_Add( "FG.External"
		# download
		GIT_REPOSITORY		"https://github.com/azhirnov/FrameGraph-External.git"
		GIT_TAG				${FGEXTERNAL_TAG}
		GIT_PROGRESS		1
		EXCLUDE_FROM_ALL	1
		LOG_DOWNLOAD		1
		# update
		PATCH_COMMAND		""
		UPDATE_COMMAND		git lfs pull
		UPDATE_DISCONNECTED	1
		LOG_UPDATE			1
		# configure
		SOURCE_DIR			"${FG_EXTERNAL_PREBUILD_PATH}"
		CONFIGURE_COMMAND	""
		# build
		BINARY_DIR			""
		BUILD_COMMAND		""
		INSTALL_COMMAND		""
		TEST_COMMAND		""
	)
	
	set_property( TARGET "FG.External" PROPERTY FOLDER "External" )

endif ()

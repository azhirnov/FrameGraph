# find or download stb

if (${FG_ENABLE_STB})
	set( FG_EXTERNAL_STB_PATH "" CACHE PATH "path to stb source" )
	mark_as_advanced( FG_EXTERNAL_STB_PATH )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_STB_PATH}/stb.h")
		message( STATUS "stb is not found in \"${FG_EXTERNAL_STB_PATH}\"" )
		set( FG_EXTERNAL_STB_PATH "${FG_EXTERNALS_PATH}/stb" CACHE PATH "" FORCE )
	else ()
		message( STATUS "stb found in \"${FG_EXTERNAL_STB_PATH}\"" )
	endif ()
	
	# select version
	if (${FG_EXTERNALS_USE_STABLE_VERSIONS})
		set( STB_TAG "master" )
	else ()
		set( STB_TAG "master" )
	endif ()

	if (NOT EXISTS "${FG_EXTERNAL_STB_PATH}/stb.h")
		set( FG_STB_REPOSITORY "https://github.com/nothings/stb.git" )
	else ()
		set( FG_STB_REPOSITORY "" )
	endif ()

	ExternalProject_Add( "External.STB"
		LIST_SEPARATOR		"${FG_LIST_SEPARATOR}"
		LOG_OUTPUT_ON_FAILURE 1
		# download
		GIT_REPOSITORY		${FG_STB_REPOSITORY}
		GIT_TAG				${STB_TAG}
		GIT_PROGRESS		1
		# update
		PATCH_COMMAND		""
		UPDATE_DISCONNECTED	1
		# configure
		SOURCE_DIR			"${FG_EXTERNAL_STB_PATH}"
		CONFIGURE_COMMAND	""
		LOG_CONFIGURE 		1
		# build
		BINARY_DIR			"${CMAKE_BINARY_DIR}/build-stb"
		BUILD_COMMAND		""
		LOG_BUILD 			1
		# install
		INSTALL_DIR 		""
		INSTALL_COMMAND		""
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)
	
	set_property( TARGET "External.STB" PROPERTY FOLDER "External" )

	add_library( "STB-lib" INTERFACE )
	target_include_directories( "STB-lib" INTERFACE "${FG_EXTERNAL_STB_PATH}" )
	target_compile_definitions( "STB-lib" INTERFACE "FG_ENABLE_STB" )
	add_dependencies( "STB-lib" "External.STB" )
endif ()

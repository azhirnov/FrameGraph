# download filesystem (MIT license)

if (TRUE)
#if (NOT ${STD_FILESYSTEM_SUPPORTED})
	set( FG_EXTERNAL_GFS_PATH "" CACHE PATH "path to FileSystem source" )
	mark_as_advanced( FG_EXTERNAL_GFS_PATH )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_GFS_PATH}/include/ghc/filesystem.hpp")
		message( STATUS "FileSystem is not found in \"${FG_EXTERNAL_GFS_PATH}\"" )
		set( FG_EXTERNAL_GFS_PATH "${FG_EXTERNALS_PATH}/FileSystem" CACHE PATH "" FORCE )
	else ()
		message( STATUS "FileSystem found in \"${FG_EXTERNAL_GFS_PATH}\"" )
	endif ()

	if (NOT EXISTS "${FG_EXTERNAL_GFS_PATH}/include/ghc/filesystem.hpp")
		set( FG_GFS_REPOSITORY "https://github.com/gulrak/filesystem.git" )
	else ()
		set( FG_GFS_REPOSITORY "" )
	endif ()
	
	# select version
	if (${FG_EXTERNALS_USE_STABLE_VERSIONS})
		set( GFS_TAG "v1.3.0" )
	else ()
		set( GFS_TAG "master" )
	endif ()

	set( FG_GFS_INSTALL_DIR "${FG_EXTERNALS_INSTALL_PATH}/FileSystem" )

	ExternalProject_Add( "External.FileSystem"
		LIST_SEPARATOR		"${FG_LIST_SEPARATOR}"
		LOG_OUTPUT_ON_FAILURE 1
		# download
		GIT_REPOSITORY		${FG_GFS_REPOSITORY}
		GIT_TAG				${GFS_TAG}
		GIT_PROGRESS		1
		# update
		UPDATE_DISCONNECTED	1
		# configure
		SOURCE_DIR			"${FG_EXTERNAL_GFS_PATH}"
		CMAKE_GENERATOR		"${CMAKE_GENERATOR}"
		CMAKE_GENERATOR_PLATFORM "${CMAKE_GENERATOR_PLATFORM}"
		CMAKE_GENERATOR_TOOLSET	"${CMAKE_GENERATOR_TOOLSET}"
		CMAKE_ARGS			"-DCMAKE_CONFIGURATION_TYPES=${FG_EXTERNAL_CONFIGURATION_TYPES}"
							"-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
							"-DCMAKE_INSTALL_PREFIX=${FG_GFS_INSTALL_DIR}"
							${FG_BUILD_TARGET_FLAGS}
		LOG_CONFIGURE 		1
		# build
		BINARY_DIR			"${CMAKE_BINARY_DIR}/build-FileSystem"
		BUILD_COMMAND		${CMAKE_COMMAND}
							--build .
							--target ALL_BUILD
							--config $<CONFIG>
		LOG_BUILD 			1
		# install
		INSTALL_DIR 		"${FG_GFS_INSTALL_DIR}"
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)
	
	set_property( TARGET "External.FileSystem" PROPERTY FOLDER "External" )
		
	add_library( "FileSystem-lib" INTERFACE )
	target_include_directories( "FileSystem-lib" INTERFACE "${FG_GFS_INSTALL_DIR}/include" )
	target_compile_definitions( "FileSystem-lib" INTERFACE "FG_ENABLE_FILESYSTEM" )
	add_dependencies( "FileSystem-lib" "External.FileSystem" )

endif ()

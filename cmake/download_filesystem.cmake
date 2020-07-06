# download filesystem (MIT license)

if (NOT ${STD_FILESYSTEM_SUPPORTED})
	set( FG_EXTERNAL_GFS_PATH "" CACHE PATH "path to std::filesystem source" )
	mark_as_advanced( FG_EXTERNAL_GFS_PATH )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_GFS_PATH}/include/ghc/filesystem.hpp")
		message( STATUS "std::filesystem is not found in \"${FG_EXTERNAL_GFS_PATH}\"" )
		set( FG_EXTERNAL_GFS_PATH "${FG_EXTERNALS_PATH}/FileSystem" CACHE PATH "" FORCE )
	else ()
		message( STATUS "std::filesystem found in \"${FG_EXTERNAL_GFS_PATH}\"" )
	endif ()
	
	# select version
	if (${FG_EXTERNALS_USE_STABLE_VERSIONS})
		set( GFS_TAG "v1.3.0" )
	else ()
		set( GFS_TAG "master" )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_GFS_PATH}/include/ghc/filesystem.hpp")
		FetchContent_Declare( ExternalGFS
			GIT_REPOSITORY		"https://github.com/gulrak/filesystem.git"
			SOURCE_DIR			"${FG_EXTERNAL_GFS_PATH}"
			GIT_TAG				${GFS_TAG}
		)
		
		FetchContent_GetProperties( ExternalGFS )
		if (NOT ExternalGFS_POPULATED)
			message( STATUS "downloading std::filesystem" )
			FetchContent_Populate( ExternalGFS )
		endif ()
	endif ()
		
	add_library( "FileSystem-lib" INTERFACE )
	target_include_directories( "FileSystem-lib" INTERFACE "${FG_EXTERNAL_GFS_PATH}/include" )
	target_compile_definitions( "FileSystem-lib" INTERFACE "FG_ENABLE_FILESYSTEM" )

endif ()

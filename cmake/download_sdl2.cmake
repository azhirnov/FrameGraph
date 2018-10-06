# find or download SDL2

if (${FG_ENABLE_SDL2})
	set( FG_EXTERNAL_SDL2_PATH "" CACHE PATH "path to SDL2 source" )
	
	# reset to default
	if (NOT EXISTS ${FG_EXTERNAL_SDL2_PATH})
		message( STATUS "SDL2 is not found in ${FG_EXTERNAL_SDL2_PATH}" )
		set( FG_EXTERNAL_SDL2_PATH "${FG_EXTERNALS_PATH}/SDL2" CACHE PATH "" FORCE )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_SDL2_PATH}" AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
		FetchContent_Declare( ExternalDownloadSDL2
			URL					https://www.libsdl.org/release/SDL2-2.0.8.zip
			SOURCE_DIR			"${FG_EXTERNAL_SDL2_PATH}"
		)
		
		FetchContent_GetProperties( ExternalDownloadSDL2 )
		if (NOT ExternalDownloadSDL2_POPULATED)
			message( STATUS "downloading SDL2" )
			FetchContent_Populate( ExternalDownloadSDL2 )
		endif ()
	endif ()
	
	set( SDL_SHARED OFF CACHE BOOL "SDL2 option" FORCE )
	set( SDL_STATIC ON CACHE BOOL "SDL2 option" FORCE )

	add_subdirectory( "${FG_EXTERNAL_SDL2_PATH}" "SDL2" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_SDL2" )

	if (${SDL_SHARED})
		set_property( TARGET "SDL2" PROPERTY FOLDER "External" )
	endif ()

	set_property( TARGET "SDL2main" PROPERTY FOLDER "External" )
	set_property( TARGET "uninstall" PROPERTY FOLDER "External" )

	if (${SDL_STATIC})
		set_property( TARGET "SDL2-static" PROPERTY FOLDER "External" )
	endif ()
endif ()
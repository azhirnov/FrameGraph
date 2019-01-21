# find or download SFML

if (${FG_ENABLE_SFML})
	set( FG_EXTERNAL_SFML_PATH "" CACHE PATH "path to SFML source" )
	
	# reset to default
	if (NOT EXISTS ${FG_EXTERNAL_SFML_PATH})
		message( STATUS "SFML is not found in ${FG_EXTERNAL_SFML_PATH}" )
		set( FG_EXTERNAL_SFML_PATH "${FG_EXTERNALS_PATH}/SFML" CACHE PATH "" FORCE )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_SFML_PATH}" AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
		FetchContent_Declare( ExternalSFML
			GIT_REPOSITORY		https://github.com/SFML/SFML.git
			GIT_TAG				master
			SOURCE_DIR			"${FG_EXTERNAL_SFML_PATH}"
		)
		
		FetchContent_GetProperties( ExternalSFML )
		if (NOT ExternalSFML_POPULATED)
			message( STATUS "downloading SFML" )
			FetchContent_Populate( ExternalSFML )
		endif ()
	endif ()

	add_subdirectory( "${FG_EXTERNAL_SFML_PATH}" "SFML" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_SFML" )
endif ()
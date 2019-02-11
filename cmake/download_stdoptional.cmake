# find or download std::optional implementation

if (NOT ${STD_OPTIONAL_SUPPORTED})
	set( FG_EXTERNAL_STDOPTIONAL_PATH "" CACHE PATH "path to std::optional source" )

	# reset to default
	if (NOT EXISTS ${FG_EXTERNAL_STDOPTIONAL_PATH})
		message( STATUS "std::optional is not found in \"${FG_EXTERNAL_STDOPTIONAL_PATH}\"" )
		set( FG_EXTERNAL_STDOPTIONAL_PATH "${FG_EXTERNALS_PATH}/optional" CACHE PATH "" FORCE )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_STDOPTIONAL_PATH}" AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
		FetchContent_Declare( ExternalStdOpt
			GIT_REPOSITORY		https://github.com/akrzemi1/Optional.git
			SOURCE_DIR			"${FG_EXTERNAL_STDOPTIONAL_PATH}"
			GIT_TAG				master
		)
		
		FetchContent_GetProperties( ExternalStdOpt )
		if (NOT ExternalStdOpt_POPULATED)
			message( STATUS "downloading std::optional" )
			FetchContent_Populate( ExternalStdOpt )
		endif ()
	endif ()

	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_OPTIONAL" )
endif ()

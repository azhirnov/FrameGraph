# find or download std::string_view implementation

if (NOT ${STD_STRINGVIEW_SUPPORTED})
	set( FG_EXTERNAL_STDSTRINGVIEW_PATH "" CACHE PATH "path to std::string_view source" )

	# reset to default
	if (NOT EXISTS ${FG_EXTERNAL_STDSTRINGVIEW_PATH})
		message( STATUS "std::string_view is not found in \"${FG_EXTERNAL_STDSTRINGVIEW_PATH}\"" )
		set( FG_EXTERNAL_STDSTRINGVIEW_PATH "${FG_EXTERNALS_PATH}/string_view" CACHE PATH "" FORCE )
	else ()
		message( STATUS "std::string_view found in \"${FG_EXTERNAL_STDSTRINGVIEW_PATH}\"" )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_STDSTRINGVIEW_PATH}" AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
		FetchContent_Declare( ExternalStdStrView
			GIT_REPOSITORY		https://github.com/martinmoene/string-view-lite.git
			SOURCE_DIR			"${FG_EXTERNAL_STDSTRINGVIEW_PATH}"
			GIT_TAG				master
		)
		
		FetchContent_GetProperties( ExternalStdStrView )
		if (NOT ExternalStdStrView_POPULATED)
			message( STATUS "downloading std::string_view" )
			FetchContent_Populate( ExternalStdStrView )
		endif ()
	endif ()

	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_OPTIONAL" )
endif ()
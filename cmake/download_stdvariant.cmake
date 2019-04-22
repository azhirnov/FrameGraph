# find or download std::variant implementation

if (NOT ${STD_VARIANT_SUPPORTED})
	set( FG_EXTERNAL_STDVARIANT_PATH "" CACHE PATH "path to std::variant source" )

	# reset to default
	if (NOT EXISTS ${FG_EXTERNAL_STDVARIANT_PATH})
		message( STATUS "std::variant is not found in \"${FG_EXTERNAL_STDVARIANT_PATH}\"" )
		set( FG_EXTERNAL_STDVARIANT_PATH "${FG_EXTERNALS_PATH}/variant" CACHE PATH "" FORCE )
	else ()
		message( STATUS "std::variant found in \"${FG_EXTERNAL_STDVARIANT_PATH}\"" )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_STDVARIANT_PATH}" AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
		FetchContent_Declare( ExternalStdVar
			GIT_REPOSITORY		https://github.com/mpark/variant.git
			SOURCE_DIR			"${FG_EXTERNAL_STDVARIANT_PATH}"
			GIT_TAG				master
		)
		
		FetchContent_GetProperties( ExternalStdVar )
		if (NOT ExternalStdVar_POPULATED)
			message( STATUS "downloading std::variant" )
			FetchContent_Populate( ExternalStdVar )
		endif ()
	endif ()

	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_VARIANT" )
endif ()

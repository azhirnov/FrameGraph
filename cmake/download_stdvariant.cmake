# find or download std::variant implementation

if (NOT ${STD_VARIANT_SUPPORTED})
	set( FG_EXTERNAL_STDVARIANT_PATH "" CACHE PATH "path to std::variant source" )
	mark_as_advanced( FG_EXTERNAL_STDVARIANT_PATH )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_STDVARIANT_PATH}/include/mpark/variant.hpp")
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

	add_library( "StdVariant-lib" INTERFACE )
	target_include_directories( "StdVariant-lib" INTERFACE "${FG_EXTERNAL_STDVARIANT_PATH}/include" )
	target_compile_definitions( "StdVariant-lib" INTERFACE "FG_ENABLE_VARIANT" )
endif ()

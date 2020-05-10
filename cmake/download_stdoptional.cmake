# find or download std::optional implementation

if (NOT ${STD_OPTIONAL_SUPPORTED})
	set( FG_EXTERNAL_STDOPTIONAL_PATH "" CACHE PATH "path to std::optional source" )
	mark_as_advanced( FG_EXTERNAL_STDOPTIONAL_PATH )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_STDOPTIONAL_PATH}/optional.hpp")
		message( STATUS "std::optional is not found in \"${FG_EXTERNAL_STDOPTIONAL_PATH}\"" )
		set( FG_EXTERNAL_STDOPTIONAL_PATH "${FG_EXTERNALS_PATH}/optional" CACHE PATH "" FORCE )
	else ()
		message( STATUS "std::optional found in \"${FG_EXTERNAL_STDOPTIONAL_PATH}\"" )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_STDOPTIONAL_PATH}/optional.hpp")
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

	add_library( "StdOptional-lib" INTERFACE )
	target_include_directories( "StdOptional-lib" INTERFACE "${FG_EXTERNAL_STDOPTIONAL_PATH}" )
	target_compile_definitions( "StdOptional-lib" INTERFACE "FG_ENABLE_OPTIONAL" )
endif ()

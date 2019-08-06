# find or download LodePNG

if (${FG_ENABLE_LODEPNG})
	set( FG_EXTERNAL_LODEPNG_PATH "" CACHE PATH "path to lodepng source" )
	mark_as_advanced( FG_EXTERNAL_LODEPNG_PATH )
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_LODEPNG_PATH}/lodepng.h")
		message( STATUS "lodepng is not found in \"${FG_EXTERNAL_LODEPNG_PATH}\"" )
		set( FG_EXTERNAL_LODEPNG_PATH "${FG_EXTERNALS_PATH}/lodepng" CACHE PATH "" FORCE )
	else ()
		message( STATUS "lodepng found in \"${FG_EXTERNAL_LODEPNG_PATH}\"" )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_LODEPNG_PATH}" AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
		if (NOT DEFINED CMAKE_FOLDER)
			message( FATAL_ERROR "CMAKE_FOLDER is not defined!" )
		endif ()

		FetchContent_Declare( ExternalLodePNG
			GIT_REPOSITORY		https://github.com/lvandeve/lodepng.git
			GIT_TAG				master
			SOURCE_DIR			"${FG_EXTERNAL_LODEPNG_PATH}"
			PATCH_COMMAND		${CMAKE_COMMAND} -E copy
								${CMAKE_FOLDER}/lodepng_CMakeLists.txt
								${FG_EXTERNAL_LODEPNG_PATH}/CMakeLists.txt
		)
		
		FetchContent_GetProperties( ExternalLodePNG )
		if (NOT ExternalLodePNG_POPULATED)
			message( STATUS "downloading lodepng" )
			FetchContent_Populate( ExternalLodePNG )
		endif ()
	endif ()
	
	add_subdirectory( "${FG_EXTERNAL_LODEPNG_PATH}" "lodepng" )

	add_library( "lodepng-lib" INTERFACE )
	set_property( TARGET "lodepng-lib" PROPERTY INTERFACE_LINK_LIBRARIES "lodepng" )
	target_include_directories( "lodepng-lib" INTERFACE "${FG_EXTERNAL_LODEPNG_PATH}" )
	target_compile_definitions( "lodepng-lib" INTERFACE "FG_ENABLE_LODEPNG" )
endif ()

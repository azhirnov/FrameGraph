# find or download LodePNG

if (${FG_ENABLE_LODEPNG})
	set( FG_EXTERNAL_LODEPNG_PATH "" CACHE PATH "path to lodepng source" )
	
	# reset to default
	if (NOT EXISTS ${FG_EXTERNAL_LODEPNG_PATH})
		message( STATUS "LODEPNG is not found in ${FG_EXTERNAL_LODEPNG_PATH}" )
		set( FG_EXTERNAL_LODEPNG_PATH "${FG_EXTERNALS_PATH}/LODEPNG" CACHE PATH "1" FORCE )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_LODEPNG_PATH}" AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
		FetchContent_Declare( ExternalDownloadLodePNG
			GIT_REPOSITORY		https://github.com/lvandeve/lodepng.git
			GIT_TAG				master
			SOURCE_DIR			"${FG_EXTERNAL_LODEPNG_PATH}"
			PATCH_COMMAND		${CMAKE_COMMAND} -E copy
								${CMAKE_SOURCE_DIR}/cmake/lodepng_CMakeLists.txt
								${FG_EXTERNAL_LODEPNG_PATH}/CMakeLists.txt
		)
		
		FetchContent_GetProperties( ExternalDownloadLodePNG )
		if (NOT ExternalDownloadLodePNG_POPULATED)
			message( STATUS "download lodepng" )
			FetchContent_Populate( ExternalDownloadLodePNG )
		endif ()
	endif ()
	
	add_subdirectory( "${FG_EXTERNAL_LODEPNG_PATH}" "lodepng" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DFG_ENABLE_LODEPNG" )
endif ()
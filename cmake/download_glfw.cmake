# find or download GLFW

if (${FG_ENABLE_GLFW})
	set( FG_EXTERNAL_GLFW_PATH "" CACHE PATH "path to glfw source" )
	
	# reset to default
	if (NOT EXISTS ${FG_EXTERNAL_GLFW_PATH})
		message( STATUS "glfw is not found in \"${FG_EXTERNAL_GLFW_PATH}\"" )
		set( FG_EXTERNAL_GLFW_PATH "${FG_EXTERNALS_PATH}/glfw" CACHE PATH "" FORCE )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_GLFW_PATH}" AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
		FetchContent_Declare( ExternalDownloadGLFW
			GIT_REPOSITORY		https://github.com/glfw/glfw.git
			GIT_TAG				master
			SOURCE_DIR			"${FG_EXTERNAL_GLFW_PATH}"
		)
		
		FetchContent_GetProperties( ExternalDownloadGLFW )
		if (NOT ExternalDownloadGLFW_POPULATED)
			message( STATUS "downloading glfw" )
			FetchContent_Populate( ExternalDownloadGLFW )
		endif ()
	endif ()
		
	set( GLFW_INSTALL ON CACHE BOOL "glfw option" FORCE )
	set( GLFW_BUILD_TESTS OFF CACHE BOOL "glfw option" FORCE )
	set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "glfw option" FORCE )
	set( GLFW_BUILD_DOCS OFF CACHE BOOL "glfw option" FORCE )
	
	add_subdirectory( "${FG_EXTERNAL_GLFW_PATH}" "glfw" )
	set_property( TARGET "glfw" PROPERTY FOLDER "External" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_GLFW" )
endif ()

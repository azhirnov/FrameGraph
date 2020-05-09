# find or download GLFW

if (${FG_ENABLE_GLFW})
	set( FG_EXTERNAL_GLFW_PATH "" CACHE PATH "path to glfw source" )
	mark_as_advanced( FG_EXTERNAL_GLFW_PATH )
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_GLFW_PATH}/include/GLFW/glfw3.h" )
		message( STATUS "glfw is not found in \"${FG_EXTERNAL_GLFW_PATH}\"" )
		set( FG_EXTERNAL_GLFW_PATH "${FG_EXTERNALS_PATH}/glfw" CACHE PATH "" FORCE )
	else ()
		message( STATUS "glfw found in \"${FG_EXTERNAL_GLFW_PATH}\"" )
	endif ()
	
	# select version
	if (${FG_EXTERNALS_USE_STABLE_VERSIONS})
		set( GLFW_TAG "3.2.1" )
	else ()
		set( GLFW_TAG "master" )
	endif ()

	# download
	if (NOT EXISTS "${FG_EXTERNAL_GLFW_PATH}/include/GLFW/glfw3.h")
		FetchContent_Declare( ExternalGLFW
			GIT_REPOSITORY		"https://github.com/glfw/glfw.git"
			GIT_TAG				${GLFW_TAG}
			SOURCE_DIR			"${FG_EXTERNAL_GLFW_PATH}"
		)
		
		FetchContent_GetProperties( ExternalGLFW )
		if (NOT ExternalGLFW_POPULATED)
			message( STATUS "downloading glfw..." )
			FetchContent_Populate( ExternalGLFW )
		endif ()
	endif ()
		
	set( GLFW_INSTALL OFF CACHE INTERNAL "glfw option" FORCE )
	set( GLFW_BUILD_TESTS OFF CACHE BOOL "glfw option" )
	set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "glfw option" )
	set( GLFW_BUILD_DOCS OFF CACHE BOOL "glfw option" )
	set( USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "glfw option" )
	
	add_subdirectory( "${FG_EXTERNAL_GLFW_PATH}" "glfw" )
	set_property( TARGET "glfw" PROPERTY FOLDER "External" )

	if (TARGET uninstall)
		set_property( TARGET "uninstall" PROPERTY FOLDER "External" )
	endif ()
	
	mark_as_advanced( GLFW_INSTALL GLFW_BUILD_TESTS GLFW_BUILD_EXAMPLES GLFW_BUILD_DOCS GLFW_DOCUMENT_INTERNALS )
	mark_as_advanced( GLFW_USE_HYBRID_HPG GLFW_VULKAN_STATIC USE_MSVC_RUNTIME_LIBRARY_DLL )

	add_library( "GLFW-lib" INTERFACE )
	set_property( TARGET "GLFW-lib" PROPERTY INTERFACE_LINK_LIBRARIES "glfw" )
	target_include_directories( "GLFW-lib" INTERFACE "${FG_EXTERNAL_GLFW_PATH}/include" )
	target_compile_definitions( "GLFW-lib" INTERFACE "FG_ENABLE_GLFW" )
endif ()

# find or download GLSL-Trace

if (${FG_ENABLE_GLSL_TRACE})
	set( FG_EXTERNAL_GLSLTRACE_PATH "" CACHE PATH "path to GLSL-Trace source" )
	mark_as_advanced( FG_EXTERNAL_GLSLTRACE_PATH )
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_GLSLTRACE_PATH}/CMakeLists.txt" )
		message( STATUS "GLSL-Trace is not found in \"${FG_EXTERNAL_GLSLTRACE_PATH}\"" )
		set( FG_EXTERNAL_GLSLTRACE_PATH "${FG_EXTERNALS_PATH}/glsl_trace" CACHE PATH "" FORCE )
	else ()
		message( STATUS "GLSL-Trace found in \"${FG_EXTERNAL_GLSLTRACE_PATH}\"" )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_GLSLTRACE_PATH}/CMakeLists.txt")
		FetchContent_Declare( ExternalGLSLTrace
			# download
			URL  				"https://github.com/azhirnov/glsl_trace/archive/master.zip"
			DOWNLOAD_DIR		"${FG_EXTERNAL_GLSLTRACE_PATH}"
			SOURCE_DIR			"${FG_EXTERNAL_GLSLTRACE_PATH}"
			LOG_DOWNLOAD		1
			# build
			BINARY_DIR			""
			BUILD_COMMAND		""
			LOG_BUILD 			1
			# install
			INSTALL_DIR 		""
			LOG_INSTALL 		1
			# test
			TEST_COMMAND		""
		)
		
		FetchContent_GetProperties( ExternalGLSLTrace )
		if (NOT ExternalGLSLTrace_POPULATED)
			message( STATUS "downloading GLSL-Trace..." )
			FetchContent_Populate( ExternalGLSLTrace )
		endif ()
	endif ()
	
	file( GLOB_RECURSE SOURCES "${FG_EXTERNAL_GLSLTRACE_PATH}/shader_trace/*.*" )
	add_library( "GLSL-Trace-lib" STATIC ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	set_property( TARGET "GLSL-Trace-lib" PROPERTY FOLDER "External" )
	target_link_libraries( "GLSL-Trace-lib" PUBLIC "STL" )
	target_link_libraries( "GLSL-Trace-lib" PUBLIC "GLSLang-lib" )
	target_compile_definitions( "GLSL-Trace-lib" INTERFACE "FG_ENABLE_GLSL_TRACE" )
	target_include_directories( "GLSL-Trace-lib" PUBLIC 
								"${FG_EXTERNAL_GLSLTRACE_PATH}/shader_trace"
								"${FG_EXTERNAL_GLSLTRACE_PATH}/shader_trace/include" )

endif ()

# find or download memory allocator library

if (${FG_ENABLE_STDALLOC})
	set( FG_EXTERNAL_STDALLOC_PATH "" CACHE PATH "path to foonathan_memory source" )
	mark_as_advanced( FG_EXTERNAL_STDALLOC_PATH )
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_STDALLOC_PATH}/include/foonathan/memory/config.hpp")
		message( STATUS "foonathan_memory is not found in \"${FG_EXTERNAL_STDALLOC_PATH}\"" )
		set( FG_EXTERNAL_STDALLOC_PATH "${FG_EXTERNALS_PATH}/foonathan_memory" CACHE PATH "" FORCE )
	else ()
		message( STATUS "foonathan_memory found in \"${FG_EXTERNAL_STDALLOC_PATH}\"" )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_STDALLOC_PATH}/include/foonathan/memory/config.hpp")
		FetchContent_Declare( ExternalSTDALLOC
			GIT_REPOSITORY		https://github.com/foonathan/memory.git
			GIT_TAG				master
			SOURCE_DIR			"${FG_EXTERNAL_STDALLOC_PATH}"
		)
		
		FetchContent_GetProperties( ExternalSTDALLOC )
		if (NOT ExternalSTDALLOC_POPULATED)
			message( STATUS "downloading foonathan_memory" )
			FetchContent_Populate( ExternalSTDALLOC )
		endif ()
	endif ()
	
	set( FOONATHAN_MEMORY_BUILD_EXAMPLES OFF CACHE INTERNAL "foonathan_memory option" FORCE )
	set( FOONATHAN_MEMORY_BUILD_TESTS OFF CACHE INTERNAL "foonathan_memory option" FORCE )
	set( FOONATHAN_MEMORY_BUILD_TOOLS OFF CACHE INTERNAL "foonathan_memory option" FORCE )

	add_subdirectory( "${FG_EXTERNAL_STDALLOC_PATH}" "foonathan_memory" )
	set_property( TARGET "foonathan_memory" PROPERTY FOLDER "External" )

	add_library( "StdAlloc-lib" INTERFACE )
	set_property( TARGET "StdAlloc-lib" PROPERTY INTERFACE_LINK_LIBRARIES "foonathan_memory" )
	target_include_directories( "StdAlloc-lib" INTERFACE "${FG_EXTERNAL_STDALLOC_PATH}/include" )
	target_compile_definitions( "StdAlloc-lib" INTERFACE "FG_ENABLE_STDALLOC" )
endif ()
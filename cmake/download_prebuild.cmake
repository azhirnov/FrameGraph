# clone repository with precompiled external dependencies

if (${FG_EXTERNALS_USE_PREBUILD})

	set( FG_EXTERNAL_PREBUILD_PATH "" CACHE PATH "path to prebuilded external libraries" )
	mark_as_advanced( FG_EXTERNAL_PREBUILD_PATH )

	# set tag
	set( FGEXTERNAL_TAG "master" )

	if (MSVC AND ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
		if (NOT ${CMAKE_GENERATOR_TOOLSET} STREQUAL "v141")
			message( FATAL_ERROR "Toolset ${CMAKE_GENERATOR_TOOLSET} is not supported, required v141 toolset" )
		endif ()
		if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
			set( FGEXTERNAL_TAG "win64-msvc-v141" )
		elseif (${CMAKE_SIZEOF_VOID_P} EQUAL 4)
			set( FGEXTERNAL_TAG "win32-msvc-v141" )
		endif ()
	endif ()
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_PREBUILD_PATH}")
		message( STATUS "prebuild is not found in \"${FG_EXTERNAL_PREBUILD_PATH}\"" )
		set( FG_EXTERNAL_PREBUILD_PATH "${FG_EXTERNALS_PATH}/prebuild/${FGEXTERNAL_TAG}" CACHE PATH "" FORCE )
	else ()
		message( STATUS "prebuild found in \"${FG_EXTERNAL_PREBUILD_PATH}\"" )
	endif ()

	
	set( EXTERNAL_REPOSITORY "https://github.com/azhirnov/FrameGraph-External.git" )
	
	if (EXISTS "${FG_EXTERNAL_PREBUILD_PATH}")
		execute_process(
			COMMAND git checkout "${FGEXTERNAL_TAG}"
			WORKING_DIRECTORY "${FG_EXTERNAL_PREBUILD_PATH}"
		)
	else ()
		execute_process(
			COMMAND git clone "${EXTERNAL_REPOSITORY}" "prebuild/${FGEXTERNAL_TAG}" --branch "${FGEXTERNAL_TAG}"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		)
	endif ()

	execute_process(
		COMMAND git lfs pull
		WORKING_DIRECTORY "${FG_EXTERNAL_PREBUILD_PATH}"
	)

endif ()

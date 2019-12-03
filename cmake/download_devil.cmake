# find or download DevIL SDK

if (${FG_ENABLE_DEVIL})
	set( FG_EXTERNAL_DEVIL_PATH "" CACHE PATH "path to DevIL SDK" )
	mark_as_advanced( FG_EXTERNAL_DEVIL_PATH )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_DEVIL_PATH}/include/IL/il.h")
		message( STATUS "DevIL SDK is not found in \"${FG_EXTERNAL_DEVIL_PATH}\"" )
		set( FG_EXTERNAL_DEVIL_PATH "${FG_EXTERNALS_PATH}/DevIL" CACHE PATH "" FORCE )
	else ()
		message( STATUS "DevIL SDK found in \"${FG_EXTERNAL_DEVIL_PATH}\"" )
	endif ()

	if (NOT EXISTS "${FG_EXTERNAL_DEVIL_PATH}/include/IL/il.h")
		set( FG_DEVIL_DOWNLOAD_LINK "https://sourceforge.net/projects/openil/files/DevIL%20Windows%20SDK/1.8.0/DevIL-Windows-SDK-1.8.0.zip/download" )
	else ()
		set( FG_DEVIL_DOWNLOAD_LINK "" )
	endif ()
	
	if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		set( FG_DEVIL_LIBRARY_DIR "${FG_EXTERNAL_DEVIL_PATH}/lib/x86/Release" )
	elseif ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
		set( FG_DEVIL_LIBRARY_DIR "${FG_EXTERNAL_DEVIL_PATH}/lib/x64/Release" )
	else ()
		message( FATAL_ERROR "can't detect 32 or 64-bit platform" )
	endif()

	ExternalProject_Add( "External.DevIL"
		LIST_SEPARATOR		"${FG_LIST_SEPARATOR}"
		LOG_OUTPUT_ON_FAILURE 1
		# download
		URL					"${FG_DEVIL_DOWNLOAD_LINK}"
		DOWNLOAD_DIR		"${FG_EXTERNAL_DEVIL_PATH}"
		SOURCE_DIR			"${FG_EXTERNAL_DEVIL_PATH}"
		LOG_DOWNLOAD		1
		# update
		PATCH_COMMAND		""
		UPDATE_DISCONNECTED	1
		# configure
		CONFIGURE_COMMAND	""
		LOG_CONFIGURE 		1
		# build
		BINARY_DIR			""
		BUILD_COMMAND		""
		LOG_BUILD 			1
		# install
		INSTALL_COMMAND		${CMAKE_COMMAND} -E copy_if_different "${FG_DEVIL_LIBRARY_DIR}/DevIL.dll" "${MAIN_BINARY_DIR}/$<CONFIG>/DevIL.dll"
							COMMAND  ${CMAKE_COMMAND} -E copy_if_different "${FG_DEVIL_LIBRARY_DIR}/ILU.dll" "${MAIN_BINARY_DIR}/$<CONFIG>/ILU.dll"
		INSTALL_DIR 		""
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)
	
	set_property( TARGET "External.DevIL" PROPERTY FOLDER "External" )

	add_library( "DevIL-lib" INTERFACE )
	set_property( TARGET "DevIL-lib" PROPERTY INTERFACE_LINK_LIBRARIES
		"${FG_DEVIL_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}DevIL${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FG_DEVIL_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}ILU${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	target_include_directories( "DevIL-lib" INTERFACE "${FG_EXTERNAL_DEVIL_PATH}/include" )
	target_compile_definitions( "DevIL-lib" INTERFACE "FG_ENABLE_DEVIL" )
	add_dependencies( "DevIL-lib" "External.DevIL" )
endif ()

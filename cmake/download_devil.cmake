# find or download DevIL SDK

if (${FG_ENABLE_DEVIL})
	set( FG_EXTERNAL_DEVIL_PATH "" CACHE PATH "path to DevIL SDK" )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_DEVIL_PATH}/include/IL/il.h")
		message( STATUS "DevIL SDK is not found in \"${FG_EXTERNAL_DEVIL_PATH}\"" )
		set( FG_EXTERNAL_DEVIL_PATH "${FG_EXTERNALS_PATH}/DevIL" CACHE PATH "" FORCE )
	endif ()

	if (NOT EXISTS "${FG_EXTERNAL_DEVIL_PATH}/include/IL/il.h")
		set( FG_DEVIL_DOWNLOAD_LINK "https://sourceforge.net/projects/openil/files/DevIL%20Windows%20SDK/1.8.0/DevIL-Windows-SDK-1.8.0.zip/download" )
	else ()
		set( FG_DEVIL_DOWNLOAD_LINK "" )
	endif ()
	
	set( FG_DEVIL_INCLUDE_DIR "${FG_EXTERNAL_DEVIL_PATH}/include" CACHE INTERNAL "" FORCE )
	
	if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		set( FG_DEVIL_LIBRARY_DIR "${FG_EXTERNAL_DEVIL_PATH}/lib/x86/Release" CACHE INTERNAL "" FORCE )
	elseif ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
		set( FG_DEVIL_LIBRARY_DIR "${FG_EXTERNAL_DEVIL_PATH}/lib/x64/Release" CACHE INTERNAL "" FORCE )
	else ()
		message( FATAL_ERROR "can't detect 32 or 64-bit platform" )
	endif()

	ExternalProject_Add( "External.DevIL"
		LIST_SEPARATOR		"${FG_LIST_SEPARATOR}"
		# download
		URL					"${FG_DEVIL_DOWNLOAD_LINK}"
		DOWNLOAD_DIR		"${FG_EXTERNAL_DEVIL_PATH}"
		SOURCE_DIR			"${FG_EXTERNAL_DEVIL_PATH}"
		EXCLUDE_FROM_ALL	1
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
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_DEVIL" )
endif ()

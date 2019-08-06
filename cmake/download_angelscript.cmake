# find or download AngelScript SDK

if (${FG_EXTERNALS_USE_PREBUILD} AND ${FG_ENABLE_ANGELSCRIPT})
	add_library( "AngelScript-lib" INTERFACE )
	target_include_directories( "AngelScript-lib" INTERFACE "${FG_EXTERNAL_PREBUILD_PATH}/angelscript/include" )
	target_compile_definitions( "AngelScript-lib" INTERFACE "FG_ENABLE_ANGELSCRIPT" )
	add_dependencies( "AngelScript-lib" "FG.External" )
	set_property( TARGET "AngelScript-lib" PROPERTY INTERFACE_LINK_LIBRARIES
		"${FG_EXTERNAL_PREBUILD_PATH}/angelscript/lib/${CMAKE_STATIC_LIBRARY_PREFIX}angelscript${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FG_EXTERNAL_PREBUILD_PATH}/angelscript/lib/${CMAKE_STATIC_LIBRARY_PREFIX}angelscript_addons${CMAKE_STATIC_LIBRARY_SUFFIX}" )


elseif (${FG_ENABLE_ANGELSCRIPT})
	set( FG_EXTERNAL_ANGELSCRIPT_PATH "" CACHE PATH "path to AngelScript SDK" )
	mark_as_advanced( FG_EXTERNAL_ANGELSCRIPT_PATH )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_ANGELSCRIPT_PATH}/angelscript/projects/cmake/CMakeLists.txt")
		message( STATUS "AngelScript SDK is not found in \"${FG_EXTERNAL_ANGELSCRIPT_PATH}\"" )
		set( FG_EXTERNAL_ANGELSCRIPT_PATH "${FG_EXTERNALS_PATH}/AngelScript" CACHE PATH "" FORCE )
	else ()
		message( STATUS "AngelScript SDK found in \"${FG_EXTERNAL_ANGELSCRIPT_PATH}\"" )
	endif ()

	if (NOT EXISTS "${FG_EXTERNAL_ANGELSCRIPT_PATH}/angelscript/projects/cmake/CMakeLists.txt")
		set( FG_ANGELSCRIPT_DOWNLOAD_LINK "https://www.angelcode.com/angelscript/sdk/files/angelscript_2.33.0.zip" )
	else ()
		set( FG_ANGELSCRIPT_DOWNLOAD_LINK "" )
	endif ()
	
	set( FG_ANGELSCRIPT_INSTALL_DIR "${FG_EXTERNALS_INSTALL_PATH}/angelscript" )

	ExternalProject_Add( "External.AngelScript"
		LIST_SEPARATOR		"${FG_LIST_SEPARATOR}"
		# download
		URL					"${FG_ANGELSCRIPT_DOWNLOAD_LINK}"
		DOWNLOAD_DIR		"${FG_EXTERNAL_ANGELSCRIPT_PATH}"
		EXCLUDE_FROM_ALL	1
		LOG_DOWNLOAD		1
		# update
		PATCH_COMMAND		${CMAKE_COMMAND} -E copy
							${CMAKE_FOLDER}/angelscript_CMakeLists.txt
							${FG_EXTERNAL_ANGELSCRIPT_PATH}/CMakeLists.txt
		UPDATE_DISCONNECTED	1
		# configure
		SOURCE_DIR			"${FG_EXTERNAL_ANGELSCRIPT_PATH}"
		CMAKE_GENERATOR		"${CMAKE_GENERATOR}"
		CMAKE_GENERATOR_PLATFORM "${CMAKE_GENERATOR_PLATFORM}"
		CMAKE_GENERATOR_TOOLSET	"${CMAKE_GENERATOR_TOOLSET}"
		CMAKE_ARGS			"-DCMAKE_CONFIGURATION_TYPES=${FG_EXTERNAL_CONFIGURATION_TYPES}"
							"-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
							"-DCMAKE_DEBUG_POSTFIX="
							"-DCMAKE_RELEASE_POSTFIX="
							"-DCMAKE_INSTALL_PREFIX=${FG_ANGELSCRIPT_INSTALL_DIR}"
							${FG_BUILD_TARGET_FLAGS}
		LOG_CONFIGURE 		1
		# build
		BINARY_DIR			"${CMAKE_BINARY_DIR}/build-AngelScript"
		BUILD_COMMAND		"${CMAKE_COMMAND}"
							--build .
							--target ALL_BUILD
							--config $<CONFIG>
		LOG_BUILD 			1
		# install
		INSTALL_DIR 		"${FG_ANGELSCRIPT_INSTALL_DIR}"
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)
	
	set_property( TARGET "External.AngelScript" PROPERTY FOLDER "External" )
	
	add_library( "AngelScript-lib" INTERFACE )
	target_include_directories( "AngelScript-lib" INTERFACE "${FG_ANGELSCRIPT_INSTALL_DIR}/include" )
	target_compile_definitions( "AngelScript-lib" INTERFACE "FG_ENABLE_ANGELSCRIPT" )
	add_dependencies( "AngelScript-lib" "External.AngelScript" )
	set_property( TARGET "AngelScript-lib" PROPERTY INTERFACE_LINK_LIBRARIES
		"${FG_ANGELSCRIPT_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}angelscript${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"${FG_ANGELSCRIPT_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}angelscript_addons${CMAKE_STATIC_LIBRARY_SUFFIX}" )

endif ()

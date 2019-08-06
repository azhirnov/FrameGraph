# find or download imgui

if (${FG_EXTERNALS_USE_PREBUILD} AND ${FG_ENABLE_IMGUI})
	add_library( "imgui-lib" INTERFACE )
	target_include_directories( "imgui-lib" INTERFACE "${FG_EXTERNAL_PREBUILD_PATH}/imgui/include" )
	target_compile_definitions( "imgui-lib" INTERFACE "FG_ENABLE_IMGUI" )
	add_dependencies( "imgui-lib" "FG.External" )
	set_property( TARGET "imgui-lib" PROPERTY INTERFACE_LINK_LIBRARIES
		"${FG_EXTERNAL_PREBUILD_PATH}/imgui/lib/${CMAKE_STATIC_LIBRARY_PREFIX}imgui${CMAKE_STATIC_LIBRARY_SUFFIX}" )


elseif (${FG_ENABLE_IMGUI})
	set( FG_EXTERNAL_IMGUI_PATH "" CACHE PATH "path to imgui source" )
	mark_as_advanced( FG_EXTERNAL_IMGUI_PATH )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_IMGUI_PATH}/imgui.h")
		message( STATUS "imgui is not found in \"${FG_EXTERNAL_IMGUI_PATH}\"" )
		set( FG_EXTERNAL_IMGUI_PATH "${FG_EXTERNALS_PATH}/imgui" CACHE PATH "" FORCE )
	else ()
		message( STATUS "imgui found in \"${FG_EXTERNAL_IMGUI_PATH}\"" )
	endif ()

	if (NOT EXISTS "${FG_EXTERNAL_IMGUI_PATH}/imgui.h")
		set( FG_IMGUI_REPOSITORY "https://github.com/ocornut/imgui.git" )
	else ()
		set( FG_IMGUI_REPOSITORY "" )
	endif ()
	
	set( FG_IMGUI_INSTALL_DIR "${FG_EXTERNALS_INSTALL_PATH}/imgui" )

	ExternalProject_Add( "External.imgui"
		LIST_SEPARATOR		"${FG_LIST_SEPARATOR}"
		# download
		DOWNLOAD_DIR		"${FG_EXTERNAL_IMGUI_PATH}"
		GIT_REPOSITORY		${FG_IMGUI_REPOSITORY}
		GIT_TAG				master
		GIT_PROGRESS		1
		EXCLUDE_FROM_ALL	1
		LOG_DOWNLOAD		1
		# update
		PATCH_COMMAND		${CMAKE_COMMAND} -E copy
							${CMAKE_FOLDER}/imgui_CMakeLists.txt
							${FG_EXTERNAL_IMGUI_PATH}/CMakeLists.txt
		UPDATE_DISCONNECTED	1
		LOG_UPDATE			1
		# configure
		SOURCE_DIR			"${FG_EXTERNAL_IMGUI_PATH}"
		CMAKE_GENERATOR		"${CMAKE_GENERATOR}"
		CMAKE_GENERATOR_PLATFORM "${CMAKE_GENERATOR_PLATFORM}"
		CMAKE_GENERATOR_TOOLSET	"${CMAKE_GENERATOR_TOOLSET}"
		CMAKE_ARGS			"-DCMAKE_CONFIGURATION_TYPES=${FG_EXTERNAL_CONFIGURATION_TYPES}"
							"-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
							"-DCMAKE_INSTALL_PREFIX=${FG_IMGUI_INSTALL_DIR}"
							"-DCMAKE_DEBUG_POSTFIX="
							"-DCMAKE_RELEASE_POSTFIX="
							${FG_BUILD_TARGET_FLAGS}
		LOG_CONFIGURE 		1
		# build
		BINARY_DIR			"${CMAKE_BINARY_DIR}/build-imgui"
		BUILD_COMMAND		"${CMAKE_COMMAND}"
							--build .
							--target imgui
							--config $<CONFIG>
		# install
		INSTALL_DIR 		"${FG_IMGUI_INSTALL_DIR}"
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)
	
	set_property( TARGET "External.imgui" PROPERTY FOLDER "External" )
	
	add_library( "imgui-lib" INTERFACE )
	target_include_directories( "imgui-lib" INTERFACE "${FG_EXTERNAL_IMGUI_PATH}" )
	target_compile_definitions( "imgui-lib" INTERFACE "FG_ENABLE_IMGUI" )
	add_dependencies( "imgui-lib" "External.imgui" )
	set_property( TARGET "imgui-lib" PROPERTY INTERFACE_LINK_LIBRARIES
		"${FG_IMGUI_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}imgui${CMAKE_STATIC_LIBRARY_SUFFIX}" )

endif ()

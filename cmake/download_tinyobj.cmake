# find or download tinyobj loader (MIT license)

if (${FG_ENABLE_TINYOBJ})
	set( FG_EXTERNAL_TINYOBJ_PATH "" CACHE PATH "path to TinyObjLoader source" )
	mark_as_advanced( FG_EXTERNAL_TINYOBJ_PATH )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_TINYOBJ_PATH}/tiny_obj_loader.h")
		message( STATUS "TinyObjLoader is not found in \"${FG_EXTERNAL_TINYOBJ_PATH}\"" )
		set( FG_EXTERNAL_TINYOBJ_PATH "${FG_EXTERNALS_PATH}/tinyobj" CACHE PATH "" FORCE )
	else ()
		message( STATUS "TinyObjLoader found in \"${FG_EXTERNAL_TINYOBJ_PATH}\"" )
	endif ()
	
	# select version
	if (${FG_EXTERNALS_USE_STABLE_VERSIONS})
		set( TINYOBJ_TAG "v2.0.0rc5" )
	else ()
		set( TINYOBJ_TAG "master" )
	endif ()

	if (NOT EXISTS "${FG_EXTERNAL_TINYOBJ_PATH}/tiny_obj_loader.h")
		set( FG_TINYOBJ_REPOSITORY "https://github.com/tinyobjloader/tinyobjloader.git" )
	else ()
		set( FG_TINYOBJ_REPOSITORY "" )
	endif ()

	ExternalProject_Add( "External.TinyObj"
		LIST_SEPARATOR		"${FG_LIST_SEPARATOR}"
		LOG_OUTPUT_ON_FAILURE 1
		# download
		GIT_REPOSITORY		${FG_TINYOBJ_REPOSITORY}
		GIT_TAG				${TINYOBJ_TAG}
		GIT_PROGRESS		1
		# update
		PATCH_COMMAND		""
		UPDATE_DISCONNECTED	1
		# configure
		SOURCE_DIR			"${FG_EXTERNAL_TINYOBJ_PATH}"
		CMAKE_GENERATOR		"${CMAKE_GENERATOR}"
		CMAKE_GENERATOR_PLATFORM "${CMAKE_GENERATOR_PLATFORM}"
		CMAKE_GENERATOR_TOOLSET	"${CMAKE_GENERATOR_TOOLSET}"
		CMAKE_ARGS			"-DCMAKE_CONFIGURATION_TYPES=${FG_EXTERNAL_CONFIGURATION_TYPES}"
							"-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
							${FG_BUILD_TARGET_FLAGS}
		LOG_CONFIGURE 		1
		# build
		BINARY_DIR			"${CMAKE_BINARY_DIR}/build-tinyobj"
		BUILD_COMMAND		""
		LOG_BUILD 			1
		# install
		INSTALL_DIR 		""
		INSTALL_COMMAND		""
		LOG_INSTALL 		1
		# test
		TEST_COMMAND		""
	)
	
	set_property( TARGET "External.TinyObj" PROPERTY FOLDER "External" )

	add_library( "TinyObj-lib" INTERFACE )
	target_include_directories( "TinyObj-lib" INTERFACE "${FG_EXTERNAL_TINYOBJ_PATH}" )
	target_compile_definitions( "TinyObj-lib" INTERFACE "FG_ENABLE_TINYOBJ" )
	add_dependencies( "TinyObj-lib" "External.TinyObj" )
endif ()

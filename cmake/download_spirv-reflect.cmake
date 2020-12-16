# download and install SPIRV-Reflect

if (${FG_ENABLE_SPIRVREFLECT})
	set( FG_EXTERNAL_SPIRVREFLECT_PATH "" CACHE PATH "path to SPIRV-Reflect source" )
	mark_as_advanced( FG_EXTERNAL_SPIRVREFLECT_PATH )
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_SPIRVREFLECT_PATH}/spirv_reflect.h")
		message( STATUS "SPIRV-Reflect is not found in \"${FG_EXTERNAL_SPIRVREFLECT_PATH}\"" )
		set( FG_EXTERNAL_SPIRVREFLECT_PATH "${FG_EXTERNALS_PATH}/SPIRV-Reflect" CACHE PATH "" FORCE )
	else ()
		message( STATUS "SPIRV-Reflect found in \"${FG_EXTERNAL_SPIRVREFLECT_PATH}\"" )
	endif ()

	# download
	if (NOT EXISTS "${FG_EXTERNAL_SPIRVREFLECT_PATH}/spirv_reflect.c")
		FetchContent_Declare( ExternalSpirvReflect
			# download
			URL  				"https://github.com/KhronosGroup/SPIRV-Reflect/archive/3c77a11472a1da7830d055306b4299c5e2398e7c.zip"
			DOWNLOAD_DIR		"${FG_EXTERNAL_SPIRVREFLECT_PATH}"
			SOURCE_DIR			"${FG_EXTERNAL_SPIRVREFLECT_PATH}"
			LOG_DOWNLOAD		1
			# build
			BINARY_DIR			"${CMAKE_BINARY_DIR}/spirv_reflect"
			BUILD_COMMAND		""
			LOG_BUILD 			1
			# install
			INSTALL_DIR 		""
			LOG_INSTALL 		1
			# test
			TEST_COMMAND		""
		)
		
		FetchContent_GetProperties( ExternalSpirvReflect )
		if (NOT ExternalSpirvReflect_POPULATED)
			message( STATUS "downloading SPIRV-Reflect..." )
			FetchContent_Populate( ExternalSpirvReflect )
		endif ()
	endif ()

	add_library( "SPIRV-Reflect-lib" STATIC
		"${FG_EXTERNAL_SPIRVREFLECT_PATH}/spirv_reflect.c"
		"${FG_EXTERNAL_SPIRVREFLECT_PATH}/spirv_reflect.h" )

	target_include_directories( "SPIRV-Reflect-lib" PUBLIC "${FG_EXTERNAL_SPIRVREFLECT_PATH}" )
	target_compile_definitions( "SPIRV-Reflect-lib" PUBLIC FG_ENABLE_SPIRV_REFLECT )
	set_property( TARGET "SPIRV-Reflect-lib" PROPERTY FOLDER "External" )
endif ()

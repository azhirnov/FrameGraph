# download and install SPIRV-Reflect

if (${FG_ENABLE_SPIRVREFLECT})
	set( FG_EXTERNAL_SPIRVREFLECT_PATH "" CACHE PATH "path to SPIRV-Reflect source" )
	mark_as_advanced( FG_EXTERNAL_SPIRVREFLECT_PATH )
	
	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_SPIRVREFLECT_PATH}/spirv_reflect.h")
		message( STATUS "SPIRV-Reflect is not found in \"${FG_EXTERNAL_SPIRVREFLECT_PATH}\"" )
		set( FG_EXTERNAL_SPIRVREFLECT_PATH "${FG_EXTERNALS_PATH}/SPIRV-Reflect" CACHE PATH "" FORCE )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_SPIRVREFLECT_PATH}/spirv_reflect.h" AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
		FetchContent_Declare( ExternalSPIRVREFLECT
			GIT_REPOSITORY		https://github.com/chaoticbob/SPIRV-Reflect.git
			GIT_TAG				master
			SOURCE_DIR			"${FG_EXTERNAL_SPIRVREFLECT_PATH}"
		)
		
		FetchContent_GetProperties( ExternalSPIRVREFLECT )
		if (NOT ExternalSPIRVREFLECT_POPULATED)
			message( STATUS "downloading spirv-reflect" )
			FetchContent_Populate( ExternalSPIRVREFLECT )
		endif ()
	endif ()

	add_library( "SPIRV-Reflect-lib" STATIC
		"${FG_EXTERNAL_SPIRVREFLECT_PATH}/spirv_reflect.c"
		"${FG_EXTERNAL_SPIRVREFLECT_PATH}/spirv_reflect.h" )

	target_include_directories( "SPIRV-Reflect-lib" PUBLIC "${FG_EXTERNAL_SPIRVREFLECT_PATH}" )
	target_compile_definitions( "SPIRV-Reflect-lib" PUBLIC FG_ENABLE_SPIRV_REFLECT )
	set_property( TARGET "SPIRV-Reflect-lib" PROPERTY FOLDER "External" )
endif ()

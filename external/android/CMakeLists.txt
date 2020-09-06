
# std::filesystem implementation
add_subdirectory( "FileSystem" )


# Vulkan Memory Allocator
add_subdirectory( "VMA" )


# glslang
set( GLSLANG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../glslang" )
if (EXISTS ${GLSLANG_PATH})
	set( ENABLE_HLSL OFF CACHE INTERNAL "" )
	set( ENABLE_OPT OFF CACHE INTERNAL "" )
	set( ENABLE_SPVREMAPPER OFF CACHE INTERNAL "" FORCE )
	set( ENABLE_GLSLANG_BINARIES OFF CACHE INTERNAL "" FORCE )
	set( SKIP_GLSLANG_INSTALL OFF CACHE INTERNAL "" FORCE )
	set( SKIP_SPIRV_TOOLS_INSTALL OFF CACHE INTERNAL "" FORCE )
	set( SPIRV_SKIP_EXECUTABLES OFF CACHE INTERNAL "" FORCE )
	set( SPIRV_SKIP_TESTS ON CACHE INTERNAL "" FORCE )
	set( BUILD_TESTING OFF CACHE INTERNAL "" FORCE )
	add_subdirectory( ${GLSLANG_PATH} "glslang" )

	add_library( "GLSLang-lib" INTERFACE )
	target_include_directories( "GLSLang-lib" INTERFACE "${GLSLANG_PATH}" )
	target_compile_definitions( "GLSLang-lib" INTERFACE "FG_ENABLE_GLSLANG" )
	if (${ENABLE_OPT})
		target_compile_definitions( "GLSLang-lib" INTERFACE "FG_GLSLANG_ENABLE_OPT" )
	endif ()
	target_link_libraries( "GLSLang-lib" INTERFACE "glslang" "SPIRV" )
endif ()

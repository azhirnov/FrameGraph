# auto generated file
cmake_minimum_required (VERSION 3.6.0)

#==================================================================================================
# project: GLSLDebugger
#==================================================================================================
if (${FG_ENABLE_SAMPLES} AND ${FG_ENABLE_GLSLANG})
	set( SOURCES 
		"main.cpp"
		"ShaderCompiler.cpp"
		"ShaderCompiler.h" )
	if (DEFINED ANDROID)
		add_library( "GLSLDebugger" SHARED ${SOURCES} )
	else()
		add_executable( "GLSLDebugger" ${SOURCES} )
	endif()
	source_group( "" FILES "main.cpp" "ShaderCompiler.cpp" "ShaderCompiler.h" )
	set_property( TARGET "GLSLDebugger" PROPERTY FOLDER "Samples" )
	target_include_directories( "GLSLDebugger" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "GLSLDebugger" PRIVATE "../../extensions" )
	target_include_directories( "GLSLDebugger" PRIVATE "${FG_GLSLANG_INCLUDE_DIR}" )
	target_link_libraries( "GLSLDebugger" "${FG_GLSLANG_LIBRARIES}" )
	target_link_libraries( "GLSLDebugger" "Framework" )
	target_compile_definitions( "GLSLDebugger" PUBLIC "${FG_GLOBAL_DEFINITIONS}" )
	add_dependencies( "GLSLDebugger" External.glslang-main )
	# compiler
	target_compile_definitions( "GLSLDebugger" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "GLSLDebugger" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "GLSLDebugger" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "GLSLDebugger" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "GLSLDebugger" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "GLSLDebugger" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "GLSLDebugger" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "GLSLDebugger" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "GLSLDebugger" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()



# auto generated file
cmake_minimum_required (VERSION 3.6.0)

#==================================================================================================
# project: Samples.Vulkan
#==================================================================================================
if (${FG_ENABLE_SAMPLES} AND ${FG_ENABLE_GLSLANG})
	set( SOURCES 
		"compiler/SpvCompiler.cpp"
		"compiler/SpvCompiler.h"
		"AsyncCompute1.cpp"
		"CMakeLists.txt"
		"FragShaderBarycentric1.cpp"
		"ImageFootprint1.cpp"
		"main.cpp"
		"MeshShader1.cpp"
		"RayTracing1.cpp"
		"RayTracing2.cpp"
		"ShadingRate1.cpp"
		"SparseImage1.cpp" )
	if (DEFINED ANDROID)
		add_library( "Samples.Vulkan" SHARED ${SOURCES} )
	else()
		add_executable( "Samples.Vulkan" ${SOURCES} )
	endif()
	source_group( "compiler" FILES "compiler/SpvCompiler.cpp" "compiler/SpvCompiler.h" )
	source_group( "" FILES "AsyncCompute1.cpp" "CMakeLists.txt" "FragShaderBarycentric1.cpp" "ImageFootprint1.cpp" "main.cpp" "MeshShader1.cpp" "RayTracing1.cpp" "RayTracing2.cpp" "ShadingRate1.cpp" "SparseImage1.cpp" )
	set_property( TARGET "Samples.Vulkan" PROPERTY FOLDER "Samples" )
	target_include_directories( "Samples.Vulkan" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "Samples.Vulkan" PRIVATE "../../extensions" )
	target_link_libraries( "Samples.Vulkan" "OSDependent" )
	target_link_libraries( "Samples.Vulkan" "HLSL" )
	target_link_libraries( "Samples.Vulkan" "SPIRV" )
	target_link_libraries( "Samples.Vulkan" "Framework" )
	target_link_libraries( "Samples.Vulkan" "glslang" )
	target_link_libraries( "Samples.Vulkan" "OGLCompiler" )
	target_compile_definitions( "Samples.Vulkan" PUBLIC "${FG_GLOBAL_DEFINITIONS}" )
	# compiler
	target_compile_definitions( "Samples.Vulkan" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Samples.Vulkan" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Samples.Vulkan" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Samples.Vulkan" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Samples.Vulkan" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Samples.Vulkan" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Samples.Vulkan" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Samples.Vulkan" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Samples.Vulkan" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()



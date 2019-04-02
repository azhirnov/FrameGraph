# auto generated file
cmake_minimum_required (VERSION 3.6.0)

#==================================================================================================
# project: Samples.Shadertoy
#==================================================================================================
if (${FG_ENABLE_SAMPLES} AND ${FG_ENABLE_GLM})
	set( SOURCES 
		"shadertoy/st_shaders/FinalCS-Barboy-Opt1.glsl"
		"shadertoy/st_shaders/FinalCS-Barboy-Opt2.glsl"
		"shadertoy/st_shaders/FinalCS-Barboy.glsl"
		"shadertoy/st_shaders/Glowballs.glsl"
		"shadertoy/st_shaders/OceanPlanet.glsl"
		"shadertoy/st_shaders/Rainforest_1.glsl"
		"shadertoy/st_shaders/Rainforest_2.glsl"
		"shadertoy/st_shaders/SirenianDawn.glsl"
		"shadertoy/st_shaders/Skyline.glsl"
		"shadertoy/st_shaders/Skyline2.glsl"
		"shadertoy/st_shaders/Skyline3-1.glsl"
		"shadertoy/st_shaders/Skyline3-2.glsl"
		"shadertoy/st_shaders/SkylineOpt1.glsl"
		"shadertoy/st_shaders/SkylineOpt2.glsl"
		"shadertoy/FGShadertoyApp.cpp"
		"shadertoy/FGShadertoyApp.h" )
	if (DEFINED ANDROID)
		add_library( "Samples.Shadertoy" SHARED ${SOURCES} )
	else()
		add_executable( "Samples.Shadertoy" ${SOURCES} )
	endif()
	source_group( "st_shaders" FILES "shadertoy/st_shaders/FinalCS-Barboy-Opt1.glsl" "shadertoy/st_shaders/FinalCS-Barboy-Opt2.glsl" "shadertoy/st_shaders/FinalCS-Barboy.glsl" "shadertoy/st_shaders/Glowballs.glsl" "shadertoy/st_shaders/OceanPlanet.glsl" "shadertoy/st_shaders/Rainforest_1.glsl" "shadertoy/st_shaders/Rainforest_2.glsl" "shadertoy/st_shaders/SirenianDawn.glsl" "shadertoy/st_shaders/Skyline.glsl" "shadertoy/st_shaders/Skyline2.glsl" "shadertoy/st_shaders/Skyline3-1.glsl" "shadertoy/st_shaders/Skyline3-2.glsl" "shadertoy/st_shaders/SkylineOpt1.glsl" "shadertoy/st_shaders/SkylineOpt2.glsl" )
	source_group( "" FILES "shadertoy/FGShadertoyApp.cpp" "shadertoy/FGShadertoyApp.h" )
	set_property( TARGET "Samples.Shadertoy" PROPERTY FOLDER "Samples" )
	target_include_directories( "Samples.Shadertoy" PUBLIC "${FG_EXTERNALS_PATH}" )
	if (${FG_ENABLE_GRAPHVIZ})
		target_link_libraries( "Samples.Shadertoy" "GraphViz" )
	endif()
	if (${FG_ENABLE_GLM})
		target_link_libraries( "Samples.Shadertoy" "Scene" )
	endif()
	if (${FG_ENABLE_GLSLANG})
		target_link_libraries( "Samples.Shadertoy" "PipelineCompiler" )
	endif()
	target_compile_definitions( "Samples.Shadertoy" PUBLIC "FG_DATA_PATH=R\"(${CMAKE_CURRENT_SOURCE_DIR}/shadertoy/)\"" )
	# compiler
	target_compile_definitions( "Samples.Shadertoy" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Samples.Shadertoy" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Samples.Shadertoy" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Samples.Shadertoy" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Samples.Shadertoy" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Samples.Shadertoy" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Samples.Shadertoy" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Samples.Shadertoy" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Samples.Shadertoy" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


#==================================================================================================
# project: Samples.RayTracing
#==================================================================================================
if (${FG_ENABLE_SAMPLES})
	set( SOURCES 
		"ray_tracing/SceneApp.cpp"
		"ray_tracing/SceneApp.h" )
	if (DEFINED ANDROID)
		add_library( "Samples.RayTracing" SHARED ${SOURCES} )
	else()
		add_executable( "Samples.RayTracing" ${SOURCES} )
	endif()
	source_group( "" FILES "ray_tracing/SceneApp.cpp" "ray_tracing/SceneApp.h" )
	set_property( TARGET "Samples.RayTracing" PROPERTY FOLDER "Samples" )
	target_include_directories( "Samples.RayTracing" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "Samples.RayTracing" PRIVATE "../../extensions" )
	if (${FG_ENABLE_GLM})
		target_link_libraries( "Samples.RayTracing" "Scene" )
	endif()
	if (${FG_ENABLE_GLSLANG})
		target_link_libraries( "Samples.RayTracing" "PipelineCompiler" )
	endif()
	target_compile_definitions( "Samples.RayTracing" PUBLIC "FG_DATA_PATH=R\"(${CMAKE_CURRENT_SOURCE_DIR}/ray_tracing/)\"" )
	# compiler
	target_compile_definitions( "Samples.RayTracing" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Samples.RayTracing" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Samples.RayTracing" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Samples.RayTracing" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Samples.RayTracing" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Samples.RayTracing" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Samples.RayTracing" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Samples.RayTracing" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Samples.RayTracing" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()



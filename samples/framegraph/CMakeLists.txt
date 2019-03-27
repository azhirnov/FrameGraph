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
# project: Samples.PBR
#==================================================================================================
if (${FG_ENABLE_SAMPLES} AND ${FG_ENABLE_GLM} AND ${FG_ENABLE_DEVIL})
	set( SOURCES 
		"pbr/SceneApp.cpp"
		"pbr/SceneApp.h" )
	if (DEFINED ANDROID)
		add_library( "Samples.PBR" SHARED ${SOURCES} )
	else()
		add_executable( "Samples.PBR" ${SOURCES} )
	endif()
	source_group( "" FILES "pbr/SceneApp.cpp" "pbr/SceneApp.h" )
	set_property( TARGET "Samples.PBR" PROPERTY FOLDER "Samples" )
	target_include_directories( "Samples.PBR" PUBLIC "${FG_EXTERNALS_PATH}" )
	if (${FG_ENABLE_GRAPHVIZ})
		target_link_libraries( "Samples.PBR" "GraphViz" )
	endif()
	if (${FG_ENABLE_GLM})
		target_link_libraries( "Samples.PBR" "Scene" )
	endif()
	if (${FG_ENABLE_GLSLANG})
		target_link_libraries( "Samples.PBR" "PipelineCompiler" )
	endif()
	target_compile_definitions( "Samples.PBR" PUBLIC "FG_DATA_PATH=R\"(${CMAKE_CURRENT_SOURCE_DIR}/pbr/)\"" )
	# compiler
	target_compile_definitions( "Samples.PBR" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Samples.PBR" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Samples.PBR" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Samples.PBR" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Samples.PBR" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Samples.PBR" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Samples.PBR" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Samples.PBR" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Samples.PBR" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


#==================================================================================================
# project: Samples.GI2D
#==================================================================================================
if (${FG_ENABLE_SAMPLES} AND ${FG_ENABLE_IMGUI})
	set( SOURCES 
		"gi2d/rt/RTRenderer.cpp"
		"gi2d/rt/RTRenderer.h"
		"gi2d/rt/updategi_hit.glsl"
		"gi2d/rt/updategi_miss.glsl"
		"gi2d/rt/updategi_raygen.glsl"
		"gi2d/shaders/common.glsl"
		"gi2d/shaders/lightaccum.glsl"
		"gi2d/shaders/objects.glsl"
		"gi2d/sdf/accum_light.glsl"
		"gi2d/sdf/build_surf.glsl"
		"gi2d/sdf/color_view.glsl"
		"gi2d/sdf/light_view.glsl"
		"gi2d/sdf/SDFRenderer.cpp"
		"gi2d/sdf/SDFRenderer.h"
		"gi2d/sdf/sdf_image.glsl"
		"gi2d/sdf/sdf_view.glsl"
		"gi2d/sdf/surf_view.glsl"
		"gi2d/sdf/update_gi.glsl"
		"gi2d/sdf/update_light_view.glsl"
		"gi2d/sdf/update_sdf.glsl"
		"gi2d/GIApp.cpp"
		"gi2d/GIApp.h"
		"gi2d/IRenderer.h"
		"gi2d/Scene.h" )
	if (DEFINED ANDROID)
		add_library( "Samples.GI2D" SHARED ${SOURCES} )
	else()
		add_executable( "Samples.GI2D" ${SOURCES} )
	endif()
	source_group( "rt" FILES "gi2d/rt/RTRenderer.cpp" "gi2d/rt/RTRenderer.h" "gi2d/rt/updategi_hit.glsl" "gi2d/rt/updategi_miss.glsl" "gi2d/rt/updategi_raygen.glsl" )
	source_group( "shaders" FILES "gi2d/shaders/common.glsl" "gi2d/shaders/lightaccum.glsl" "gi2d/shaders/objects.glsl" )
	source_group( "sdf" FILES "gi2d/sdf/accum_light.glsl" "gi2d/sdf/build_surf.glsl" "gi2d/sdf/color_view.glsl" "gi2d/sdf/light_view.glsl" "gi2d/sdf/SDFRenderer.cpp" "gi2d/sdf/SDFRenderer.h" "gi2d/sdf/sdf_image.glsl" "gi2d/sdf/sdf_view.glsl" "gi2d/sdf/surf_view.glsl" "gi2d/sdf/update_gi.glsl" "gi2d/sdf/update_light_view.glsl" "gi2d/sdf/update_sdf.glsl" )
	source_group( "" FILES "gi2d/GIApp.cpp" "gi2d/GIApp.h" "gi2d/IRenderer.h" "gi2d/Scene.h" )
	set_property( TARGET "Samples.GI2D" PROPERTY FOLDER "Samples" )
	target_include_directories( "Samples.GI2D" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "Samples.GI2D" PRIVATE "../../extensions" )
	if (${FG_ENABLE_GRAPHVIZ})
		target_link_libraries( "Samples.GI2D" "GraphViz" )
	endif()
	if (${FG_ENABLE_IMGUI})
		target_link_libraries( "Samples.GI2D" "UI" )
	endif()
	if (${FG_ENABLE_GLSLANG})
		target_link_libraries( "Samples.GI2D" "PipelineCompiler" )
	endif()
	target_compile_definitions( "Samples.GI2D" PUBLIC "FG_DATA_PATH=R\"(${CMAKE_CURRENT_SOURCE_DIR}/gi2d/)\"" )
	# compiler
	target_compile_definitions( "Samples.GI2D" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Samples.GI2D" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Samples.GI2D" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Samples.GI2D" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Samples.GI2D" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Samples.GI2D" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Samples.GI2D" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Samples.GI2D" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Samples.GI2D" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
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



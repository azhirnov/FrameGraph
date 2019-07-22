# auto generated file
cmake_minimum_required (VERSION 3.6.0)

#==================================================================================================
# project: Samples.Vulkan
#==================================================================================================
if (${FG_ENABLE_SAMPLES} AND ${FG_ENABLE_GLSLANG})
	set( SOURCES 
		"vulkan/compiler/SpvCompiler.cpp"
		"vulkan/compiler/SpvCompiler.h"
		"vulkan/AsyncCompute1.cpp"
		"vulkan/AsyncCompute2.cpp"
		"vulkan/AsyncCompute3.cpp"
		"vulkan/CacheTest.cpp"
		"vulkan/ClearImage.cpp"
		"vulkan/FragShaderBarycentric1.cpp"
		"vulkan/GenMipmaps.cpp"
		"vulkan/ImageFootprint1.cpp"
		"vulkan/main.cpp"
		"vulkan/MeshShader1.cpp"
		"vulkan/RayTracing1.cpp"
		"vulkan/RayTracing2.cpp"
		"vulkan/ShadingRate1.cpp"
		"vulkan/SparseImage1.cpp"
		"vulkan/ZOrderPattern.cpp" )
	if (DEFINED ANDROID)
		add_library( "Samples.Vulkan" SHARED ${SOURCES} )
	else()
		add_executable( "Samples.Vulkan" ${SOURCES} )
	endif()
	source_group( "compiler" FILES "vulkan/compiler/SpvCompiler.cpp" "vulkan/compiler/SpvCompiler.h" )
	source_group( "" FILES "vulkan/AsyncCompute1.cpp" "vulkan/AsyncCompute2.cpp" "vulkan/AsyncCompute3.cpp" "vulkan/CacheTest.cpp" "vulkan/ClearImage.cpp" "vulkan/FragShaderBarycentric1.cpp" "vulkan/GenMipmaps.cpp" "vulkan/ImageFootprint1.cpp" "vulkan/main.cpp" "vulkan/MeshShader1.cpp" "vulkan/RayTracing1.cpp" "vulkan/RayTracing2.cpp" "vulkan/ShadingRate1.cpp" "vulkan/SparseImage1.cpp" "vulkan/ZOrderPattern.cpp" )
	set_property( TARGET "Samples.Vulkan" PROPERTY FOLDER "Samples" )
	target_include_directories( "Samples.Vulkan" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "Samples.Vulkan" PRIVATE "${FG_GLSLANG_INCLUDE_DIR}" )
	target_include_directories( "Samples.Vulkan" PRIVATE "../extensions" )
	target_link_libraries( "Samples.Vulkan" "${FG_GLSLANG_LIBRARIES}" )
	target_link_libraries( "Samples.Vulkan" "Framework" )
	target_compile_definitions( "Samples.Vulkan" PUBLIC "${FG_GLOBAL_DEFINITIONS}" )
	add_dependencies( "Samples.Vulkan" External.glslang-main )
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


#==================================================================================================
# project: Samples.Shadertoy
#==================================================================================================
if (${FG_ENABLE_SAMPLES} AND ${FG_ENABLE_GLM})
	set( SOURCES 
		"shadertoy/my_shaders/Building_1.glsl"
		"shadertoy/my_shaders/ConvexShape2D.glsl"
		"shadertoy/my_shaders/ConvexShape3D.glsl"
		"shadertoy/my_shaders/OptSDF_1.glsl"
		"shadertoy/my_shaders/OptSDF_2.glsl"
		"shadertoy/my_shaders/OptSDF_3.glsl"
		"shaderlib/Color.glsl"
		"shaderlib/CubeMap.glsl"
		"shaderlib/Geometry.glsl"
		"shaderlib/GlobalIndex.glsl"
		"shaderlib/Hash.glsl"
		"shaderlib/Lighting.glsl"
		"shaderlib/Math.glsl"
		"shaderlib/Matrix.glsl"
		"shaderlib/Perlin.glsl"
		"shaderlib/Quaternion.glsl"
		"shaderlib/RayTracing.glsl"
		"shaderlib/SDF.glsl"
		"shaderlib/Voronoi.glsl"
		"shaderlib/Waves.glsl"
		"shadertoy/st_shaders/Glowballs.glsl"
		"shadertoy/st_shaders/Skyline.glsl"
		"shadertoy/st_shaders/Skyline2.glsl"
		"shadertoy/FGShadertoyApp.cpp"
		"shadertoy/FGShadertoyApp.h" )
	if (DEFINED ANDROID)
		add_library( "Samples.Shadertoy" SHARED ${SOURCES} )
	else()
		add_executable( "Samples.Shadertoy" ${SOURCES} )
	endif()
	source_group( "my_shaders" FILES "shadertoy/my_shaders/Building_1.glsl" "shadertoy/my_shaders/ConvexShape2D.glsl" "shadertoy/my_shaders/ConvexShape3D.glsl" "shadertoy/my_shaders/OptSDF_1.glsl" "shadertoy/my_shaders/OptSDF_2.glsl" "shadertoy/my_shaders/OptSDF_3.glsl" )
	source_group( "shaderlib" FILES "shaderlib/Color.glsl" "shaderlib/CubeMap.glsl" "shaderlib/Geometry.glsl" "shaderlib/GlobalIndex.glsl" "shaderlib/Hash.glsl" "shaderlib/Lighting.glsl" "shaderlib/Math.glsl" "shaderlib/Matrix.glsl" "shaderlib/Perlin.glsl" "shaderlib/Quaternion.glsl" "shaderlib/RayTracing.glsl" "shaderlib/SDF.glsl" "shaderlib/Voronoi.glsl" "shaderlib/Waves.glsl" )
	source_group( "st_shaders" FILES "shadertoy/st_shaders/Glowballs.glsl" "shadertoy/st_shaders/Skyline.glsl" "shadertoy/st_shaders/Skyline2.glsl" )
	source_group( "" FILES "shadertoy/FGShadertoyApp.cpp" "shadertoy/FGShadertoyApp.h" )
	set_property( TARGET "Samples.Shadertoy" PROPERTY FOLDER "Samples" )
	target_include_directories( "Samples.Shadertoy" PUBLIC "${FG_EXTERNALS_PATH}" )
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
if (${FG_ENABLE_SAMPLES} AND ${FG_ENABLE_ASSIMP} AND ${FG_ENABLE_GLM} AND ${FG_ENABLE_DEVIL})
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
	target_include_directories( "Samples.RayTracing" PRIVATE "../extensions" )
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




#==================================================================================================
# project: Samples.Planet
#==================================================================================================
if (${FG_ENABLE_SAMPLES} AND ${FG_ENABLE_GLM} AND ${FG_ENABLE_DEVIL})
	set( SOURCES 
		"planet/shaders/gen_height.glsl"
		"planet/shaders/planet.glsl"
		"planet/GenPlanetApp.cpp"
		"planet/GenPlanetApp.h"
		"planet/SphericalCube.cpp"
		"planet/SphericalCube.h"
		"planet/SphericalCubeMath.h"
		"planet/UnitTest_SphericalCubeMath.cpp" )
	if (DEFINED ANDROID)
		add_library( "Samples.Planet" SHARED ${SOURCES} )
	else()
		add_executable( "Samples.Planet" ${SOURCES} )
	endif()
	source_group( "shaders" FILES "planet/shaders/gen_height.glsl" "planet/shaders/planet.glsl" )
	source_group( "" FILES "planet/GenPlanetApp.cpp" "planet/GenPlanetApp.h" "planet/SphericalCube.cpp" "planet/SphericalCube.h" "planet/SphericalCubeMath.h" "planet/UnitTest_SphericalCubeMath.cpp" )
	set_property( TARGET "Samples.Planet" PROPERTY FOLDER "Samples" )
	target_include_directories( "Samples.Planet" PUBLIC "${FG_EXTERNALS_PATH}" )
	if (${FG_ENABLE_GLM})
		target_link_libraries( "Samples.Planet" "Scene" )
	endif()
	if (${FG_ENABLE_GLSLANG})
		target_link_libraries( "Samples.Planet" "PipelineCompiler" )
	endif()
	target_compile_definitions( "Samples.Planet" PUBLIC "FG_DATA_PATH=R\"(${CMAKE_CURRENT_SOURCE_DIR}/planet/)\"" )
	# compiler
	target_compile_definitions( "Samples.Planet" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Samples.Planet" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Samples.Planet" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Samples.Planet" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Samples.Planet" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Samples.Planet" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Samples.Planet" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Samples.Planet" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Samples.Planet" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()

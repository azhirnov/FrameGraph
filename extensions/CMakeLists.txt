# auto generated file
cmake_minimum_required (VERSION 3.6.0)

#==================================================================================================
# project: VulkanLoader
#==================================================================================================
set( SOURCES 
	"vulkan_loader/fn_vulkan_dev.h"
	"vulkan_loader/fn_vulkan_inst.h"
	"vulkan_loader/fn_vulkan_lib.h"
	"vulkan_loader/VulkanCheckError.cpp"
	"vulkan_loader/VulkanCheckError.h"
	"vulkan_loader/VulkanLoader.cpp"
	"vulkan_loader/VulkanLoader.h" )
add_library( "VulkanLoader" STATIC ${SOURCES} )
source_group( "" FILES "vulkan_loader/fn_vulkan_dev.h" "vulkan_loader/fn_vulkan_inst.h" "vulkan_loader/fn_vulkan_lib.h" "vulkan_loader/VulkanCheckError.cpp" "vulkan_loader/VulkanCheckError.h" "vulkan_loader/VulkanLoader.cpp" "vulkan_loader/VulkanLoader.h" )
set_property( TARGET "VulkanLoader" PROPERTY FOLDER "Extensions" )
target_include_directories( "VulkanLoader" PUBLIC "${Vulkan_INCLUDE_DIRS}" )
target_include_directories( "VulkanLoader" PUBLIC "${FG_EXTERNALS_PATH}" )
target_include_directories( "VulkanLoader" PRIVATE ".." )
target_link_libraries( "VulkanLoader" "STL" )
# compiler
target_compile_definitions( "VulkanLoader" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
set_target_properties( "VulkanLoader" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
target_compile_definitions( "VulkanLoader" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
target_compile_options( "VulkanLoader" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
set_target_properties( "VulkanLoader" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
target_compile_options( "VulkanLoader" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
set_target_properties( "VulkanLoader" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
target_compile_options( "VulkanLoader" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
target_compile_definitions( "VulkanLoader" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )

install( TARGETS "VulkanLoader" ARCHIVE DESTINATION "lib" )


#==================================================================================================
# project: PipelineCompiler
#==================================================================================================
if (${FG_ENABLE_GLSLANG})
	set( SOURCES 
		"pipeline_compiler/EShaderCompilationFlags.h"
		"pipeline_compiler/PipelineCppSerializer.cpp"
		"pipeline_compiler/PipelineCppSerializer.h"
		"pipeline_compiler/PrivateDefines.h"
		"pipeline_compiler/SpirvCompiler.cpp"
		"pipeline_compiler/SpirvCompiler.h"
		"pipeline_compiler/SpirvCompiler_Resources.cpp"
		"pipeline_compiler/VPipelineCompiler.cpp"
		"pipeline_compiler/VPipelineCompiler.h" )
	add_library( "PipelineCompiler" STATIC ${SOURCES} )
	source_group( "" FILES "pipeline_compiler/EShaderCompilationFlags.h" "pipeline_compiler/PipelineCppSerializer.cpp" "pipeline_compiler/PipelineCppSerializer.h" "pipeline_compiler/PrivateDefines.h" "pipeline_compiler/SpirvCompiler.cpp" "pipeline_compiler/SpirvCompiler.h" "pipeline_compiler/SpirvCompiler_Resources.cpp" "pipeline_compiler/VPipelineCompiler.cpp" "pipeline_compiler/VPipelineCompiler.h" )
	set_property( TARGET "PipelineCompiler" PROPERTY FOLDER "Extensions" )
	target_include_directories( "PipelineCompiler" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "PipelineCompiler" PRIVATE ".." )
	target_include_directories( "PipelineCompiler" PRIVATE "pipeline_compiler" )
	target_include_directories( "PipelineCompiler" PRIVATE "${FG_GLSLANG_INCLUDE_DIR}" )
	if (UNIX)
		target_link_libraries( "PipelineCompiler" "pthread" )
	endif()
	target_link_libraries( "PipelineCompiler" "${FG_GLSLANG_LIBRARY_DIR}/OGLCompiler${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	target_link_libraries( "PipelineCompiler" "FrameGraph" )
	target_link_libraries( "PipelineCompiler" "${FG_GLSLANG_LIBRARY_DIR}/glslang${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	target_link_libraries( "PipelineCompiler" "${FG_GLSLANG_LIBRARY_DIR}/SPIRV-Tools-opt${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	target_link_libraries( "PipelineCompiler" "${FG_GLSLANG_LIBRARY_DIR}/OSDependent${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	target_link_libraries( "PipelineCompiler" "${FG_GLSLANG_LIBRARY_DIR}/SPIRV${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	target_link_libraries( "PipelineCompiler" "${FG_GLSLANG_LIBRARY_DIR}/HLSL${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	target_link_libraries( "PipelineCompiler" "${FG_GLSLANG_LIBRARY_DIR}/SPVRemapper${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	target_link_libraries( "PipelineCompiler" "${FG_GLSLANG_LIBRARY_DIR}/SPIRV-Tools${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	target_compile_definitions( "PipelineCompiler" PUBLIC "${FG_GLOBAL_DEFINITIONS}" )
	add_dependencies( "PipelineCompiler" External.glslang-main )
	# compiler
	target_compile_definitions( "PipelineCompiler" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "PipelineCompiler" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "PipelineCompiler" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "PipelineCompiler" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "PipelineCompiler" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "PipelineCompiler" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "PipelineCompiler" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "PipelineCompiler" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "PipelineCompiler" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
	install( TARGETS "PipelineCompiler" ARCHIVE DESTINATION "lib" )
endif()


#==================================================================================================
# project: UnitTests.PipelineCompiler
#==================================================================================================
if (${FG_ENABLE_GLSLANG} AND ${FG_ENABLE_TESTS} AND NOT UNIX)
	set( SOURCES 
		"../tests/pipeline_compiler/main.cpp"
		"../tests/pipeline_compiler/Test_Serializer1.cpp"
		"../tests/pipeline_compiler/Test_Serializer2.cpp"
		"../tests/pipeline_compiler/Test_Shader1.cpp"
		"../tests/pipeline_compiler/Test_Shader2.cpp"
		"../tests/pipeline_compiler/Test_Shader3.cpp"
		"../tests/pipeline_compiler/Test_Shader4.cpp"
		"../tests/pipeline_compiler/Test_Shader5.cpp"
		"../tests/pipeline_compiler/Test_Shader6.cpp"
		"../tests/pipeline_compiler/Test_Shader7.cpp"
		"../tests/pipeline_compiler/Test_Shader8.cpp"
		"../tests/pipeline_compiler/Test_Shader9.cpp"
		"../tests/pipeline_compiler/Utils.h" )
	if (DEFINED ANDROID)
		add_library( "UnitTests.PipelineCompiler" SHARED ${SOURCES} )
	else()
		add_executable( "UnitTests.PipelineCompiler" ${SOURCES} )
	endif()
	source_group( "" FILES "../tests/pipeline_compiler/main.cpp" "../tests/pipeline_compiler/Test_Serializer1.cpp" "../tests/pipeline_compiler/Test_Serializer2.cpp" "../tests/pipeline_compiler/Test_Shader1.cpp" "../tests/pipeline_compiler/Test_Shader2.cpp" "../tests/pipeline_compiler/Test_Shader3.cpp" "../tests/pipeline_compiler/Test_Shader4.cpp" "../tests/pipeline_compiler/Test_Shader5.cpp" "../tests/pipeline_compiler/Test_Shader6.cpp" "../tests/pipeline_compiler/Test_Shader7.cpp" "../tests/pipeline_compiler/Test_Shader8.cpp" "../tests/pipeline_compiler/Test_Shader9.cpp" "../tests/pipeline_compiler/Utils.h" )
	set_property( TARGET "UnitTests.PipelineCompiler" PROPERTY FOLDER "Tests" )
	target_include_directories( "UnitTests.PipelineCompiler" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "UnitTests.PipelineCompiler" PRIVATE "../tests/.." )
	target_include_directories( "UnitTests.PipelineCompiler" PRIVATE "" )
	if (${FG_ENABLE_GLSLANG})
		target_link_libraries( "UnitTests.PipelineCompiler" "PipelineCompiler" )
	endif()
	# compiler
	target_compile_definitions( "UnitTests.PipelineCompiler" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "UnitTests.PipelineCompiler" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "UnitTests.PipelineCompiler" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "UnitTests.PipelineCompiler" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "UnitTests.PipelineCompiler" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "UnitTests.PipelineCompiler" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "UnitTests.PipelineCompiler" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "UnitTests.PipelineCompiler" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "UnitTests.PipelineCompiler" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


#==================================================================================================
# project: OfflineCompiler
#==================================================================================================
if (${FG_ENABLE_GLSLANG})
	set( SOURCES 
		"offline_compiler/main.cpp" )
	if (DEFINED ANDROID)
		add_library( "OfflineCompiler" SHARED ${SOURCES} )
	else()
		add_executable( "OfflineCompiler" ${SOURCES} )
	endif()
	source_group( "" FILES "offline_compiler/main.cpp" )
	set_property( TARGET "OfflineCompiler" PROPERTY FOLDER "Extensions" )
	target_include_directories( "OfflineCompiler" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "OfflineCompiler" PRIVATE "offline_compiler" )
	target_include_directories( "OfflineCompiler" PRIVATE ".." )
	target_link_libraries( "OfflineCompiler" "PipelineCompiler" )
	# compiler
	target_compile_definitions( "OfflineCompiler" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "OfflineCompiler" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "OfflineCompiler" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "OfflineCompiler" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "OfflineCompiler" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "OfflineCompiler" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "OfflineCompiler" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "OfflineCompiler" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "OfflineCompiler" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
	#-----------------------------------
	if ( CONFIGURATION_DEPENDENT_PATH )
		set( OFFLINE_COMPILER_EXE "${MAIN_BINARY_DIR}/$<CONFIG>/OfflineCompiler${CMAKE_EXECUTABLE_SUFFIX}" CACHE INTERNAL "" FORCE )
	else ()
		set( OFFLINE_COMPILER_EXE "${MAIN_BINARY_DIR}/OfflineCompiler${CMAKE_EXECUTABLE_SUFFIX}" CACHE INTERNAL "" FORCE )
	endif ()
	
	#-----------------------------------
endif()


#==================================================================================================
# project: Framework
#==================================================================================================
set( SOURCES 
	"framework/Window/IWindow.h"
	"framework/Window/WindowGLFW.cpp"
	"framework/Window/WindowGLFW.h"
	"framework/Window/WindowSDL2.cpp"
	"framework/Window/WindowSDL2.h"
	"framework/Window/WindowSFML.cpp"
	"framework/Window/WindowSFML.h"
	"framework/Vulkan/VulkanDevice.cpp"
	"framework/Vulkan/VulkanDevice.h"
	"framework/Vulkan/VulkanDeviceExt.cpp"
	"framework/Vulkan/VulkanDeviceExt.h"
	"framework/Vulkan/VulkanSurface.cpp"
	"framework/Vulkan/VulkanSurface.h"
	"framework/Vulkan/VulkanSwapchain.cpp"
	"framework/Vulkan/VulkanSwapchain.h" )
add_library( "Framework" STATIC ${SOURCES} )
source_group( "Window" FILES "framework/Window/IWindow.h" "framework/Window/WindowGLFW.cpp" "framework/Window/WindowGLFW.h" "framework/Window/WindowSDL2.cpp" "framework/Window/WindowSDL2.h" "framework/Window/WindowSFML.cpp" "framework/Window/WindowSFML.h" )
source_group( "Vulkan" FILES "framework/Vulkan/VulkanDevice.cpp" "framework/Vulkan/VulkanDevice.h" "framework/Vulkan/VulkanDeviceExt.cpp" "framework/Vulkan/VulkanDeviceExt.h" "framework/Vulkan/VulkanSurface.cpp" "framework/Vulkan/VulkanSurface.h" "framework/Vulkan/VulkanSwapchain.cpp" "framework/Vulkan/VulkanSwapchain.h" )
set_property( TARGET "Framework" PROPERTY FOLDER "Extensions" )
target_include_directories( "Framework" PUBLIC "${FG_EXTERNALS_PATH}" )
target_include_directories( "Framework" PRIVATE ".." )
target_include_directories( "Framework" PRIVATE "framework/.." )
target_include_directories( "Framework" PRIVATE "framework" )
if (${FG_ENABLE_GLFW})
	target_link_libraries( "Framework" "glfw" )
endif()
if (${FG_ENABLE_SFML})
	target_link_libraries( "Framework" "sfml-window" )
endif()
if (${FG_ENABLE_SDL2})
	target_link_libraries( "Framework" "SDL2-static" )
endif()
target_link_libraries( "Framework" "VulkanLoader" )
target_link_libraries( "Framework" "STL" )
target_compile_definitions( "Framework" PUBLIC "${FG_GLOBAL_DEFINITIONS}" )
# compiler
target_compile_definitions( "Framework" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
set_target_properties( "Framework" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
target_compile_definitions( "Framework" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
target_compile_options( "Framework" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
set_target_properties( "Framework" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
target_compile_options( "Framework" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
set_target_properties( "Framework" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
target_compile_options( "Framework" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
target_compile_definitions( "Framework" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )

install( TARGETS "Framework" ARCHIVE DESTINATION "lib" )


#==================================================================================================
# project: Tests.Framework
#==================================================================================================
if (${FG_ENABLE_TESTS})
	set( SOURCES 
		"../tests/framework/FW_Test1.cpp"
		"../tests/framework/FW_Test2.cpp"
		"../tests/framework/main.cpp" )
	if (DEFINED ANDROID)
		add_library( "Tests.Framework" SHARED ${SOURCES} )
	else()
		add_executable( "Tests.Framework" ${SOURCES} )
	endif()
	source_group( "" FILES "../tests/framework/FW_Test1.cpp" "../tests/framework/FW_Test2.cpp" "../tests/framework/main.cpp" )
	set_property( TARGET "Tests.Framework" PROPERTY FOLDER "Tests" )
	target_include_directories( "Tests.Framework" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "Tests.Framework" PRIVATE "" )
	target_link_libraries( "Tests.Framework" "Framework" )
	# compiler
	target_compile_definitions( "Tests.Framework" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Tests.Framework" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Tests.Framework" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Tests.Framework" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Tests.Framework" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Tests.Framework" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Tests.Framework" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Tests.Framework" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Tests.Framework" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


add_test( NAME "UnitTests.PipelineCompiler" COMMAND "UnitTests.PipelineCompiler" )


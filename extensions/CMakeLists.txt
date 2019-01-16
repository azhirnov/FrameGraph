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


#==================================================================================================
# project: GLSLTrace
#==================================================================================================
if (${FG_ENABLE_GLSLANG})
	set( SOURCES 
		"glsl_trace/include/ShaderTrace.h"
		"glsl_trace/source/Common.h"
		"glsl_trace/source/ConvertTraceToRdc.cpp"
		"glsl_trace/source/InsertDebugAsserts.cpp"
		"glsl_trace/source/InsertInstructionCounter.cpp"
		"glsl_trace/source/InsertTraceRecording.cpp"
		"glsl_trace/source/ParseShaderTrace.cpp"
		"glsl_trace/source/ShaderDebugUtils.cpp" )
	add_library( "GLSLTrace" STATIC ${SOURCES} )
	source_group( "include" FILES "glsl_trace/include/ShaderTrace.h" )
	source_group( "source" FILES "glsl_trace/source/Common.h" "glsl_trace/source/ConvertTraceToRdc.cpp" "glsl_trace/source/InsertDebugAsserts.cpp" "glsl_trace/source/InsertInstructionCounter.cpp" "glsl_trace/source/InsertTraceRecording.cpp" "glsl_trace/source/ParseShaderTrace.cpp" "glsl_trace/source/ShaderDebugUtils.cpp" )
	set_property( TARGET "GLSLTrace" PROPERTY FOLDER "Extensions" )
	target_include_directories( "GLSLTrace" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "GLSLTrace" PRIVATE "glsl_trace/.." )
	target_include_directories( "GLSLTrace" PRIVATE "${FG_GLSLANG_INCLUDE_DIR}" )
	target_link_libraries( "GLSLTrace" "${FG_GLSLANG_LIBRARIES}" )
	target_link_libraries( "GLSLTrace" "STL" )
	target_compile_definitions( "GLSLTrace" PUBLIC "${FG_GLOBAL_DEFINITIONS}" )
	add_dependencies( "GLSLTrace" External.glslang-main )
	# compiler
	target_compile_definitions( "GLSLTrace" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "GLSLTrace" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "GLSLTrace" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "GLSLTrace" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "GLSLTrace" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "GLSLTrace" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "GLSLTrace" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "GLSLTrace" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "GLSLTrace" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


#==================================================================================================
# project: Tests.GLSLTrace
#==================================================================================================
if (${FG_ENABLE_TESTS} AND ${FG_ENABLE_GLSLANG})
	set( SOURCES 
		"../tests/glsl_trace/GLSLShaderTraceTestUtils.h"
		"../tests/glsl_trace/GLSLShaderTrace_Test1.cpp"
		"../tests/glsl_trace/GLSLShaderTrace_Test2.cpp"
		"../tests/glsl_trace/GLSLShaderTrace_Test3.cpp"
		"../tests/glsl_trace/GLSLShaderTrace_Test4.cpp"
		"../tests/glsl_trace/main.cpp"
		"../tests/glsl_trace/ShaderCompiler.cpp"
		"../tests/glsl_trace/ShaderCompiler.h" )
	if (DEFINED ANDROID)
		add_library( "Tests.GLSLTrace" SHARED ${SOURCES} )
	else()
		add_executable( "Tests.GLSLTrace" ${SOURCES} )
	endif()
	source_group( "" FILES "../tests/glsl_trace/GLSLShaderTraceTestUtils.h" "../tests/glsl_trace/GLSLShaderTrace_Test1.cpp" "../tests/glsl_trace/GLSLShaderTrace_Test2.cpp" "../tests/glsl_trace/GLSLShaderTrace_Test3.cpp" "../tests/glsl_trace/GLSLShaderTrace_Test4.cpp" "../tests/glsl_trace/main.cpp" "../tests/glsl_trace/ShaderCompiler.cpp" "../tests/glsl_trace/ShaderCompiler.h" )
	set_property( TARGET "Tests.GLSLTrace" PROPERTY FOLDER "Tests" )
	target_include_directories( "Tests.GLSLTrace" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "Tests.GLSLTrace" PRIVATE "" )
	target_include_directories( "Tests.GLSLTrace" PRIVATE "${FG_GLSLANG_INCLUDE_DIR}" )
	target_link_libraries( "Tests.GLSLTrace" "Framework" )
	if (${FG_ENABLE_GLSLANG})
		target_link_libraries( "Tests.GLSLTrace" "GLSLTrace" )
	endif()
	# compiler
	target_compile_definitions( "Tests.GLSLTrace" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Tests.GLSLTrace" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Tests.GLSLTrace" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Tests.GLSLTrace" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Tests.GLSLTrace" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Tests.GLSLTrace" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Tests.GLSLTrace" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Tests.GLSLTrace" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Tests.GLSLTrace" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


#==================================================================================================
# project: PipelineCompiler
#==================================================================================================
if (${FG_ENABLE_GLSLANG})
	set( SOURCES 
		"pipeline_compiler/EShaderCompilationFlags.h"
		"pipeline_compiler/PrivateDefines.h"
		"pipeline_compiler/SpirvCompiler.cpp"
		"pipeline_compiler/SpirvCompiler.h"
		"pipeline_compiler/SpirvCompiler_Resources.cpp"
		"pipeline_compiler/VCachedDebuggableShaderData.h"
		"pipeline_compiler/VPipelineCompiler.cpp"
		"pipeline_compiler/VPipelineCompiler.h" )
	add_library( "PipelineCompiler" STATIC ${SOURCES} )
	source_group( "" FILES "pipeline_compiler/EShaderCompilationFlags.h" "pipeline_compiler/PrivateDefines.h" "pipeline_compiler/SpirvCompiler.cpp" "pipeline_compiler/SpirvCompiler.h" "pipeline_compiler/SpirvCompiler_Resources.cpp" "pipeline_compiler/VCachedDebuggableShaderData.h" "pipeline_compiler/VPipelineCompiler.cpp" "pipeline_compiler/VPipelineCompiler.h" )
	set_property( TARGET "PipelineCompiler" PROPERTY FOLDER "Extensions" )
	target_include_directories( "PipelineCompiler" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "PipelineCompiler" PRIVATE ".." )
	target_include_directories( "PipelineCompiler" PRIVATE "pipeline_compiler" )
	target_include_directories( "PipelineCompiler" PRIVATE "${FG_GLSLANG_INCLUDE_DIR}" )
	target_link_libraries( "PipelineCompiler" "FrameGraph" )
	target_link_libraries( "PipelineCompiler" "${FG_GLSLANG_LIBRARIES}" )
	target_link_libraries( "PipelineCompiler" "GLSLTrace" )
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
		"../tests/pipeline_compiler/Test_Shader1.cpp"
		"../tests/pipeline_compiler/Test_Shader10.cpp"
		"../tests/pipeline_compiler/Test_Shader11.cpp"
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
	source_group( "" FILES "../tests/pipeline_compiler/main.cpp" "../tests/pipeline_compiler/Test_Shader1.cpp" "../tests/pipeline_compiler/Test_Shader10.cpp" "../tests/pipeline_compiler/Test_Shader11.cpp" "../tests/pipeline_compiler/Test_Shader2.cpp" "../tests/pipeline_compiler/Test_Shader3.cpp" "../tests/pipeline_compiler/Test_Shader4.cpp" "../tests/pipeline_compiler/Test_Shader5.cpp" "../tests/pipeline_compiler/Test_Shader6.cpp" "../tests/pipeline_compiler/Test_Shader7.cpp" "../tests/pipeline_compiler/Test_Shader8.cpp" "../tests/pipeline_compiler/Test_Shader9.cpp" "../tests/pipeline_compiler/Utils.h" )
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
		"offline_compiler/main.cpp"
		"offline_compiler/PipelineCppSerializer.cpp"
		"offline_compiler/PipelineCppSerializer.h" )
	if (DEFINED ANDROID)
		add_library( "OfflineCompiler" SHARED ${SOURCES} )
	else()
		add_executable( "OfflineCompiler" ${SOURCES} )
	endif()
	source_group( "" FILES "offline_compiler/main.cpp" "offline_compiler/PipelineCppSerializer.cpp" "offline_compiler/PipelineCppSerializer.h" )
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
# project: UnitTests.OfflineCompiler
#==================================================================================================
if (${FG_ENABLE_GLSLANG} AND ${FG_ENABLE_TESTS} AND NOT UNIX)
	set( SOURCES 
		"../tests/offline_compiler/main.cpp"
		"../tests/offline_compiler/Test_Serializer1.cpp"
		"../tests/offline_compiler/Test_Serializer2.cpp" )
	if (DEFINED ANDROID)
		add_library( "UnitTests.OfflineCompiler" SHARED ${SOURCES} )
	else()
		add_executable( "UnitTests.OfflineCompiler" ${SOURCES} )
	endif()
	source_group( "" FILES "../tests/offline_compiler/main.cpp" "../tests/offline_compiler/Test_Serializer1.cpp" "../tests/offline_compiler/Test_Serializer2.cpp" )
	set_property( TARGET "UnitTests.OfflineCompiler" PROPERTY FOLDER "Tests" )
	target_include_directories( "UnitTests.OfflineCompiler" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "UnitTests.OfflineCompiler" PRIVATE "../tests/.." )
	target_include_directories( "UnitTests.OfflineCompiler" PRIVATE "" )
	if (${FG_ENABLE_GLSLANG})
		target_link_libraries( "UnitTests.OfflineCompiler" "PipelineCompiler" )
	endif()
	# compiler
	target_compile_definitions( "UnitTests.OfflineCompiler" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "UnitTests.OfflineCompiler" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "UnitTests.OfflineCompiler" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "UnitTests.OfflineCompiler" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "UnitTests.OfflineCompiler" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "UnitTests.OfflineCompiler" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "UnitTests.OfflineCompiler" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "UnitTests.OfflineCompiler" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "UnitTests.OfflineCompiler" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


#==================================================================================================
# project: GraphViz
#==================================================================================================
if (${FG_ENABLE_GRAPHVIZ})
	set( SOURCES 
		"graphviz/GraphViz.cpp"
		"graphviz/GraphViz.h" )
	add_library( "GraphViz" STATIC ${SOURCES} )
	source_group( "" FILES "graphviz/GraphViz.cpp" "graphviz/GraphViz.h" )
	set_property( TARGET "GraphViz" PROPERTY FOLDER "Extensions" )
	target_include_directories( "GraphViz" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_link_libraries( "GraphViz" "STL" )
	target_compile_definitions( "GraphViz" PUBLIC "${FG_GLOBAL_DEFINITIONS}" )
	# compiler
	target_compile_definitions( "GraphViz" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "GraphViz" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "GraphViz" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "GraphViz" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "GraphViz" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "GraphViz" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "GraphViz" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "GraphViz" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "GraphViz" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


#==================================================================================================
# project: Scene
#==================================================================================================
if (${FG_ENABLE_GLM})
	set( SOURCES 
		"scene/SceneManager/Simple/SimpleRayTracing.cpp"
		"scene/SceneManager/Simple/SimpleRayTracing.h"
		"scene/SceneManager/Simple/SimpleScene.cpp"
		"scene/SceneManager/Simple/SimpleScene.h"
		"scene/Utils/Ext/FPSCamera.h"
		"scene/ThreadPool/FGThread.h"
		"scene/ThreadPool/IOThread.h"
		"scene/ThreadPool/ThreadPool.cpp"
		"scene/ThreadPool/ThreadPool.h"
		"scene/Pipelines/ForwardPlus/default.gppln"
		"scene/Renderer/Prototype/ppln/default.glsl"
		"scene/SceneManager/Octree/OctreeScene.h"
		"scene/Loader/Intermediate/IntermediateImage.h"
		"scene/Loader/Intermediate/IntermediateLight.h"
		"scene/Loader/Intermediate/IntermediateMaterial.h"
		"scene/Loader/Intermediate/IntermediateMesh.cpp"
		"scene/Loader/Intermediate/IntermediateMesh.h"
		"scene/Loader/Intermediate/IntermediateScene.h"
		"scene/Loader/Intermediate/VertexAttributes.h"
		"scene/Renderer/Enums.h"
		"scene/Renderer/IRenderTechnique.h"
		"scene/Renderer/RenderQueue.h"
		"scene/Renderer/ScenePreRender.h"
		"scene/Utils/Math/AABB.h"
		"scene/Utils/Math/Camera.h"
		"scene/Utils/Math/Frustum.h"
		"scene/Utils/Math/GLM.h"
		"scene/Utils/Math/OOBB.h"
		"scene/Utils/Math/Radians.h"
		"scene/Utils/Math/Sphere.h"
		"scene/Utils/Math/Transform.h"
		"scene/Renderer/Prototype/RendererPrototype.cpp"
		"scene/Renderer/Prototype/RendererPrototype.h"
		"scene/Common.h"
		"scene/SceneManager/Resources/FpsCamera.h"
		"scene/SceneManager/Resources/LightSource.h"
		"scene/SceneManager/Resources/Material.h"
		"scene/SceneManager/Resources/Model.cpp"
		"scene/SceneManager/Resources/Model.h"
		"scene/SceneManager/Resources/ParticleEmitter.h"
		"scene/SceneManager/Resources/SceletalModel.h"
		"scene/Loader/Assimp/AssimpLoader.cpp"
		"scene/Loader/Assimp/AssimpLoader.h"
		"scene/SceneManager/DefaultImageCache.cpp"
		"scene/SceneManager/DefaultImageCache.h"
		"scene/SceneManager/DefaultSceneManager.cpp"
		"scene/SceneManager/DefaultSceneManager.h"
		"scene/SceneManager/IImageCache.h"
		"scene/SceneManager/ISceneHierarchy.h"
		"scene/SceneManager/ISceneManager.h"
		"scene/SceneManager/IViewport.h"
		"scene/Loader/DevIL/DevILLoader.cpp"
		"scene/Loader/DevIL/DevILLoader.h" )
	add_library( "Scene" STATIC ${SOURCES} )
	source_group( "SceneManager\\Simple" FILES "scene/SceneManager/Simple/SimpleRayTracing.cpp" "scene/SceneManager/Simple/SimpleRayTracing.h" "scene/SceneManager/Simple/SimpleScene.cpp" "scene/SceneManager/Simple/SimpleScene.h" )
	source_group( "Utils\\Ext" FILES "scene/Utils/Ext/FPSCamera.h" )
	source_group( "ThreadPool" FILES "scene/ThreadPool/FGThread.h" "scene/ThreadPool/IOThread.h" "scene/ThreadPool/ThreadPool.cpp" "scene/ThreadPool/ThreadPool.h" )
	source_group( "Pipelines\\ForwardPlus" FILES "scene/Pipelines/ForwardPlus/default.gppln" )
	source_group( "Renderer\\Prototype\\ppln" FILES "scene/Renderer/Prototype/ppln/default.glsl" )
	source_group( "SceneManager\\Octree" FILES "scene/SceneManager/Octree/OctreeScene.h" )
	source_group( "Loader\\Intermediate" FILES "scene/Loader/Intermediate/IntermediateImage.h" "scene/Loader/Intermediate/IntermediateLight.h" "scene/Loader/Intermediate/IntermediateMaterial.h" "scene/Loader/Intermediate/IntermediateMesh.cpp" "scene/Loader/Intermediate/IntermediateMesh.h" "scene/Loader/Intermediate/IntermediateScene.h" "scene/Loader/Intermediate/VertexAttributes.h" )
	source_group( "Renderer" FILES "scene/Renderer/Enums.h" "scene/Renderer/IRenderTechnique.h" "scene/Renderer/RenderQueue.h" "scene/Renderer/ScenePreRender.h" )
	source_group( "Utils\\Math" FILES "scene/Utils/Math/AABB.h" "scene/Utils/Math/Camera.h" "scene/Utils/Math/Frustum.h" "scene/Utils/Math/GLM.h" "scene/Utils/Math/OOBB.h" "scene/Utils/Math/Radians.h" "scene/Utils/Math/Sphere.h" "scene/Utils/Math/Transform.h" )
	source_group( "Renderer\\Prototype" FILES "scene/Renderer/Prototype/RendererPrototype.cpp" "scene/Renderer/Prototype/RendererPrototype.h" )
	source_group( "" FILES "scene/Common.h" )
	source_group( "SceneManager\\Resources" FILES "scene/SceneManager/Resources/FpsCamera.h" "scene/SceneManager/Resources/LightSource.h" "scene/SceneManager/Resources/Material.h" "scene/SceneManager/Resources/Model.cpp" "scene/SceneManager/Resources/Model.h" "scene/SceneManager/Resources/ParticleEmitter.h" "scene/SceneManager/Resources/SceletalModel.h" )
	source_group( "Loader\\Assimp" FILES "scene/Loader/Assimp/AssimpLoader.cpp" "scene/Loader/Assimp/AssimpLoader.h" )
	source_group( "SceneManager" FILES "scene/SceneManager/DefaultImageCache.cpp" "scene/SceneManager/DefaultImageCache.h" "scene/SceneManager/DefaultSceneManager.cpp" "scene/SceneManager/DefaultSceneManager.h" "scene/SceneManager/IImageCache.h" "scene/SceneManager/ISceneHierarchy.h" "scene/SceneManager/ISceneManager.h" "scene/SceneManager/IViewport.h" )
	source_group( "Loader\\DevIL" FILES "scene/Loader/DevIL/DevILLoader.cpp" "scene/Loader/DevIL/DevILLoader.h" )
	set_property( TARGET "Scene" PROPERTY FOLDER "Extensions" )
	target_include_directories( "Scene" PUBLIC "${FG_GLM_INCLUDE_DIR}" )
	target_include_directories( "Scene" PUBLIC "scene/.." )
	target_include_directories( "Scene" PUBLIC "${FG_EXTERNALS_PATH}" )
	if (${FG_ENABLE_DEVIL})
		target_include_directories( "Scene" PRIVATE "${FG_DEVIL_INCLUDE_DIR}" )
	endif()
	if (${FG_ENABLE_ASSIMP})
		target_include_directories( "Scene" PRIVATE "${FG_ASSIMP_INCLUDE_DIR}" )
	endif()
	target_link_libraries( "Scene" "FrameGraph" )
	if (${FG_ENABLE_DEVIL})
		target_link_libraries( "Scene" "${FG_DEVIL_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}ILU${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	endif()
	target_link_libraries( "Scene" "Framework" )
	if (${FG_ENABLE_ASSIMP})
		target_link_libraries( "Scene" "${FG_ASSIMP_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}assimp-vc140-mt${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	endif()
	if (${FG_ENABLE_ASSIMP})
		target_link_libraries( "Scene" "${FG_ASSIMP_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}IrrXML${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	endif()
	if (${FG_ENABLE_ASSIMP})
		target_link_libraries( "Scene" "${FG_ASSIMP_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}zlibstatic${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	endif()
	if (${FG_ENABLE_DEVIL})
		target_link_libraries( "Scene" "${FG_DEVIL_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}DevIL${CMAKE_STATIC_LIBRARY_SUFFIX}" )
	endif()
	if (${FG_ENABLE_ASSIMP})
		add_dependencies( "Scene" External.Assimp )
	endif()
	if (${FG_ENABLE_DEVIL})
		add_dependencies( "Scene" External.DevIL )
	endif()
	add_dependencies( "Scene" External.GLM )
	# compiler
	target_compile_definitions( "Scene" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Scene" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Scene" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Scene" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Scene" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Scene" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Scene" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Scene" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Scene" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


#==================================================================================================
# project: Tests.Scene
#==================================================================================================
if (${FG_ENABLE_TESTS} AND ${FG_ENABLE_GLM})
	set( SOURCES 
		"../tests/scene/main.cpp"
		"../tests/scene/SceneApp.cpp"
		"../tests/scene/SceneApp.h"
		"../tests/scene/UnitTest_Common.h"
		"../tests/scene/UnitTest_Frustum.cpp"
		"../tests/scene/UnitTest_Transformation.cpp" )
	if (DEFINED ANDROID)
		add_library( "Tests.Scene" SHARED ${SOURCES} )
	else()
		add_executable( "Tests.Scene" ${SOURCES} )
	endif()
	source_group( "" FILES "../tests/scene/main.cpp" "../tests/scene/SceneApp.cpp" "../tests/scene/SceneApp.h" "../tests/scene/UnitTest_Common.h" "../tests/scene/UnitTest_Frustum.cpp" "../tests/scene/UnitTest_Transformation.cpp" )
	set_property( TARGET "Tests.Scene" PROPERTY FOLDER "Tests" )
	target_include_directories( "Tests.Scene" PUBLIC "${FG_EXTERNALS_PATH}" )
	if (${FG_ENABLE_GRAPHVIZ})
		target_link_libraries( "Tests.Scene" "GraphViz" )
	endif()
	if (${FG_ENABLE_GLM})
		target_link_libraries( "Tests.Scene" "Scene" )
	endif()
	if (${FG_ENABLE_GLSLANG})
		target_link_libraries( "Tests.Scene" "PipelineCompiler" )
	endif()
	# compiler
	target_compile_definitions( "Tests.Scene" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Tests.Scene" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Tests.Scene" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Tests.Scene" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Tests.Scene" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Tests.Scene" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Tests.Scene" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Tests.Scene" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Tests.Scene" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


#==================================================================================================
# project: UI
#==================================================================================================
if (${FG_ENABLE_IMGUI})
	set( SOURCES 
		"ui/ImguiRenderer.cpp"
		"ui/ImguiRenderer.h"
		"ui/main.cpp" )
	add_library( "UI" STATIC ${SOURCES} )
	source_group( "" FILES "ui/ImguiRenderer.cpp" "ui/ImguiRenderer.h" "ui/main.cpp" )
	set_property( TARGET "UI" PROPERTY FOLDER "Extensions" )
	target_include_directories( "UI" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "UI" PUBLIC "${FG_IMGUI_SOURCE_DIR}" )
	target_link_libraries( "UI" "FrameGraph" )
	target_link_libraries( "UI" "${FG_IMGUI_LIBRARY}" )
	target_link_libraries( "UI" "Framework" )
	add_dependencies( "UI" External.imgui )
	# compiler
	target_compile_definitions( "UI" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "UI" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "UI" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "UI" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "UI" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "UI" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "UI" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "UI" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "UI" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


#==================================================================================================
# project: Tests.UI
#==================================================================================================
if (${FG_ENABLE_TESTS} AND ${FG_ENABLE_IMGUI})
	set( SOURCES 
		"../tests/ui/main.cpp"
		"../tests/ui/UIApp.cpp"
		"../tests/ui/UIApp.h" )
	if (DEFINED ANDROID)
		add_library( "Tests.UI" SHARED ${SOURCES} )
	else()
		add_executable( "Tests.UI" ${SOURCES} )
	endif()
	source_group( "" FILES "../tests/ui/main.cpp" "../tests/ui/UIApp.cpp" "../tests/ui/UIApp.h" )
	set_property( TARGET "Tests.UI" PROPERTY FOLDER "Tests" )
	target_include_directories( "Tests.UI" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "Tests.UI" PRIVATE "" )
	if (${FG_ENABLE_GRAPHVIZ})
		target_link_libraries( "Tests.UI" "GraphViz" )
	endif()
	if (${FG_ENABLE_IMGUI})
		target_link_libraries( "Tests.UI" "UI" )
	endif()
	if (${FG_ENABLE_GLSLANG})
		target_link_libraries( "Tests.UI" "PipelineCompiler" )
	endif()
	# compiler
	target_compile_definitions( "Tests.UI" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Tests.UI" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Tests.UI" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Tests.UI" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Tests.UI" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Tests.UI" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Tests.UI" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Tests.UI" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Tests.UI" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


add_test( NAME "UnitTests.OfflineCompiler" COMMAND "UnitTests.OfflineCompiler" )

add_test( NAME "UnitTests.PipelineCompiler" COMMAND "UnitTests.PipelineCompiler" )


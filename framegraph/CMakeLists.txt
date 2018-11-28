# auto generated file
cmake_minimum_required (VERSION 3.6.0)

set( FrameGraph_VERSION_MAJOR 0 )
set( FrameGraph_VERSION_MINOR 5 )

#==================================================================================================
# project: FrameGraph
#==================================================================================================
set( SOURCES 
	"Vulkan/Memory/VMemoryManager.cpp"
	"Vulkan/Memory/VMemoryManager.h"
	"Vulkan/Memory/VMemoryManager_VMAllocator.h"
	"Vulkan/Memory/VMemoryObj.cpp"
	"Vulkan/Memory/VMemoryObj.h"
	"Vulkan/RayTracing/VRayTracingGeometry.cpp"
	"Vulkan/RayTracing/VRayTracingGeometry.h"
	"Vulkan/RayTracing/VRayTracingScene.cpp"
	"Vulkan/RayTracing/VRayTracingScene.h"
	"Public/BindingIndex.h"
	"Public/BufferDesc.h"
	"Public/BufferView.h"
	"Public/Config.h"
	"Public/EResourceState.h"
	"Public/FGEnums.h"
	"Public/FrameGraph.h"
	"Public/FrameGraphDrawTask.h"
	"Public/FrameGraphTask.h"
	"Public/FrameGraphThread.h"
	"Public/IDs.h"
	"Public/ImageDesc.h"
	"Public/ImageLayer.h"
	"Public/ImageSwizzle.h"
	"Public/ImageView.h"
	"Public/IPipelineCompiler.h"
	"Public/MemoryDesc.h"
	"Public/MipmapLevel.h"
	"Public/MultiSamples.h"
	"Public/Pipeline.h"
	"Public/PipelineResources.h"
	"Public/RayTracingGeometryDesc.h"
	"Public/RayTracingSceneDesc.h"
	"Public/RenderState.h"
	"Public/RenderStateEnums.h"
	"Public/ResourceEnums.h"
	"Public/SamplerDesc.h"
	"Public/SamplerEnums.h"
	"Public/ShaderEnums.h"
	"Public/SubmissionGraph.h"
	"Public/Types.h"
	"Public/VertexDesc.h"
	"Public/VertexEnums.h"
	"Public/VertexInputState.h"
	"Public/VulkanTypes.h"
	"Vulkan/VCommon.h"
	"Vulkan/Thread/VBarrierManager.h"
	"Vulkan/Thread/VDrawTask.h"
	"Vulkan/Thread/VFrameGraphThread.cpp"
	"Vulkan/Thread/VFrameGraphThread.h"
	"Vulkan/Thread/VFrameGraphThread_Tasks.cpp"
	"Vulkan/Thread/VResourceManagerThread.cpp"
	"Vulkan/Thread/VResourceManagerThread.h"
	"Vulkan/Thread/VStagingBufferManager.cpp"
	"Vulkan/Thread/VStagingBufferManager.h"
	"Vulkan/Thread/VTaskGraph.h"
	"Vulkan/Thread/VTaskGraph.hpp"
	"Vulkan/Thread/VTaskProcessor.cpp"
	"Vulkan/Thread/VTaskProcessor.h"
	"Vulkan/Main/VDevice.cpp"
	"Vulkan/Main/VDevice.h"
	"Vulkan/Main/VFrameGraph.cpp"
	"Vulkan/Main/VFrameGraph.h"
	"Vulkan/Main/VResourceManager.cpp"
	"Vulkan/Main/VResourceManager.h"
	"Vulkan/Main/VSubmissionGraph.cpp"
	"Vulkan/Main/VSubmissionGraph.h"
	"Vulkan/Debugger/VDebugger.cpp"
	"Vulkan/Debugger/VDebugger.h"
	"Vulkan/Debugger/VFrameGraphDebugger.cpp"
	"Vulkan/Debugger/VFrameGraphDebugger.h"
	"Vulkan/Debugger/VFrameGraphDebugger2.cpp"
	"Vulkan/Image/VImage.cpp"
	"Vulkan/Image/VImage.h"
	"Vulkan/Image/VLocalImage.cpp"
	"Vulkan/Image/VLocalImage.h"
	"Vulkan/Image/VSampler.cpp"
	"Vulkan/Image/VSampler.h"
	"Vulkan/Swapchain/VSwapchain.h"
	"Vulkan/Swapchain/VSwapchainKHR.cpp"
	"Vulkan/Swapchain/VSwapchainKHR.h"
	"Shared/CreateFrameGraph.cpp"
	"Shared/EnumToString.h"
	"Shared/EnumUtils.h"
	"Shared/ImageDataRange.h"
	"Shared/ImageView.cpp"
	"Shared/ImageViewDesc.cpp"
	"Shared/ImageViewDesc.h"
	"Shared/LocalResourceID.h"
	"Shared/Pipeline.cpp"
	"Shared/PipelineResourcesInitializer.h"
	"Shared/RenderState.cpp"
	"Shared/ResourceBase.h"
	"Shared/ResourceDataRange.h"
	"Shared/VertexInputState.cpp"
	"Vulkan/RenderPass/VFramebuffer.cpp"
	"Vulkan/RenderPass/VFramebuffer.h"
	"Vulkan/RenderPass/VLogicalRenderPass.cpp"
	"Vulkan/RenderPass/VLogicalRenderPass.h"
	"Vulkan/RenderPass/VRenderPass.cpp"
	"Vulkan/RenderPass/VRenderPass.h"
	"Vulkan/RenderPass/VRenderPassCache.cpp"
	"Vulkan/RenderPass/VRenderPassCache.h"
	"Vulkan/Utils/VEnumCast.h"
	"Vulkan/Utils/VEnums.h"
	"Vulkan/Utils/VEnumToString.h"
	"Vulkan/Buffer/VBuffer.cpp"
	"Vulkan/Buffer/VBuffer.h"
	"Vulkan/Buffer/VLocalBuffer.cpp"
	"Vulkan/Buffer/VLocalBuffer.h"
	"../cmake/compilers.cmake"
	"../cmake/compiler_tests.cmake"
	"../cmake/download_assimp.cmake"
	"../cmake/download_devil.cmake"
	"../cmake/download_freeimage.cmake"
	"../cmake/download_glfw.cmake"
	"../cmake/download_glm.cmake"
	"../cmake/download_glslang.cmake"
	"../cmake/download_imgui.cmake"
	"../cmake/download_lodepng.cmake"
	"../cmake/download_mem.cmake"
	"../cmake/download_sdl2.cmake"
	"../cmake/download_sfml.cmake"
	"../cmake/download_vk.cmake"
	"../cmake/download_vma.cmake"
	"../cmake/graphviz.cmake"
	"../cmake/lodepng_CMakeLists.txt"
	"Vulkan/Pipeline/VComputePipeline.cpp"
	"Vulkan/Pipeline/VComputePipeline.h"
	"Vulkan/Pipeline/VDescriptorSetLayout.cpp"
	"Vulkan/Pipeline/VDescriptorSetLayout.h"
	"Vulkan/Pipeline/VGraphicsPipeline.cpp"
	"Vulkan/Pipeline/VGraphicsPipeline.h"
	"Vulkan/Pipeline/VMeshPipeline.cpp"
	"Vulkan/Pipeline/VMeshPipeline.h"
	"Vulkan/Pipeline/VPipelineCache.cpp"
	"Vulkan/Pipeline/VPipelineCache.h"
	"Vulkan/Pipeline/VPipelineLayout.cpp"
	"Vulkan/Pipeline/VPipelineLayout.h"
	"Vulkan/Pipeline/VPipelineResources.cpp"
	"Vulkan/Pipeline/VPipelineResources.h"
	"Vulkan/Pipeline/VRayTracingPipeline.cpp"
	"Vulkan/Pipeline/VRayTracingPipeline.h"
	"CMakeLists.txt"
	"VFG.h" )
add_library( "FrameGraph" STATIC ${SOURCES} )
source_group( "Vulkan\\Memory" FILES "Vulkan/Memory/VMemoryManager.cpp" "Vulkan/Memory/VMemoryManager.h" "Vulkan/Memory/VMemoryManager_VMAllocator.h" "Vulkan/Memory/VMemoryObj.cpp" "Vulkan/Memory/VMemoryObj.h" )
source_group( "Vulkan\\RayTracing" FILES "Vulkan/RayTracing/VRayTracingGeometry.cpp" "Vulkan/RayTracing/VRayTracingGeometry.h" "Vulkan/RayTracing/VRayTracingScene.cpp" "Vulkan/RayTracing/VRayTracingScene.h" )
source_group( "Public" FILES "Public/BindingIndex.h" "Public/BufferDesc.h" "Public/BufferView.h" "Public/Config.h" "Public/EResourceState.h" "Public/FGEnums.h" "Public/FrameGraph.h" "Public/FrameGraphDrawTask.h" "Public/FrameGraphTask.h" "Public/FrameGraphThread.h" "Public/IDs.h" "Public/ImageDesc.h" "Public/ImageLayer.h" "Public/ImageSwizzle.h" "Public/ImageView.h" "Public/IPipelineCompiler.h" "Public/MemoryDesc.h" "Public/MipmapLevel.h" "Public/MultiSamples.h" "Public/Pipeline.h" "Public/PipelineResources.h" "Public/RayTracingGeometryDesc.h" "Public/RayTracingSceneDesc.h" "Public/RenderState.h" "Public/RenderStateEnums.h" "Public/ResourceEnums.h" "Public/SamplerDesc.h" "Public/SamplerEnums.h" "Public/ShaderEnums.h" "Public/SubmissionGraph.h" "Public/Types.h" "Public/VertexDesc.h" "Public/VertexEnums.h" "Public/VertexInputState.h" "Public/VulkanTypes.h" )
source_group( "Vulkan" FILES "Vulkan/VCommon.h" )
source_group( "Vulkan\\Thread" FILES "Vulkan/Thread/VBarrierManager.h" "Vulkan/Thread/VDrawTask.h" "Vulkan/Thread/VFrameGraphThread.cpp" "Vulkan/Thread/VFrameGraphThread.h" "Vulkan/Thread/VFrameGraphThread_Tasks.cpp" "Vulkan/Thread/VResourceManagerThread.cpp" "Vulkan/Thread/VResourceManagerThread.h" "Vulkan/Thread/VStagingBufferManager.cpp" "Vulkan/Thread/VStagingBufferManager.h" "Vulkan/Thread/VTaskGraph.h" "Vulkan/Thread/VTaskGraph.hpp" "Vulkan/Thread/VTaskProcessor.cpp" "Vulkan/Thread/VTaskProcessor.h" )
source_group( "Vulkan\\Main" FILES "Vulkan/Main/VDevice.cpp" "Vulkan/Main/VDevice.h" "Vulkan/Main/VFrameGraph.cpp" "Vulkan/Main/VFrameGraph.h" "Vulkan/Main/VResourceManager.cpp" "Vulkan/Main/VResourceManager.h" "Vulkan/Main/VSubmissionGraph.cpp" "Vulkan/Main/VSubmissionGraph.h" )
source_group( "Vulkan\\Debugger" FILES "Vulkan/Debugger/VDebugger.cpp" "Vulkan/Debugger/VDebugger.h" "Vulkan/Debugger/VFrameGraphDebugger.cpp" "Vulkan/Debugger/VFrameGraphDebugger.h" "Vulkan/Debugger/VFrameGraphDebugger2.cpp" )
source_group( "Vulkan\\Image" FILES "Vulkan/Image/VImage.cpp" "Vulkan/Image/VImage.h" "Vulkan/Image/VLocalImage.cpp" "Vulkan/Image/VLocalImage.h" "Vulkan/Image/VSampler.cpp" "Vulkan/Image/VSampler.h" )
source_group( "Vulkan\\Swapchain" FILES "Vulkan/Swapchain/VSwapchain.h" "Vulkan/Swapchain/VSwapchainKHR.cpp" "Vulkan/Swapchain/VSwapchainKHR.h" )
source_group( "Shared" FILES "Shared/CreateFrameGraph.cpp" "Shared/EnumToString.h" "Shared/EnumUtils.h" "Shared/ImageDataRange.h" "Shared/ImageView.cpp" "Shared/ImageViewDesc.cpp" "Shared/ImageViewDesc.h" "Shared/LocalResourceID.h" "Shared/Pipeline.cpp" "Shared/PipelineResourcesInitializer.h" "Shared/RenderState.cpp" "Shared/ResourceBase.h" "Shared/ResourceDataRange.h" "Shared/VertexInputState.cpp" )
source_group( "Vulkan\\RenderPass" FILES "Vulkan/RenderPass/VFramebuffer.cpp" "Vulkan/RenderPass/VFramebuffer.h" "Vulkan/RenderPass/VLogicalRenderPass.cpp" "Vulkan/RenderPass/VLogicalRenderPass.h" "Vulkan/RenderPass/VRenderPass.cpp" "Vulkan/RenderPass/VRenderPass.h" "Vulkan/RenderPass/VRenderPassCache.cpp" "Vulkan/RenderPass/VRenderPassCache.h" )
source_group( "Vulkan\\Utils" FILES "Vulkan/Utils/VEnumCast.h" "Vulkan/Utils/VEnums.h" "Vulkan/Utils/VEnumToString.h" )
source_group( "Vulkan\\Buffer" FILES "Vulkan/Buffer/VBuffer.cpp" "Vulkan/Buffer/VBuffer.h" "Vulkan/Buffer/VLocalBuffer.cpp" "Vulkan/Buffer/VLocalBuffer.h" )
source_group( "cmake" FILES "../cmake/compilers.cmake" "../cmake/compiler_tests.cmake" "../cmake/download_assimp.cmake" "../cmake/download_devil.cmake" "../cmake/download_freeimage.cmake" "../cmake/download_glfw.cmake" "../cmake/download_glm.cmake" "../cmake/download_glslang.cmake" "../cmake/download_imgui.cmake" "../cmake/download_lodepng.cmake" "../cmake/download_mem.cmake" "../cmake/download_sdl2.cmake" "../cmake/download_sfml.cmake" "../cmake/download_vk.cmake" "../cmake/download_vma.cmake" "../cmake/graphviz.cmake" "../cmake/lodepng_CMakeLists.txt" )
source_group( "Vulkan\\Pipeline" FILES "Vulkan/Pipeline/VComputePipeline.cpp" "Vulkan/Pipeline/VComputePipeline.h" "Vulkan/Pipeline/VDescriptorSetLayout.cpp" "Vulkan/Pipeline/VDescriptorSetLayout.h" "Vulkan/Pipeline/VGraphicsPipeline.cpp" "Vulkan/Pipeline/VGraphicsPipeline.h" "Vulkan/Pipeline/VMeshPipeline.cpp" "Vulkan/Pipeline/VMeshPipeline.h" "Vulkan/Pipeline/VPipelineCache.cpp" "Vulkan/Pipeline/VPipelineCache.h" "Vulkan/Pipeline/VPipelineLayout.cpp" "Vulkan/Pipeline/VPipelineLayout.h" "Vulkan/Pipeline/VPipelineResources.cpp" "Vulkan/Pipeline/VPipelineResources.h" "Vulkan/Pipeline/VRayTracingPipeline.cpp" "Vulkan/Pipeline/VRayTracingPipeline.h" )
source_group( "" FILES "CMakeLists.txt" "VFG.h" )
target_include_directories( "FrameGraph" PUBLIC ".." )
target_include_directories( "FrameGraph" PUBLIC "${FG_EXTERNALS_PATH}" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Utils" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/RenderPass" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Pipeline" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/RayTracing" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Main" )
target_include_directories( "FrameGraph" PRIVATE "" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Swapchain" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Thread" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Image" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Memory" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Debugger" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Buffer" )
if (${FG_ENABLE_STDALLOC})
	target_link_libraries( "FrameGraph" "foonathan_memory" )
endif()
target_link_libraries( "FrameGraph" "VulkanLoader" )
target_link_libraries( "FrameGraph" "STL" )
target_compile_definitions( "FrameGraph" PUBLIC "${FG_GLOBAL_DEFINITIONS}" )
# compiler
target_compile_definitions( "FrameGraph" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
set_target_properties( "FrameGraph" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
target_compile_definitions( "FrameGraph" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
target_compile_options( "FrameGraph" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
set_target_properties( "FrameGraph" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
target_compile_options( "FrameGraph" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
set_target_properties( "FrameGraph" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
target_compile_options( "FrameGraph" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
target_compile_definitions( "FrameGraph" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )

install( TARGETS "FrameGraph" ARCHIVE DESTINATION "lib" )


#==================================================================================================
# project: UnitTests.FrameGraph
#==================================================================================================
if (${FG_ENABLE_TESTS})
	set( SOURCES 
		"../tests/framegraph_unit/DummyTask.h"
		"../tests/framegraph_unit/main.cpp"
		"../tests/framegraph_unit/UnitTest_Common.h"
		"../tests/framegraph_unit/UnitTest_ID.cpp"
		"../tests/framegraph_unit/UnitTest_ImageSwizzle.cpp"
		"../tests/framegraph_unit/UnitTest_Main.cpp"
		"../tests/framegraph_unit/UnitTest_VBuffer.cpp"
		"../tests/framegraph_unit/UnitTest_VertexInput.cpp"
		"../tests/framegraph_unit/UnitTest_VImage.cpp"
		"../tests/framegraph_unit/UnitTest_VResourceManager.cpp" )
	if (DEFINED ANDROID)
		add_library( "UnitTests.FrameGraph" SHARED ${SOURCES} )
	else()
		add_executable( "UnitTests.FrameGraph" ${SOURCES} )
	endif()
	source_group( "" FILES "../tests/framegraph_unit/DummyTask.h" "../tests/framegraph_unit/main.cpp" "../tests/framegraph_unit/UnitTest_Common.h" "../tests/framegraph_unit/UnitTest_ID.cpp" "../tests/framegraph_unit/UnitTest_ImageSwizzle.cpp" "../tests/framegraph_unit/UnitTest_Main.cpp" "../tests/framegraph_unit/UnitTest_VBuffer.cpp" "../tests/framegraph_unit/UnitTest_VertexInput.cpp" "../tests/framegraph_unit/UnitTest_VImage.cpp" "../tests/framegraph_unit/UnitTest_VResourceManager.cpp" )
	set_property( TARGET "UnitTests.FrameGraph" PROPERTY FOLDER "Tests" )
	target_include_directories( "UnitTests.FrameGraph" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Buffer" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/RenderPass" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/RayTracing" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/.." )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Main" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Image" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Memory" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Utils" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Thread" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Debugger" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Swapchain" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Pipeline" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../extensions" )
	target_link_libraries( "UnitTests.FrameGraph" "FrameGraph" )
	target_link_libraries( "UnitTests.FrameGraph" "Framework" )
	# compiler
	target_compile_definitions( "UnitTests.FrameGraph" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "UnitTests.FrameGraph" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "UnitTests.FrameGraph" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "UnitTests.FrameGraph" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "UnitTests.FrameGraph" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "UnitTests.FrameGraph" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "UnitTests.FrameGraph" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "UnitTests.FrameGraph" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "UnitTests.FrameGraph" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


#==================================================================================================
# project: IntTests.FrameGraph
#==================================================================================================
if (${FG_ENABLE_GLSLANG} AND ${FG_ENABLE_TESTS} AND ${FG_ENABLE_GLFW} OR ${FG_ENABLE_SDL2})
	set( SOURCES 
		"../tests/framegraph/DrawingTests/Test_Compute1.cpp"
		"../tests/framegraph/DrawingTests/Test_CopyBuffer1.cpp"
		"../tests/framegraph/DrawingTests/Test_CopyImage1.cpp"
		"../tests/framegraph/DrawingTests/Test_CopyImage2.cpp"
		"../tests/framegraph/DrawingTests/Test_CopyImage3.cpp"
		"../tests/framegraph/DrawingTests/Test_CopyImage4.cpp"
		"../tests/framegraph/DrawingTests/Test_Draw1.cpp"
		"../tests/framegraph/DrawingTests/Test_Draw2.cpp"
		"../tests/framegraph/DrawingTests/Test_Draw3.cpp"
		"../tests/framegraph/FGApp.cpp"
		"../tests/framegraph/FGApp.h"
		"../tests/framegraph/main.cpp"
		"../tests/framegraph/ImplTests/ImplTest_Scene1.cpp" )
	if (DEFINED ANDROID)
		add_library( "IntTests.FrameGraph" SHARED ${SOURCES} )
	else()
		add_executable( "IntTests.FrameGraph" ${SOURCES} )
	endif()
	source_group( "DrawingTests" FILES "../tests/framegraph/DrawingTests/Test_Compute1.cpp" "../tests/framegraph/DrawingTests/Test_CopyBuffer1.cpp" "../tests/framegraph/DrawingTests/Test_CopyImage1.cpp" "../tests/framegraph/DrawingTests/Test_CopyImage2.cpp" "../tests/framegraph/DrawingTests/Test_CopyImage3.cpp" "../tests/framegraph/DrawingTests/Test_CopyImage4.cpp" "../tests/framegraph/DrawingTests/Test_Draw1.cpp" "../tests/framegraph/DrawingTests/Test_Draw2.cpp" "../tests/framegraph/DrawingTests/Test_Draw3.cpp" )
	source_group( "" FILES "../tests/framegraph/FGApp.cpp" "../tests/framegraph/FGApp.h" "../tests/framegraph/main.cpp" )
	source_group( "ImplTests" FILES "../tests/framegraph/ImplTests/ImplTest_Scene1.cpp" )
	set_property( TARGET "IntTests.FrameGraph" PROPERTY FOLDER "Tests" )
	target_include_directories( "IntTests.FrameGraph" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "IntTests.FrameGraph" PRIVATE "../tests/.." )
	target_include_directories( "IntTests.FrameGraph" PRIVATE "../extensions" )
	target_link_libraries( "IntTests.FrameGraph" "FrameGraph" )
	if (${FG_ENABLE_LODEPNG})
		target_link_libraries( "IntTests.FrameGraph" "lodepng" )
	endif()
	target_link_libraries( "IntTests.FrameGraph" "PipelineCompiler" )
	target_link_libraries( "IntTests.FrameGraph" "Framework" )
	target_compile_definitions( "IntTests.FrameGraph" PUBLIC "FG_TEST_GRAPHS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/../tests/framegraph/Graphs\"" )
	target_compile_definitions( "IntTests.FrameGraph" PUBLIC "FG_TEST_DUMPS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/../tests/framegraph/Dumps\"" )
	# compiler
	target_compile_definitions( "IntTests.FrameGraph" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "IntTests.FrameGraph" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "IntTests.FrameGraph" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "IntTests.FrameGraph" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "IntTests.FrameGraph" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "IntTests.FrameGraph" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "IntTests.FrameGraph" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "IntTests.FrameGraph" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "IntTests.FrameGraph" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


add_test( NAME "IntTests.FrameGraph" COMMAND "IntTests.FrameGraph" )

add_test( NAME "UnitTests.FrameGraph" COMMAND "UnitTests.FrameGraph" )


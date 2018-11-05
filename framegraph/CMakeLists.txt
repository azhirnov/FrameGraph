# auto generated file
cmake_minimum_required (VERSION 3.6.0)

set( FrameGraph_VERSION_MAJOR 0 )
set( FrameGraph_VERSION_MINOR 3 )

#==================================================================================================
# project: FrameGraph
#==================================================================================================
set( SOURCES 
	"Vulkan/Memory/VMemoryManager.cpp"
	"Vulkan/Memory/VMemoryManager.h"
	"Vulkan/Memory/VMemoryManager_VMAllocator.h"
	"Vulkan/Memory/VMemoryObj.cpp"
	"Vulkan/Memory/VMemoryObj.h"
	"Public/Config.h"
	"Public/FGEnums.h"
	"Public/FrameGraph.h"
	"Public/FrameGraphDrawTask.h"
	"Public/FrameGraphTask.h"
	"Public/FrameGraphThread.h"
	"Public/IPipelineCompiler.h"
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
	"Vulkan/Thread/VTransferThread.h"
	"Vulkan/Systems/VDevice.cpp"
	"Vulkan/Systems/VDevice.h"
	"Vulkan/Systems/VFrameGraph.cpp"
	"Vulkan/Systems/VFrameGraph.h"
	"Vulkan/Systems/VResourceManager.cpp"
	"Vulkan/Systems/VResourceManager.h"
	"Vulkan/Image/VImage.cpp"
	"Vulkan/Image/VImage.h"
	"Vulkan/Image/VlocalImage.cpp"
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
	"Shared/ThreadID.h"
	"Shared/VertexInputState.cpp"
	"Vulkan/RenderPass/VFramebuffer.cpp"
	"Vulkan/RenderPass/VFramebuffer.h"
	"Vulkan/RenderPass/VLogicalRenderPass.cpp"
	"Vulkan/RenderPass/VLogicalRenderPass.h"
	"Vulkan/RenderPass/VRenderPass.cpp"
	"Vulkan/RenderPass/VRenderPass.h"
	"Vulkan/Utils/VEnumCast.h"
	"Vulkan/Utils/VEnums.h"
	"Vulkan/Utils/VEnumToString.h"
	"Vulkan/Buffer/VBuffer.cpp"
	"Vulkan/Buffer/VBuffer.h"
	"Vulkan/Buffer/VLocalBuffer.cpp"
	"Vulkan/Buffer/VLocalBuffer.h"
	"Public/LowLevel/BindingIndex.h"
	"Public/LowLevel/BufferDesc.h"
	"Public/LowLevel/BufferView.h"
	"Public/LowLevel/EResourceState.h"
	"Public/LowLevel/IDs.h"
	"Public/LowLevel/ImageDesc.h"
	"Public/LowLevel/ImageLayer.h"
	"Public/LowLevel/ImageSwizzle.h"
	"Public/LowLevel/ImageView.h"
	"Public/LowLevel/MemoryDesc.h"
	"Public/LowLevel/MipmapLevel.h"
	"Public/LowLevel/MultiSamples.h"
	"Public/LowLevel/Pipeline.h"
	"Public/LowLevel/PipelineResources.h"
	"Public/LowLevel/RenderState.h"
	"Public/LowLevel/RenderStateEnums.h"
	"Public/LowLevel/ResourceEnums.h"
	"Public/LowLevel/SamplerDesc.h"
	"Public/LowLevel/SamplerEnums.h"
	"Public/LowLevel/ShaderEnums.h"
	"Public/LowLevel/Types.h"
	"Public/LowLevel/VertexDesc.h"
	"Public/LowLevel/VertexEnums.h"
	"Public/LowLevel/VertexInputState.h"
	"Public/LowLevel/VulkanTypes.h"
	"../cmake/compilers.cmake"
	"../cmake/compiler_tests.cmake"
	"../cmake/download_assimp.cmake"
	"../cmake/download_glfw.cmake"
	"../cmake/download_glslang.cmake"
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
source_group( "Public" FILES "Public/Config.h" "Public/FGEnums.h" "Public/FrameGraph.h" "Public/FrameGraphDrawTask.h" "Public/FrameGraphTask.h" "Public/FrameGraphThread.h" "Public/IPipelineCompiler.h" )
source_group( "Vulkan" FILES "Vulkan/VCommon.h" )
source_group( "Vulkan\\Thread" FILES "Vulkan/Thread/VBarrierManager.h" "Vulkan/Thread/VDrawTask.h" "Vulkan/Thread/VFrameGraphThread.cpp" "Vulkan/Thread/VFrameGraphThread.h" "Vulkan/Thread/VFrameGraphThread_Tasks.cpp" "Vulkan/Thread/VResourceManagerThread.cpp" "Vulkan/Thread/VResourceManagerThread.h" "Vulkan/Thread/VStagingBufferManager.cpp" "Vulkan/Thread/VStagingBufferManager.h" "Vulkan/Thread/VTaskGraph.h" "Vulkan/Thread/VTaskGraph.hpp" "Vulkan/Thread/VTaskProcessor.cpp" "Vulkan/Thread/VTaskProcessor.h" "Vulkan/Thread/VTransferThread.h" )
source_group( "Vulkan\\Systems" FILES "Vulkan/Systems/VDevice.cpp" "Vulkan/Systems/VDevice.h" "Vulkan/Systems/VFrameGraph.cpp" "Vulkan/Systems/VFrameGraph.h" "Vulkan/Systems/VResourceManager.cpp" "Vulkan/Systems/VResourceManager.h" )
source_group( "Vulkan\\Image" FILES "Vulkan/Image/VImage.cpp" "Vulkan/Image/VImage.h" "Vulkan/Image/VlocalImage.cpp" "Vulkan/Image/VLocalImage.h" "Vulkan/Image/VSampler.cpp" "Vulkan/Image/VSampler.h" )
source_group( "Vulkan\\Swapchain" FILES "Vulkan/Swapchain/VSwapchain.h" "Vulkan/Swapchain/VSwapchainKHR.cpp" "Vulkan/Swapchain/VSwapchainKHR.h" )
source_group( "Shared" FILES "Shared/CreateFrameGraph.cpp" "Shared/EnumToString.h" "Shared/EnumUtils.h" "Shared/ImageDataRange.h" "Shared/ImageView.cpp" "Shared/ImageViewDesc.cpp" "Shared/ImageViewDesc.h" "Shared/LocalResourceID.h" "Shared/Pipeline.cpp" "Shared/PipelineResourcesInitializer.h" "Shared/RenderState.cpp" "Shared/ResourceBase.h" "Shared/ResourceDataRange.h" "Shared/ThreadID.h" "Shared/VertexInputState.cpp" )
source_group( "Vulkan\\RenderPass" FILES "Vulkan/RenderPass/VFramebuffer.cpp" "Vulkan/RenderPass/VFramebuffer.h" "Vulkan/RenderPass/VLogicalRenderPass.cpp" "Vulkan/RenderPass/VLogicalRenderPass.h" "Vulkan/RenderPass/VRenderPass.cpp" "Vulkan/RenderPass/VRenderPass.h" )
source_group( "Vulkan\\Utils" FILES "Vulkan/Utils/VEnumCast.h" "Vulkan/Utils/VEnums.h" "Vulkan/Utils/VEnumToString.h" )
source_group( "Vulkan\\Buffer" FILES "Vulkan/Buffer/VBuffer.cpp" "Vulkan/Buffer/VBuffer.h" "Vulkan/Buffer/VLocalBuffer.cpp" "Vulkan/Buffer/VLocalBuffer.h" )
source_group( "Public\\LowLevel" FILES "Public/LowLevel/BindingIndex.h" "Public/LowLevel/BufferDesc.h" "Public/LowLevel/BufferView.h" "Public/LowLevel/EResourceState.h" "Public/LowLevel/IDs.h" "Public/LowLevel/ImageDesc.h" "Public/LowLevel/ImageLayer.h" "Public/LowLevel/ImageSwizzle.h" "Public/LowLevel/ImageView.h" "Public/LowLevel/MemoryDesc.h" "Public/LowLevel/MipmapLevel.h" "Public/LowLevel/MultiSamples.h" "Public/LowLevel/Pipeline.h" "Public/LowLevel/PipelineResources.h" "Public/LowLevel/RenderState.h" "Public/LowLevel/RenderStateEnums.h" "Public/LowLevel/ResourceEnums.h" "Public/LowLevel/SamplerDesc.h" "Public/LowLevel/SamplerEnums.h" "Public/LowLevel/ShaderEnums.h" "Public/LowLevel/Types.h" "Public/LowLevel/VertexDesc.h" "Public/LowLevel/VertexEnums.h" "Public/LowLevel/VertexInputState.h" "Public/LowLevel/VulkanTypes.h" )
source_group( "cmake" FILES "../cmake/compilers.cmake" "../cmake/compiler_tests.cmake" "../cmake/download_assimp.cmake" "../cmake/download_glfw.cmake" "../cmake/download_glslang.cmake" "../cmake/download_lodepng.cmake" "../cmake/download_mem.cmake" "../cmake/download_sdl2.cmake" "../cmake/download_sfml.cmake" "../cmake/download_vk.cmake" "../cmake/download_vma.cmake" "../cmake/graphviz.cmake" "../cmake/lodepng_CMakeLists.txt" )
source_group( "Vulkan\\Pipeline" FILES "Vulkan/Pipeline/VComputePipeline.cpp" "Vulkan/Pipeline/VComputePipeline.h" "Vulkan/Pipeline/VDescriptorSetLayout.cpp" "Vulkan/Pipeline/VDescriptorSetLayout.h" "Vulkan/Pipeline/VGraphicsPipeline.cpp" "Vulkan/Pipeline/VGraphicsPipeline.h" "Vulkan/Pipeline/VMeshPipeline.cpp" "Vulkan/Pipeline/VMeshPipeline.h" "Vulkan/Pipeline/VPipelineCache.cpp" "Vulkan/Pipeline/VPipelineCache.h" "Vulkan/Pipeline/VPipelineLayout.cpp" "Vulkan/Pipeline/VPipelineLayout.h" "Vulkan/Pipeline/VPipelineResources.cpp" "Vulkan/Pipeline/VPipelineResources.h" "Vulkan/Pipeline/VRayTracingPipeline.cpp" "Vulkan/Pipeline/VRayTracingPipeline.h" )
source_group( "" FILES "CMakeLists.txt" "VFG.h" )
target_include_directories( "FrameGraph" PUBLIC ".." )
target_include_directories( "FrameGraph" PUBLIC "${FG_EXTERNALS_PATH}" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Utils" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/RenderPass" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Pipeline" )
target_include_directories( "FrameGraph" PRIVATE "" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Swapchain" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Systems" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Thread" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Image" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Memory" )
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
		"../tests/framegraph_unit/UnitTestCommon.h"
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
	source_group( "" FILES "../tests/framegraph_unit/DummyTask.h" "../tests/framegraph_unit/main.cpp" "../tests/framegraph_unit/UnitTestCommon.h" "../tests/framegraph_unit/UnitTest_ImageSwizzle.cpp" "../tests/framegraph_unit/UnitTest_Main.cpp" "../tests/framegraph_unit/UnitTest_VBuffer.cpp" "../tests/framegraph_unit/UnitTest_VertexInput.cpp" "../tests/framegraph_unit/UnitTest_VImage.cpp" "../tests/framegraph_unit/UnitTest_VResourceManager.cpp" )
	set_property( TARGET "UnitTests.FrameGraph" PROPERTY FOLDER "Tests" )
	target_include_directories( "UnitTests.FrameGraph" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Buffer" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/RenderPass" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/.." )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Image" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Memory" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Utils" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Systems" )
	target_include_directories( "UnitTests.FrameGraph" PRIVATE "../tests/framegraph_unit/../../framegraph/Vulkan/Thread" )
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
		"../tests/framegraph/DrawTests/DrawTest_CopyBuffer.cpp"
		"../tests/framegraph/DrawTests/DrawTest_CopyImage2D.cpp"
		"../tests/framegraph/DrawTests/DrawTest_FirstTriangle.cpp"
		"../tests/framegraph/DrawTests/DrawTest_GLSLCompute.cpp"
		"../tests/framegraph/FGApp.cpp"
		"../tests/framegraph/FGApp.h"
		"../tests/framegraph/main.cpp" )
	if (DEFINED ANDROID)
		add_library( "IntTests.FrameGraph" SHARED ${SOURCES} )
	else()
		add_executable( "IntTests.FrameGraph" ${SOURCES} )
	endif()
	source_group( "DrawTests" FILES "../tests/framegraph/DrawTests/DrawTest_CopyBuffer.cpp" "../tests/framegraph/DrawTests/DrawTest_CopyImage2D.cpp" "../tests/framegraph/DrawTests/DrawTest_FirstTriangle.cpp" "../tests/framegraph/DrawTests/DrawTest_GLSLCompute.cpp" )
	source_group( "" FILES "../tests/framegraph/FGApp.cpp" "../tests/framegraph/FGApp.h" "../tests/framegraph/main.cpp" )
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


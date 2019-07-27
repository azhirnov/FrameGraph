# auto generated file
cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

set( FrameGraph_VERSION_MAJOR 0 )
set( FrameGraph_VERSION_MINOR 11 )

#==================================================================================================
# project: FrameGraph
#==================================================================================================
set( SOURCES 
	"Vulkan/Memory/VMemoryManager.cpp"
	"Vulkan/Memory/VMemoryManager.h"
	"Vulkan/Memory/VMemoryManager_VMAllocator.cpp"
	"Vulkan/Memory/VMemoryObj.cpp"
	"Vulkan/Memory/VMemoryObj.h"
	"Vulkan/RayTracing/VLocalRTGeometry.cpp"
	"Vulkan/RayTracing/VLocalRTGeometry.h"
	"Vulkan/RayTracing/VLocalRTScene.cpp"
	"Vulkan/RayTracing/VLocalRTScene.h"
	"Vulkan/RayTracing/VRayTracingGeometry.cpp"
	"Vulkan/RayTracing/VRayTracingGeometry.h"
	"Vulkan/RayTracing/VRayTracingPipeline.cpp"
	"Vulkan/RayTracing/VRayTracingPipeline.h"
	"Vulkan/RayTracing/VRayTracingScene.cpp"
	"Vulkan/RayTracing/VRayTracingScene.h"
	"Vulkan/RayTracing/VRayTracingShaderTable.cpp"
	"Vulkan/RayTracing/VRayTracingShaderTable.h"
	"Public/BindingIndex.h"
	"Public/BufferDesc.h"
	"Public/BufferView.h"
	"Public/ColorScheme.h"
	"Public/CommandBuffer.h"
	"Public/CommandBufferPtr.h"
	"Public/Config.h"
	"Public/DrawCommandBuffer.h"
	"Public/DrawContext.h"
	"Public/EResourceState.h"
	"Public/FGEnums.h"
	"Public/FrameGraph.h"
	"Public/FrameGraphDrawTask.h"
	"Public/FrameGraphTask.h"
	"Public/IDs.h"
	"Public/ImageDesc.h"
	"Public/ImageLayer.h"
	"Public/ImageSwizzle.h"
	"Public/ImageView.h"
	"Public/MemoryDesc.h"
	"Public/MipmapLevel.h"
	"Public/MultiSamples.h"
	"Public/Pipeline.h"
	"Public/PipelineCompiler.h"
	"Public/PipelineResources.h"
	"Public/RayTracingEnums.h"
	"Public/RayTracingGeometryDesc.h"
	"Public/RayTracingSceneDesc.h"
	"Public/RenderPassDesc.h"
	"Public/RenderState.h"
	"Public/RenderStateEnums.h"
	"Public/ResourceEnums.h"
	"Public/SamplerDesc.h"
	"Public/SamplerEnums.h"
	"Public/ShaderEnums.h"
	"Public/Types.h"
	"Public/VertexDesc.h"
	"Public/VertexEnums.h"
	"Public/VertexInputState.h"
	"Public/VulkanTypes.h"
	"Vulkan/VCommon.h"
	"Vulkan/Instance/VDevice.cpp"
	"Vulkan/Instance/VDevice.h"
	"Vulkan/Instance/VFrameGraph.cpp"
	"Vulkan/Instance/VFrameGraph.h"
	"Vulkan/Instance/VResourceManager.cpp"
	"Vulkan/Instance/VResourceManager.h"
	"Vulkan/Debugger/VDebugger.cpp"
	"Vulkan/Debugger/VDebugger.h"
	"Vulkan/Debugger/VLocalDebugger.cpp"
	"Vulkan/Debugger/VLocalDebugger.h"
	"Vulkan/Debugger/VLocalDebugger2.cpp"
	"Vulkan/Image/VImage.cpp"
	"Vulkan/Image/VImage.h"
	"Vulkan/Image/VLocalImage.cpp"
	"Vulkan/Image/VLocalImage.h"
	"Vulkan/Image/VSampler.cpp"
	"Vulkan/Image/VSampler.h"
	"Vulkan/Swapchain/VSwapchain.cpp"
	"Vulkan/Swapchain/VSwapchain.h"
	"Shared/CreateFrameGraph.cpp"
	"Shared/EnumToString.h"
	"Shared/EnumUtils.h"
	"Shared/FrameGraph_Statistics.cpp"
	"Shared/HashCollisionCheck.h"
	"Shared/ImageDataRange.h"
	"Shared/ImageView.cpp"
	"Shared/ImageViewDesc.cpp"
	"Shared/ImageViewDesc.h"
	"Shared/LocalResourceID.h"
	"Shared/Pipeline.cpp"
	"Shared/PipelineResources.cpp"
	"Shared/PipelineResourcesHelper.h"
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
	"Vulkan/Utils/FGEnumCast.h"
	"Vulkan/Utils/VEnumCast.h"
	"Vulkan/Utils/VEnums.h"
	"Vulkan/Utils/VEnumToString.h"
	"Vulkan/Buffer/VBuffer.cpp"
	"Vulkan/Buffer/VBuffer.h"
	"Vulkan/Buffer/VLocalBuffer.cpp"
	"Vulkan/Buffer/VLocalBuffer.h"
	"../cmake/angelscript_CMakeLists.txt"
	"../cmake/compilers.cmake"
	"../cmake/compilers_minimal.cmake"
	"../cmake/compiler_tests.cmake"
	"../cmake/download_angelscript.cmake"
	"../cmake/download_assimp.cmake"
	"../cmake/download_devil.cmake"
	"../cmake/download_freeimage.cmake"
	"../cmake/download_glfw.cmake"
	"../cmake/download_glm.cmake"
	"../cmake/download_glslang.cmake"
	"../cmake/download_imgui.cmake"
	"../cmake/download_lodepng.cmake"
	"../cmake/download_openvr.cmake"
	"../cmake/download_sdl2.cmake"
	"../cmake/download_stdalloc.cmake"
	"../cmake/download_stdoptional.cmake"
	"../cmake/download_stdstringview.cmake"
	"../cmake/download_stdvariant.cmake"
	"../cmake/download_vk.cmake"
	"../cmake/download_vma.cmake"
	"../cmake/graphviz.cmake"
	"../cmake/imgui_CMakeLists.txt"
	"../cmake/lodepng_CMakeLists.txt"
	"Vulkan/Pipeline/VComputePipeline.cpp"
	"Vulkan/Pipeline/VComputePipeline.h"
	"Vulkan/Pipeline/VGraphicsPipeline.cpp"
	"Vulkan/Pipeline/VGraphicsPipeline.h"
	"Vulkan/Pipeline/VMeshPipeline.cpp"
	"Vulkan/Pipeline/VMeshPipeline.h"
	"Vulkan/Pipeline/VPipelineCache.cpp"
	"Vulkan/Pipeline/VPipelineCache.h"
	"Vulkan/Pipeline/VPipelineLayout.cpp"
	"Vulkan/Pipeline/VPipelineLayout.h"
	"CMakeLists.txt"
	"FG.h"
	"Vulkan/CommandBuffer/VBarrierManager.h"
	"Vulkan/CommandBuffer/VCmdBatch.cpp"
	"Vulkan/CommandBuffer/VCmdBatch.h"
	"Vulkan/CommandBuffer/VCommandBuffer.cpp"
	"Vulkan/CommandBuffer/VCommandBuffer.h"
	"Vulkan/CommandBuffer/VCommandPool.cpp"
	"Vulkan/CommandBuffer/VCommandPool.h"
	"Vulkan/CommandBuffer/VDrawTask.h"
	"Vulkan/CommandBuffer/VSubmitted.cpp"
	"Vulkan/CommandBuffer/VSubmitted.h"
	"Vulkan/CommandBuffer/VTaskGraph.h"
	"Vulkan/CommandBuffer/VTaskGraph.hpp"
	"Vulkan/CommandBuffer/VTaskProcessor.cpp"
	"Vulkan/CommandBuffer/VTaskProcessor.h"
	"Vulkan/Descriptors/VDescriptorManager.cpp"
	"Vulkan/Descriptors/VDescriptorManager.h"
	"Vulkan/Descriptors/VDescriptorSetLayout.cpp"
	"Vulkan/Descriptors/VDescriptorSetLayout.h"
	"Vulkan/Descriptors/VPipelineResources.cpp"
	"Vulkan/Descriptors/VPipelineResources.h" )
add_library( "FrameGraph" STATIC ${SOURCES} )
source_group( "Vulkan\\Memory" FILES "Vulkan/Memory/VMemoryManager.cpp" "Vulkan/Memory/VMemoryManager.h" "Vulkan/Memory/VMemoryManager_VMAllocator.cpp" "Vulkan/Memory/VMemoryObj.cpp" "Vulkan/Memory/VMemoryObj.h" )
source_group( "Vulkan\\RayTracing" FILES "Vulkan/RayTracing/VLocalRTGeometry.cpp" "Vulkan/RayTracing/VLocalRTGeometry.h" "Vulkan/RayTracing/VLocalRTScene.cpp" "Vulkan/RayTracing/VLocalRTScene.h" "Vulkan/RayTracing/VRayTracingGeometry.cpp" "Vulkan/RayTracing/VRayTracingGeometry.h" "Vulkan/RayTracing/VRayTracingPipeline.cpp" "Vulkan/RayTracing/VRayTracingPipeline.h" "Vulkan/RayTracing/VRayTracingScene.cpp" "Vulkan/RayTracing/VRayTracingScene.h" "Vulkan/RayTracing/VRayTracingShaderTable.cpp" "Vulkan/RayTracing/VRayTracingShaderTable.h" )
source_group( "Public" FILES "Public/BindingIndex.h" "Public/BufferDesc.h" "Public/BufferView.h" "Public/ColorScheme.h" "Public/CommandBuffer.h" "Public/CommandBufferPtr.h" "Public/Config.h" "Public/DrawCommandBuffer.h" "Public/DrawContext.h" "Public/EResourceState.h" "Public/FGEnums.h" "Public/FrameGraph.h" "Public/FrameGraphDrawTask.h" "Public/FrameGraphTask.h" "Public/IDs.h" "Public/ImageDesc.h" "Public/ImageLayer.h" "Public/ImageSwizzle.h" "Public/ImageView.h" "Public/MemoryDesc.h" "Public/MipmapLevel.h" "Public/MultiSamples.h" "Public/Pipeline.h" "Public/PipelineCompiler.h" "Public/PipelineResources.h" "Public/RayTracingEnums.h" "Public/RayTracingGeometryDesc.h" "Public/RayTracingSceneDesc.h" "Public/RenderPassDesc.h" "Public/RenderState.h" "Public/RenderStateEnums.h" "Public/ResourceEnums.h" "Public/SamplerDesc.h" "Public/SamplerEnums.h" "Public/ShaderEnums.h" "Public/Types.h" "Public/VertexDesc.h" "Public/VertexEnums.h" "Public/VertexInputState.h" "Public/VulkanTypes.h" )
source_group( "Vulkan" FILES "Vulkan/VCommon.h" )
source_group( "Vulkan\\Instance" FILES "Vulkan/Instance/VDevice.cpp" "Vulkan/Instance/VDevice.h" "Vulkan/Instance/VFrameGraph.cpp" "Vulkan/Instance/VFrameGraph.h" "Vulkan/Instance/VResourceManager.cpp" "Vulkan/Instance/VResourceManager.h" )
source_group( "Vulkan\\Debugger" FILES "Vulkan/Debugger/VDebugger.cpp" "Vulkan/Debugger/VDebugger.h" "Vulkan/Debugger/VLocalDebugger.cpp" "Vulkan/Debugger/VLocalDebugger.h" "Vulkan/Debugger/VLocalDebugger2.cpp" )
source_group( "Vulkan\\Image" FILES "Vulkan/Image/VImage.cpp" "Vulkan/Image/VImage.h" "Vulkan/Image/VLocalImage.cpp" "Vulkan/Image/VLocalImage.h" "Vulkan/Image/VSampler.cpp" "Vulkan/Image/VSampler.h" )
source_group( "Vulkan\\Swapchain" FILES "Vulkan/Swapchain/VSwapchain.cpp" "Vulkan/Swapchain/VSwapchain.h" )
source_group( "Shared" FILES "Shared/CreateFrameGraph.cpp" "Shared/EnumToString.h" "Shared/EnumUtils.h" "Shared/FrameGraph_Statistics.cpp" "Shared/HashCollisionCheck.h" "Shared/ImageDataRange.h" "Shared/ImageView.cpp" "Shared/ImageViewDesc.cpp" "Shared/ImageViewDesc.h" "Shared/LocalResourceID.h" "Shared/Pipeline.cpp" "Shared/PipelineResources.cpp" "Shared/PipelineResourcesHelper.h" "Shared/RenderState.cpp" "Shared/ResourceBase.h" "Shared/ResourceDataRange.h" "Shared/VertexInputState.cpp" )
source_group( "Vulkan\\RenderPass" FILES "Vulkan/RenderPass/VFramebuffer.cpp" "Vulkan/RenderPass/VFramebuffer.h" "Vulkan/RenderPass/VLogicalRenderPass.cpp" "Vulkan/RenderPass/VLogicalRenderPass.h" "Vulkan/RenderPass/VRenderPass.cpp" "Vulkan/RenderPass/VRenderPass.h" )
source_group( "Vulkan\\Utils" FILES "Vulkan/Utils/FGEnumCast.h" "Vulkan/Utils/VEnumCast.h" "Vulkan/Utils/VEnums.h" "Vulkan/Utils/VEnumToString.h" )
source_group( "Vulkan\\Buffer" FILES "Vulkan/Buffer/VBuffer.cpp" "Vulkan/Buffer/VBuffer.h" "Vulkan/Buffer/VLocalBuffer.cpp" "Vulkan/Buffer/VLocalBuffer.h" )
source_group( "cmake" FILES "../cmake/angelscript_CMakeLists.txt" "../cmake/compilers.cmake" "../cmake/compilers_minimal.cmake" "../cmake/compiler_tests.cmake" "../cmake/download_angelscript.cmake" "../cmake/download_assimp.cmake" "../cmake/download_devil.cmake" "../cmake/download_freeimage.cmake" "../cmake/download_freetype.cmake" "../cmake/download_glfw.cmake" "../cmake/download_glm.cmake" "../cmake/download_glslang.cmake" "../cmake/download_imgui.cmake" "../cmake/download_lodepng.cmake" "../cmake/download_openvr.cmake" "../cmake/download_sdl2.cmake" "../cmake/download_stdalloc.cmake" "../cmake/download_stdoptional.cmake" "../cmake/download_stdstringview.cmake" "../cmake/download_stdvariant.cmake" "../cmake/download_utf8proc.cmake" "../cmake/download_vk.cmake" "../cmake/download_vma.cmake" "../cmake/graphviz.cmake" "../cmake/imgui_CMakeLists.txt" "../cmake/lodepng_CMakeLists.txt" )
source_group( "Vulkan\\Pipeline" FILES "Vulkan/Pipeline/VComputePipeline.cpp" "Vulkan/Pipeline/VComputePipeline.h" "Vulkan/Pipeline/VGraphicsPipeline.cpp" "Vulkan/Pipeline/VGraphicsPipeline.h" "Vulkan/Pipeline/VMeshPipeline.cpp" "Vulkan/Pipeline/VMeshPipeline.h" "Vulkan/Pipeline/VPipelineCache.cpp" "Vulkan/Pipeline/VPipelineCache.h" "Vulkan/Pipeline/VPipelineLayout.cpp" "Vulkan/Pipeline/VPipelineLayout.h" )
source_group( "" FILES "CMakeLists.txt" "FG.h" )
source_group( "Vulkan\\CommandBuffer" FILES "Vulkan/CommandBuffer/VBarrierManager.h" "Vulkan/CommandBuffer/VCmdBatch.cpp" "Vulkan/CommandBuffer/VCmdBatch.h" "Vulkan/CommandBuffer/VCommandBuffer.cpp" "Vulkan/CommandBuffer/VCommandBuffer.h" "Vulkan/CommandBuffer/VCommandPool.cpp" "Vulkan/CommandBuffer/VCommandPool.h" "Vulkan/CommandBuffer/VDrawTask.h" "Vulkan/CommandBuffer/VSubmitted.cpp" "Vulkan/CommandBuffer/VSubmitted.h" "Vulkan/CommandBuffer/VTaskGraph.h" "Vulkan/CommandBuffer/VTaskGraph.hpp" "Vulkan/CommandBuffer/VTaskProcessor.cpp" "Vulkan/CommandBuffer/VTaskProcessor.h" )
source_group( "Vulkan\\Descriptors" FILES "Vulkan/Descriptors/VDescriptorManager.cpp" "Vulkan/Descriptors/VDescriptorManager.h" "Vulkan/Descriptors/VDescriptorSetLayout.cpp" "Vulkan/Descriptors/VDescriptorSetLayout.h" "Vulkan/Descriptors/VPipelineResources.cpp" "Vulkan/Descriptors/VPipelineResources.h" )
target_include_directories( "FrameGraph" PUBLIC ".." )
target_include_directories( "FrameGraph" PUBLIC "${FG_EXTERNALS_PATH}" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Utils" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/RenderPass" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Pipeline" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/RayTracing" )
target_include_directories( "FrameGraph" PRIVATE "" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/CommandBuffer" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Swapchain" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Instance" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Image" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Memory" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Debugger" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Buffer" )
target_include_directories( "FrameGraph" PRIVATE "Vulkan/Descriptors" )
if (${FG_ENABLE_STDALLOC})
	target_link_libraries( "FrameGraph" "foonathan_memory" )
endif()
target_link_libraries( "FrameGraph" "VulkanLoader" )
target_link_libraries( "FrameGraph" "STL" )
target_compile_definitions( "FrameGraph" PUBLIC "${FG_GLOBAL_DEFINITIONS}" )
# compiler
if (PROJECTS_SHARED_CXX_FLAGS_DEBUG)
	target_compile_options( "FrameGraph" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
endif()
if (PROJECTS_SHARED_CXX_FLAGS_RELEASE)
	target_compile_options( "FrameGraph" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
endif()
if (PROJECTS_SHARED_DEFINES_RELEASE)
	target_compile_definitions( "FrameGraph" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
endif()
if (PROJECTS_SHARED_DEFINES_DEBUG)
	target_compile_definitions( "FrameGraph" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
endif()
if (PROJECTS_SHARED_DEFINES_PROFILE)
	target_compile_definitions( "FrameGraph" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
endif()
if (PROJECTS_SHARED_LINKER_FLAGS_PROFILE)
	set_target_properties( "FrameGraph" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
endif()
if (PROJECTS_SHARED_LINKER_FLAGS_DEBUG)
	set_target_properties( "FrameGraph" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
endif()
if (PROJECTS_SHARED_LINKER_FLAGS_RELEASE)
	set_target_properties( "FrameGraph" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
endif()
if (PROJECTS_SHARED_CXX_FLAGS_PROFILE)
	target_compile_options( "FrameGraph" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
endif()
set_target_properties( "FrameGraph" PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )


install( TARGETS "FrameGraph" ARCHIVE DESTINATION "lib" )


#==================================================================================================
# project: Tests.FrameGraph
#==================================================================================================
if (${FG_ENABLE_GLSLANG} AND ${FG_ENABLE_TESTS} AND ${FG_ENABLE_GLFW} OR ${FG_ENABLE_SDL2})
	set( SOURCES 
		"../tests/framegraph/UnitTests/DummyTask.h"
		"../tests/framegraph/UnitTests/UnitTest_Common.h"
		"../tests/framegraph/UnitTests/UnitTest_ID.cpp"
		"../tests/framegraph/UnitTests/UnitTest_ImageSwizzle.cpp"
		"../tests/framegraph/UnitTests/UnitTest_PixelFormat.cpp"
		"../tests/framegraph/UnitTests/UnitTest_VBuffer.cpp"
		"../tests/framegraph/UnitTests/UnitTest_VertexInput.cpp"
		"../tests/framegraph/UnitTests/UnitTest_VImage.cpp"
		"../tests/framegraph/UnitTests/UnitTest_VResourceManager.cpp"
		"../tests/framegraph/DrawingTests/Test_ArrayOfTextures1.cpp"
		"../tests/framegraph/DrawingTests/Test_ArrayOfTextures2.cpp"
		"../tests/framegraph/DrawingTests/Test_AsyncCompute1.cpp"
		"../tests/framegraph/DrawingTests/Test_AsyncCompute2.cpp"
		"../tests/framegraph/DrawingTests/Test_Compute1.cpp"
		"../tests/framegraph/DrawingTests/Test_Compute2.cpp"
		"../tests/framegraph/DrawingTests/Test_CopyBuffer1.cpp"
		"../tests/framegraph/DrawingTests/Test_CopyImage1.cpp"
		"../tests/framegraph/DrawingTests/Test_CopyImage2.cpp"
		"../tests/framegraph/DrawingTests/Test_CopyImage3.cpp"
		"../tests/framegraph/DrawingTests/Test_Draw1.cpp"
		"../tests/framegraph/DrawingTests/Test_Draw2.cpp"
		"../tests/framegraph/DrawingTests/Test_Draw3.cpp"
		"../tests/framegraph/DrawingTests/Test_Draw4.cpp"
		"../tests/framegraph/DrawingTests/Test_Draw5.cpp"
		"../tests/framegraph/DrawingTests/Test_Draw6.cpp"
		"../tests/framegraph/DrawingTests/Test_DrawMeshes1.cpp"
		"../tests/framegraph/DrawingTests/Test_DynamicOffset.cpp"
		"../tests/framegraph/DrawingTests/Test_ExternalCmdBuf1.cpp"
		"../tests/framegraph/DrawingTests/Test_InvalidID.cpp"
		"../tests/framegraph/DrawingTests/Test_PushConst1.cpp"
		"../tests/framegraph/DrawingTests/Test_RawDraw1.cpp"
		"../tests/framegraph/DrawingTests/Test_RayTracingDebugger1.cpp"
		"../tests/framegraph/DrawingTests/Test_ReadAttachment1.cpp"
		"../tests/framegraph/DrawingTests/Test_ShaderDebugger1.cpp"
		"../tests/framegraph/DrawingTests/Test_ShaderDebugger2.cpp"
		"../tests/framegraph/DrawingTests/Test_ShadingRate1.cpp"
		"../tests/framegraph/DrawingTests/Test_TraceRays1.cpp"
		"../tests/framegraph/DrawingTests/Test_TraceRays2.cpp"
		"../tests/framegraph/DrawingTests/Test_TraceRays3.cpp"
		"../tests/framegraph/FGApp.cpp"
		"../tests/framegraph/FGApp.h"
		"../tests/framegraph/main.cpp"
		"../tests/framegraph/ImplTests/ImplTest_CacheOverflow1.cpp"
		"../tests/framegraph/ImplTests/ImplTest_Multithreading1.cpp"
		"../tests/framegraph/ImplTests/ImplTest_Multithreading2.cpp"
		"../tests/framegraph/ImplTests/ImplTest_Multithreading3.cpp"
		"../tests/framegraph/ImplTests/ImplTest_Multithreading4.cpp"
		"../tests/framegraph/ImplTests/ImplTest_Scene1.cpp" )
	if (DEFINED ANDROID)
		add_library( "Tests.FrameGraph" SHARED ${SOURCES} )
	else()
		add_executable( "Tests.FrameGraph" ${SOURCES} )
	endif()
	source_group( "UnitTests" FILES "../tests/framegraph/UnitTests/DummyTask.h" "../tests/framegraph/UnitTests/UnitTest_Common.h" "../tests/framegraph/UnitTests/UnitTest_ID.cpp" "../tests/framegraph/UnitTests/UnitTest_ImageSwizzle.cpp" "../tests/framegraph/UnitTests/UnitTest_PixelFormat.cpp" "../tests/framegraph/UnitTests/UnitTest_VBuffer.cpp" "../tests/framegraph/UnitTests/UnitTest_VertexInput.cpp" "../tests/framegraph/UnitTests/UnitTest_VImage.cpp" "../tests/framegraph/UnitTests/UnitTest_VResourceManager.cpp" )
	source_group( "DrawingTests" FILES "../tests/framegraph/DrawingTests/Test_ArrayOfTextures1.cpp" "../tests/framegraph/DrawingTests/Test_ArrayOfTextures2.cpp" "../tests/framegraph/DrawingTests/Test_AsyncCompute1.cpp" "../tests/framegraph/DrawingTests/Test_AsyncCompute2.cpp" "../tests/framegraph/DrawingTests/Test_Compute1.cpp" "../tests/framegraph/DrawingTests/Test_Compute2.cpp" "../tests/framegraph/DrawingTests/Test_CopyBuffer1.cpp" "../tests/framegraph/DrawingTests/Test_CopyImage1.cpp" "../tests/framegraph/DrawingTests/Test_CopyImage2.cpp" "../tests/framegraph/DrawingTests/Test_CopyImage3.cpp" "../tests/framegraph/DrawingTests/Test_Draw1.cpp" "../tests/framegraph/DrawingTests/Test_Draw2.cpp" "../tests/framegraph/DrawingTests/Test_Draw3.cpp" "../tests/framegraph/DrawingTests/Test_Draw4.cpp" "../tests/framegraph/DrawingTests/Test_Draw5.cpp" "../tests/framegraph/DrawingTests/Test_Draw6.cpp" "../tests/framegraph/DrawingTests/Test_DrawMeshes1.cpp" "../tests/framegraph/DrawingTests/Test_DynamicOffset.cpp" "../tests/framegraph/DrawingTests/Test_ExternalCmdBuf1.cpp" "../tests/framegraph/DrawingTests/Test_InvalidID.cpp" "../tests/framegraph/DrawingTests/Test_PushConst1.cpp" "../tests/framegraph/DrawingTests/Test_RawDraw1.cpp" "../tests/framegraph/DrawingTests/Test_RayTracingDebugger1.cpp" "../tests/framegraph/DrawingTests/Test_ReadAttachment1.cpp" "../tests/framegraph/DrawingTests/Test_ShaderDebugger1.cpp" "../tests/framegraph/DrawingTests/Test_ShaderDebugger2.cpp" "../tests/framegraph/DrawingTests/Test_ShadingRate1.cpp" "../tests/framegraph/DrawingTests/Test_TraceRays1.cpp" "../tests/framegraph/DrawingTests/Test_TraceRays2.cpp" "../tests/framegraph/DrawingTests/Test_TraceRays3.cpp" )
	source_group( "" FILES "../tests/framegraph/FGApp.cpp" "../tests/framegraph/FGApp.h" "../tests/framegraph/main.cpp" )
	source_group( "ImplTests" FILES "../tests/framegraph/ImplTests/ImplTest_CacheOverflow1.cpp" "../tests/framegraph/ImplTests/ImplTest_Multithreading1.cpp" "../tests/framegraph/ImplTests/ImplTest_Multithreading2.cpp" "../tests/framegraph/ImplTests/ImplTest_Multithreading3.cpp" "../tests/framegraph/ImplTests/ImplTest_Multithreading4.cpp" "../tests/framegraph/ImplTests/ImplTest_Scene1.cpp" )
	set_property( TARGET "Tests.FrameGraph" PROPERTY FOLDER "Tests" )
	target_include_directories( "Tests.FrameGraph" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan/CommandBuffer" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan/Buffer" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/.." )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan/Memory" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan/RayTracing" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan/Swapchain" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan/Instance" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan/RenderPass" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan/Descriptors" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan/Pipeline" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan/Image" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan/Debugger" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../extensions" )
	target_include_directories( "Tests.FrameGraph" PRIVATE "../tests/framegraph/../../framegraph/Vulkan/Utils" )
	target_link_libraries( "Tests.FrameGraph" "FrameGraph" )
	if (${FG_ENABLE_GRAPHVIZ})
		target_link_libraries( "Tests.FrameGraph" "GraphViz" )
	endif()
	if (${FG_ENABLE_LODEPNG})
		target_link_libraries( "Tests.FrameGraph" "lodepng" )
	endif()
	target_link_libraries( "Tests.FrameGraph" "PipelineCompiler" )
	target_link_libraries( "Tests.FrameGraph" "Framework" )
	target_compile_definitions( "Tests.FrameGraph" PUBLIC "FG_TEST_GRAPHS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/../tests/framegraph/Graphs\"" )
	target_compile_definitions( "Tests.FrameGraph" PUBLIC "FG_TEST_DUMPS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/../tests/framegraph/Dumps\"" )
	# compiler
	if (PROJECTS_SHARED_CXX_FLAGS_DEBUG)
		target_compile_options( "Tests.FrameGraph" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	endif()
	if (PROJECTS_SHARED_CXX_FLAGS_RELEASE)
		target_compile_options( "Tests.FrameGraph" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	endif()
	if (PROJECTS_SHARED_DEFINES_RELEASE)
		target_compile_definitions( "Tests.FrameGraph" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	endif()
	if (PROJECTS_SHARED_DEFINES_DEBUG)
		target_compile_definitions( "Tests.FrameGraph" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	endif()
	if (PROJECTS_SHARED_DEFINES_PROFILE)
		target_compile_definitions( "Tests.FrameGraph" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	endif()
	if (PROJECTS_SHARED_LINKER_FLAGS_PROFILE)
		set_target_properties( "Tests.FrameGraph" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	endif()
	if (PROJECTS_SHARED_LINKER_FLAGS_DEBUG)
		set_target_properties( "Tests.FrameGraph" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	endif()
	if (PROJECTS_SHARED_LINKER_FLAGS_RELEASE)
		set_target_properties( "Tests.FrameGraph" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	endif()
	if (PROJECTS_SHARED_CXX_FLAGS_PROFILE)
		target_compile_options( "Tests.FrameGraph" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	endif()
	set_target_properties( "Tests.FrameGraph" PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )
	
	
endif()


add_test( NAME "Tests.FrameGraph" COMMAND "Tests.FrameGraph" )


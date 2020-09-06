
file( GLOB  SHARED_SOURCES "Public/*.*" "Shared/*.*" "*.*" )

if (TARGET "VulkanLoader")
	file( GLOB_RECURSE  VULKAN_SOURCES "Vulkan/*.*" )
endif ()

add_library( "FrameGraph" STATIC ${SHARED_SOURCES} ${VULKAN_SOURCES} ${REMOTE_SOURCES} )

source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SHARED_SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${VULKAN_SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${REMOTE_SOURCES} )
set_property( TARGET "FrameGraph" PROPERTY FOLDER "" )

target_include_directories( "FrameGraph" PUBLIC "." )

if (TARGET "VulkanLoader")
	target_include_directories( "FrameGraph" PRIVATE "Vulkan" )
	target_include_directories( "FrameGraph" PRIVATE "Vulkan/Buffer" )
	target_include_directories( "FrameGraph" PRIVATE "Vulkan/CommandBuffer" )
	target_include_directories( "FrameGraph" PRIVATE "Vulkan/Debugger" )
	target_include_directories( "FrameGraph" PRIVATE "Vulkan/Descriptors" )
	target_include_directories( "FrameGraph" PRIVATE "Vulkan/Image" )
	target_include_directories( "FrameGraph" PRIVATE "Vulkan/Instance" )
	target_include_directories( "FrameGraph" PRIVATE "Vulkan/Memory" )
	target_include_directories( "FrameGraph" PRIVATE "Vulkan/Pipeline" )
	target_include_directories( "FrameGraph" PRIVATE "Vulkan/RenderPass" )
	target_include_directories( "FrameGraph" PRIVATE "Vulkan/RayTracing" )
	target_include_directories( "FrameGraph" PRIVATE "Vulkan/Swapchain" )
	target_include_directories( "FrameGraph" PRIVATE "Vulkan/Utils" )
endif ()

target_link_libraries( "FrameGraph" PUBLIC "STL" )

if (TARGET "VulkanLoader")
	target_link_libraries( "FrameGraph" PRIVATE "VulkanLoader" )
endif ()

if (TARGET "VMA-lib")
	target_link_libraries( "FrameGraph" PRIVATE "VMA-lib" )
endif()
#if (${FG_ENABLE_STDALLOC})
#	target_link_libraries( "FrameGraph" PRIVATE "StdAlloc-lib" )
#endif()

# Version
target_compile_definitions( "FrameGraph" PRIVATE "FG_VERSION_STR=\"${PROJECT_VERSION}\"" )
if (DEFINED FG_COMMIT_HASH)
	target_compile_definitions( "FrameGraph" PRIVATE "FG_COMMIT_HASH=\"${FG_COMMIT_HASH}\"" )
endif()

install( TARGETS "FrameGraph" ARCHIVE DESTINATION "libs/$<CONFIG>" )
install( DIRECTORY "Public" DESTINATION "include/FrameGraph" FILES_MATCHING PATTERN "*.h*" )
install( DIRECTORY "Shared" DESTINATION "include/FrameGraph" FILES_MATCHING PATTERN "*.h*" )
install( FILES "FG.h" DESTINATION "include/FrameGraph" )

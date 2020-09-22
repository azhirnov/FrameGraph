# attaches Vulkan Memory Allocator headers (MIT license)

set( VMA_DST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src" )

add_library( "VMA-lib" INTERFACE )
target_include_directories( "VMA-lib" INTERFACE "${VMA_DST_PATH}" )
target_compile_definitions( "VMA-lib" INTERFACE "FG_ENABLE_VULKAN_MEMORY_ALLOCATOR" )

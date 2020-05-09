# find or download Vulkan Memory Allocator

if (${FG_ENABLE_VMA})
	set( FG_EXTERNAL_VMA_PATH "" CACHE PATH "path to VulkanMemoryAllocator source" )
	mark_as_advanced( FG_EXTERNAL_VMA_PATH )

	# reset to default
	if (NOT EXISTS "${FG_EXTERNAL_VMA_PATH}/src/vk_mem_alloc.h")
		message( STATUS "VulkanMemoryAllocator is not found in \"${FG_EXTERNAL_VMA_PATH}\"" )
		set( FG_EXTERNAL_VMA_PATH "${FG_EXTERNALS_PATH}/VulkanMemoryAllocator" CACHE PATH "" FORCE )
	else ()
		message( STATUS "VulkanMemoryAllocator found in \"${FG_EXTERNAL_VMA_PATH}\"" )
	endif ()
	
	# select version
	if (${FG_EXTERNALS_USE_STABLE_VERSIONS})
		set( VMA_TAG "v2.2.0" )
	else ()
		set( VMA_TAG "master" )
	endif ()

	# download
	if (NOT EXISTS "${FG_EXTERNAL_VMA_PATH}/src/vk_mem_alloc.h")
		FetchContent_Declare( ExternalVMA
			GIT_REPOSITORY		https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
			SOURCE_DIR			"${FG_EXTERNAL_VMA_PATH}"
			GIT_TAG				${VMA_TAG}
		)
		
		FetchContent_GetProperties( ExternalVMA )
		if (NOT ExternalVMA_POPULATED)
			message( STATUS "downloading VulkanMemoryAllocator" )
			FetchContent_Populate( ExternalVMA )
		endif ()
	endif ()

	add_library( "VMA-lib" INTERFACE )
	target_include_directories( "VMA-lib" INTERFACE "${FG_EXTERNAL_VMA_PATH}/src" )
	target_compile_definitions( "VMA-lib" INTERFACE "FG_ENABLE_VULKAN_MEMORY_ALLOCATOR" )
endif ()

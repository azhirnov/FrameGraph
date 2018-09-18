# find or download Vulkan Memory Allocator

if (${FG_ENABLE_VMA})
	set( FG_EXTERNAL_VMA_PATH "" CACHE PATH "path to VulkanMemoryAllocator source" )

	# reset to default
	if (NOT EXISTS ${FG_EXTERNAL_VMA_PATH})
		message( STATUS "VulkanMemoryAllocator is not found in ${FG_EXTERNAL_VMA_PATH}" )
		set( FG_EXTERNAL_VMA_PATH "${FG_EXTERNALS_PATH}/VulkanMemoryAllocator" CACHE PATH "" FORCE )
	endif ()
	
	# download
	if (NOT EXISTS "${FG_EXTERNAL_VMA_PATH}" AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
		FetchContent_Declare( ExternalDownloadVulkanMemoryAllocator
			GIT_REPOSITORY		https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
			SOURCE_DIR			"${FG_EXTERNAL_VMA_PATH}"
			GIT_TAG				master
		)
		
		FetchContent_GetProperties( ExternalDownloadVulkanMemoryAllocator )
		if (NOT ExternalDownloadVulkanMemoryAllocator_POPULATED)
			message( STATUS "downloading VulkanMemoryAllocator" )
			FetchContent_Populate( ExternalDownloadVulkanMemoryAllocator )
		endif ()
	endif ()

	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DFG_ENABLE_VULKAN_MEMORY_ALLOCATOR" )
endif ()

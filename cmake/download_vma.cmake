# find or download Vulkan Memory Allocator

if (${FG_ENABLE_VMA})
	set( FG_EXTERNAL_VMA_PATH "" CACHE PATH "path to VulkanMemoryAllocator source" )

	# reset to default
	if (NOT EXISTS ${FG_EXTERNAL_VMA_PATH})
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
	if (NOT EXISTS "${FG_EXTERNAL_VMA_PATH}" AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
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

	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "FG_ENABLE_VULKAN_MEMORY_ALLOCATOR" )
endif ()

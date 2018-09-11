# Find or download Vulkan headers

if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
	find_package( Vulkan )
endif ()


if (NOT Vulkan_FOUND AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
	FetchContent_Declare( ExternalDownloadVulkan
		GIT_REPOSITORY		https://github.com/KhronosGroup/Vulkan-Headers.git
		GIT_TAG				master
		SOURCE_DIR			"${FG_DOWNLOAD_PATH}/Vulkan-Headers"
	)
	
	FetchContent_GetProperties( ExternalDownloadVulkan )
	if (NOT ExternalDownloadVulkan_POPULATED)
		FetchContent_Populate( ExternalDownloadVulkan )
	endif ()

	set( Vulkan_INCLUDE_DIRS "${FG_DOWNLOAD_PATH}/Vulkan-Headers/include" )
	set( Vulkan_FOUND TRUE )
endif ()


if (NOT Vulkan_FOUND)
	if (EXISTS "${FG_EXTERNALS_PATH}/vulkan/vulkan_core.h")
		set( Vulkan_INCLUDE_DIRS "${FG_EXTERNALS_PATH}" )
		set( Vulkan_FOUND TRUE )

	elseif (EXISTS "${FG_EXTERNALS_PATH}/vulkan/include/vulkan/vulkan_core.h")
		set( Vulkan_INCLUDE_DIRS "${FG_EXTERNALS_PATH}/vulkan/include" )
		set( Vulkan_FOUND TRUE )

	elseif (EXISTS "${FG_EXTERNALS_PATH}/Vulkan-Headers/include/vulkan/vulkan_core.h")
		set( Vulkan_INCLUDE_DIRS "${FG_EXTERNALS_PATH}/Vulkan-Headers/include" )
		set( Vulkan_FOUND TRUE )
	endif ()
endif ()


if (NOT Vulkan_FOUND)
	message( FATAL_ERROR "Vulkan headers is not found! Install VulkanSDK or download from https://github.com/KhronosGroup/Vulkan-Headers" )
else ()
	message( STATUS "Vulkan_LIBRARIES: ${Vulkan_LIBRARIES}" )
	message( STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}" )
endif ()


set( Vulkan_LIBRARIES "${Vulkan_LIBRARIES}" CACHE INTERNAL "" FORCE )
set( Vulkan_INCLUDE_DIRS "${Vulkan_INCLUDE_DIRS}" CACHE INTERNAL "" FORCE )



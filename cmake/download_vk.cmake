# Find or download Vulkan headers

if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
#	find_package( Vulkan )
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


if (NOT Vulkan_FOUND AND NOT CMAKE_VERSION VERSION_LESS 3.11.0)
	FetchContent_Declare( ExternalVulkanHeaders
		GIT_REPOSITORY		https://github.com/KhronosGroup/Vulkan-Headers.git
		GIT_TAG				master
		SOURCE_DIR			"${FG_EXTERNALS_PATH}/Vulkan-Headers"
	)
	
	FetchContent_GetProperties( ExternalVulkanHeaders )
	if (NOT ExternalVulkanHeaders_POPULATED)
		message( STATUS "downloading Vulkan-Headers" )
		FetchContent_Populate( ExternalVulkanHeaders )
	endif ()

	set( Vulkan_INCLUDE_DIRS "${FG_EXTERNALS_PATH}/Vulkan-Headers/include" )
	set( Vulkan_FOUND TRUE )
endif ()


if (NOT Vulkan_FOUND)
	message( FATAL_ERROR "Vulkan headers is not found! Install VulkanSDK or download from https://github.com/KhronosGroup/Vulkan-Headers" )
else ()
	#message( STATUS "Vulkan_LIBRARIES: ${Vulkan_LIBRARIES}" )
	message( STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}" )
endif ()


#set( Vulkan_LIBRARIES "${Vulkan_LIBRARIES}" CACHE INTERNAL "" FORCE )
set( Vulkan_INCLUDE_DIRS "${Vulkan_INCLUDE_DIRS}" CACHE INTERNAL "" FORCE )

if (FALSE)
	if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "VK_USE_PLATFORM_XCB_KHR=1" "VK_USE_PLATFORM_XLIB_KHR=1" )	# TODO

	elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "VK_USE_PLATFORM_ANDROID_KHR=1" )

	elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "VK_USE_PLATFORM_MACOS_MVK=1" )

	elseif (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "VK_USE_PLATFORM_IOS_MVK=1" )

	elseif (WIN32 OR ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "VK_USE_PLATFORM_WIN32_KHR=1" )
		set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "NOMINMAX" "NOMCX" "NOIME" "NOSERVICE" "WIN32_LEAN_AND_MEAN" )
	endif ()
endif ()


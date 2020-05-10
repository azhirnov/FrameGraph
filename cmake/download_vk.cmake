# Find or download Vulkan headers

if (${FG_USE_VULKAN_SDK})
	find_package( Vulkan )
	if (Vulkan_FOUND)
		message( STATUS "found VulkanSDK, warning: current headers may be incompatible with FrameGraph" )
	endif ()
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

# select version
set( VULKAN_HEADERS_TAG "v1.2.132" )

# download
if (NOT Vulkan_FOUND)
	FetchContent_Declare( ExternalVulkanHeaders
		GIT_REPOSITORY		https://github.com/KhronosGroup/Vulkan-Headers.git
		GIT_TAG				${VULKAN_HEADERS_TAG}
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
endif ()

set( FG_VULKAN_DEFINITIONS "FG_ENABLE_VULKAN" )

if (FALSE)
	if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
		set( FG_VULKAN_DEFINITIONS "${FG_VULKAN_DEFINITIONS}" "VK_USE_PLATFORM_XCB_KHR=1" "VK_USE_PLATFORM_XLIB_KHR=1" )	# TODO

	elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
		set( FG_VULKAN_DEFINITIONS "${FG_VULKAN_DEFINITIONS}" "-VK_USE_PLATFORM_ANDROID_KHR=1" )

	elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
		set( FG_VULKAN_DEFINITIONS "${FG_VULKAN_DEFINITIONS}" "VK_USE_PLATFORM_MACOS_MVK=1" )

	elseif (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
		set( FG_VULKAN_DEFINITIONS "${FG_VULKAN_DEFINITIONS}" "VK_USE_PLATFORM_IOS_MVK=1" )

	elseif (WIN32 OR ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
		set( FG_VULKAN_DEFINITIONS "${FG_VULKAN_DEFINITIONS}" "VK_USE_PLATFORM_WIN32_KHR=1" )
		set( FG_VULKAN_DEFINITIONS "${FG_VULKAN_DEFINITIONS}" "NOMINMAX" "NOMCX" "NOIME" "NOSERVICE" "WIN32_LEAN_AND_MEAN" )
	endif ()
endif ()


add_library( "Vulkan-lib" INTERFACE )
target_include_directories( "Vulkan-lib" INTERFACE "${Vulkan_INCLUDE_DIRS}" )
target_compile_definitions( "Vulkan-lib" INTERFACE "${FG_VULKAN_DEFINITIONS}" )

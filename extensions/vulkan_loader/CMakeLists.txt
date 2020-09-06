if (NOT (${FG_VULKAN_VERSION} STREQUAL "0"))
	file( GLOB_RECURSE SOURCES "*.*" )
	add_library( "VulkanLoader" STATIC ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	target_include_directories( "VulkanLoader" PUBLIC ".." )
	set_property( TARGET "VulkanLoader" PROPERTY FOLDER "Extensions" )
	target_link_libraries( "VulkanLoader" PUBLIC "STL" )

	if (${FG_VULKAN_VERSION} STREQUAL "100")
		# tag 'sdk-1.1.82.0' without Vulkan 1.1 functions
		target_include_directories( "VulkanLoader" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/1.0" )
	elseif (${FG_VULKAN_VERSION} STREQUAL "110")
		# tag 'sdk-1.1.130.0'
		target_include_directories( "VulkanLoader" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/1.1" )
	elseif (${FG_VULKAN_VERSION} STREQUAL "120")
		# tag 'sdk-1.2.148.0'
		target_include_directories( "VulkanLoader" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/1.2" )
	else ()
		message( FATAL_ERROR "unknown Vulkan version: ${FG_VULKAN_VERSION}" )
	endif ()

	if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
		target_compile_definitions( "VulkanLoader" PUBLIC VK_USE_PLATFORM_ANDROID_KHR=1 )
	endif ()

	target_compile_definitions( "VulkanLoader" PUBLIC FG_ENABLE_VULKAN )
	target_compile_definitions( "VulkanLoader" PUBLIC FG_VULKAN_TARGET_VERSION=${FG_VULKAN_VERSION} )

	install( TARGETS "VulkanLoader" ARCHIVE DESTINATION "libs/$<CONFIG>" )
	install( DIRECTORY "." DESTINATION "include/VulkanLoader" FILES_MATCHING PATTERN "*.h*" )
endif ()

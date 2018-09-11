# Check and include existing externals


# Vulkan Memory Allocator
if (${FG_ENABLE_VMA})
	if (NOT EXISTS "${FG_EXTERNALS_PATH}/VulkanMemoryAllocator")
		message( FATAL_ERROR "'VulkanMemoryAllocator' library not found!" )
	endif ()

	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DFG_ENABLE_VULKAN_MEMORY_ALLOCATOR" )
endif ()


# GLSLANG
if (${FG_ENABLE_GLSLANG})
	if (NOT EXISTS "${FG_EXTERNALS_PATH}/glslang")
		message( FATAL_ERROR "'glslang' library not found!" )
	endif ()

	add_subdirectory( "${FG_EXTERNALS_PATH}/glslang" "glslang" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DFG_ENABLE_GLSLANG" )
endif ()


# GLFW
if (${FG_ENABLE_GLFW})
	if (NOT EXISTS "${FG_EXTERNALS_PATH}/glfw")
		message( FATAL_ERROR "'glfw' library not found!" )
	endif ()

	add_subdirectory( "${FG_EXTERNALS_PATH}/glfw" "glfw" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DFG_ENABLE_GLFW" )
endif ()


# SDL2
if (${FG_ENABLE_SDL2})
	if (NOT EXISTS "${FG_EXTERNALS_PATH}/SDL2")
		message( FATAL_ERROR "'SDL2' library not found!" )
	endif ()

	add_subdirectory( "${FG_EXTERNALS_PATH}/SDL2" "SDL2" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DFG_ENABLE_SDL2" )
endif ()


# LodePNG
if (${FG_ENABLE_LODEPNG})
	if (NOT EXISTS "${FG_EXTERNALS_PATH}/lodepng")
		message( FATAL_ERROR "'lodepng' library not found!" )
	endif ()

	add_subdirectory( "${FG_EXTERNALS_PATH}/lodepng" "lodepng" )
	set( FG_GLOBAL_DEFINITIONS "${FG_GLOBAL_DEFINITIONS}" "-DFG_ENABLE_LODEPNG" )
endif ()

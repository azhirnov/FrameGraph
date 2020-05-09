
file( GLOB_RECURSE SOURCES "*.*" )
add_library( "Framework" STATIC ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
target_include_directories( "Framework" PUBLIC ".." )
set_property( TARGET "Framework" PROPERTY FOLDER "Extensions" )

target_link_libraries( "Framework" PUBLIC "VulkanLoader" )

if (${FG_ENABLE_GLFW})
	target_link_libraries( "Framework" PUBLIC "GLFW-lib" )
endif()
if (${FG_ENABLE_SDL2})
	target_link_libraries( "Framework" PUBLIC "SDL2-lib" )
endif()
if (${FG_ENABLE_OPENVR})
	target_link_libraries( "Framework" PUBLIC "OpenVR-lib" )
endif()

install( TARGETS "Framework" ARCHIVE DESTINATION "libs/$<CONFIG>" )
install( DIRECTORY "." DESTINATION "include/Framework" FILES_MATCHING PATTERN "*.h*" )


set( CMAKE_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
file( GLOB_RECURSE CMAKE_SOURCES "${CMAKE_FOLDER}/*.*" )

add_library( "ProjectTemplate" STATIC EXCLUDE_FROM_ALL ${CMAKE_SOURCES} )
set_property( TARGET "ProjectTemplate" PROPERTY FOLDER "Utils" )
source_group( TREE "${CMAKE_FOLDER}/.." FILES ${CMAKE_SOURCES} )

target_compile_definitions( "ProjectTemplate" PUBLIC ${FG_COMPILER_DEFINITIONS} )
target_link_libraries( "ProjectTemplate" PUBLIC "${FG_LINK_LIBRARIES}" )
	
# Debug
if (PROJECTS_SHARED_CXX_FLAGS_DEBUG)
	target_compile_options( "ProjectTemplate" PUBLIC $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
endif ()
if (PROJECTS_SHARED_DEFINES_DEBUG)
	target_compile_definitions( "ProjectTemplate" PUBLIC $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
endif ()
if (PROJECTS_SHARED_LINKER_FLAGS_DEBUG)
	set_target_properties( "ProjectTemplate" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
endif ()

# Release
if (PROJECTS_SHARED_CXX_FLAGS_RELEASE)
	target_compile_options( "ProjectTemplate" PUBLIC $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
endif ()
if (PROJECTS_SHARED_DEFINES_RELEASE)
	target_compile_definitions( "ProjectTemplate" PUBLIC $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
endif ()
if (PROJECTS_SHARED_LINKER_FLAGS_RELEASE)
	set_target_properties( "ProjectTemplate" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
endif ()

# Profile
if (PROJECTS_SHARED_DEFINES_PROFILE)
	target_compile_definitions( "ProjectTemplate" PUBLIC $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
endif ()
if (PROJECTS_SHARED_LINKER_FLAGS_PROFILE)
	set_target_properties( "ProjectTemplate" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
endif ()
if (PROJECTS_SHARED_CXX_FLAGS_PROFILE)
	target_compile_options( "ProjectTemplate" PUBLIC $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
endif ()

set_target_properties( "ProjectTemplate" PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES )
target_compile_features( "ProjectTemplate" PUBLIC cxx_std_17 )

if (${FG_CI_BUILD})
	target_compile_definitions( "ProjectTemplate" PUBLIC "FG_CI_BUILD" )
endif ()

if (${FG_ENABLE_MEMLEAK_CHECKS})
	target_compile_definitions( "ProjectTemplate" PUBLIC "FG_ENABLE_MEMLEAK_CHECKS" )
endif ()

if (${FG_ALLOW_GPL})
	target_compile_definitions( "ProjectTemplate" PUBLIC "FG_ALLOW_GPL" )
endif ()

if (${FG_NO_EXCEPTIONS})
	target_compile_definitions( "ProjectTemplate" PUBLIC "FG_NO_EXCEPTIONS" )
endif ()

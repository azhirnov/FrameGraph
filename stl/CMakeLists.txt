# auto generated file
cmake_minimum_required (VERSION 3.6.0)

#==================================================================================================
# project: STL
#==================================================================================================
set( SOURCES 
	"include/ArrayView.h"
	"include/Bytes.h"
	"include/Cast.h"
	"include/Color.h"
	"include/Common.h"
	"include/DefaultType.h"
	"include/Defines.h"
	"include/EnumUtils.h"
	"include/FixedArray.h"
	"include/FixedMap.h"
	"include/Hash.h"
	"include/Log.h"
	"include/Math.h"
	"include/MemUtils.h"
	"include/PoolAllocator.h"
	"include/Rectangle.h"
	"include/Singleton.h"
	"include/StaticString.h"
	"include/StringUtils.h"
	"include/Union.h"
	"include/Vec.h"
	"source/Log.cpp"
	"CMakeLists.txt" )
add_library( "STL" STATIC ${SOURCES} )
source_group( "include" FILES "include/ArrayView.h" "include/Bytes.h" "include/Cast.h" "include/Color.h" "include/Common.h" "include/DefaultType.h" "include/Defines.h" "include/EnumUtils.h" "include/FixedArray.h" "include/FixedMap.h" "include/Hash.h" "include/Log.h" "include/Math.h" "include/MemUtils.h" "include/PoolAllocator.h" "include/Rectangle.h" "include/Singleton.h" "include/StaticString.h" "include/StringUtils.h" "include/Union.h" "include/Vec.h" )
source_group( "source" FILES "source/Log.cpp" )
source_group( "" FILES "CMakeLists.txt" )
target_include_directories( "STL" PUBLIC ".." )
target_include_directories( "STL" PUBLIC "${FG_EXTERNALS_PATH}" )
target_include_directories( "STL" PRIVATE "" )
if (UNIX)
	target_link_libraries( "STL" "dl;pthread" )
endif()
# compiler
target_compile_options( "STL" PRIVATE $<$<CONFIG:DebugAnalyze>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUGANALYZE}> )
target_compile_definitions( "STL" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
target_compile_definitions( "STL" PRIVATE $<$<CONFIG:DebugAnalyze>: ${PROJECTS_SHARED_DEFINES_DEBUGANALYZE}> )
set_target_properties( "STL" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
target_compile_definitions( "STL" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
target_compile_options( "STL" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
set_target_properties( "STL" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
set_target_properties( "STL" PROPERTIES LINK_FLAGS_DEBUGANALYZE ${PROJECTS_SHARED_LINKER_FLAGS_DEBUGANALYZE} )
target_compile_options( "STL" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
set_target_properties( "STL" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
target_compile_options( "STL" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
target_compile_definitions( "STL" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )

#-----------------------------------
add_definitions( ${FG_GLOBAL_DEFINITIONS} )

#-----------------------------------
install( TARGETS "STL" ARCHIVE DESTINATION "lib" )


#==================================================================================================
# project: Tests.STL
#==================================================================================================
if (${FG_ENABLE_TESTS})
	set( SOURCES 
		"../tests/stl/main.cpp"
		"../tests/stl/UnitTestCommon.h"
		"../tests/stl/UnitTest_FixedArray.cpp"
		"../tests/stl/UnitTest_StaticString.cpp" )
	if (DEFINED ANDROID)
		add_library( "Tests.STL" SHARED ${SOURCES} )
	else()
		add_executable( "Tests.STL" ${SOURCES} )
	endif()
	source_group( "" FILES "../tests/stl/main.cpp" "../tests/stl/UnitTestCommon.h" "../tests/stl/UnitTest_FixedArray.cpp" "../tests/stl/UnitTest_StaticString.cpp" )
	set_property( TARGET "Tests.STL" PROPERTY FOLDER "Tests" )
	target_include_directories( "Tests.STL" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "Tests.STL" PRIVATE "../tests/.." )
	target_link_libraries( "Tests.STL" "STL" )
	# compiler
	target_compile_options( "Tests.STL" PRIVATE $<$<CONFIG:DebugAnalyze>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUGANALYZE}> )
	target_compile_definitions( "Tests.STL" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	target_compile_definitions( "Tests.STL" PRIVATE $<$<CONFIG:DebugAnalyze>: ${PROJECTS_SHARED_DEFINES_DEBUGANALYZE}> )
	set_target_properties( "Tests.STL" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Tests.STL" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Tests.STL" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Tests.STL" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	set_target_properties( "Tests.STL" PROPERTIES LINK_FLAGS_DEBUGANALYZE ${PROJECTS_SHARED_LINKER_FLAGS_DEBUGANALYZE} )
	target_compile_options( "Tests.STL" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Tests.STL" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Tests.STL" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Tests.STL" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


add_test( NAME "Tests.STL" COMMAND "Tests.STL" )


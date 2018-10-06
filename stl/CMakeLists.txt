# auto generated file
cmake_minimum_required (VERSION 3.6.0)

#==================================================================================================
# project: STL
#==================================================================================================
set( SOURCES 
	"CompileTime/DefaultType.h"
	"CompileTime/TypeList.h"
	"CompileTime/TypeTraits.h"
	"Math/Bytes.h"
	"Math/Color.h"
	"Math/Math.h"
	"Math/Rectangle.h"
	"Math/Vec.h"
	"Containers/AlignedBuffer.h"
	"Containers/ArrayView.h"
	"Containers/FixedArray.h"
	"Containers/FixedMap.h"
	"Containers/PoolAllocator.h"
	"Containers/Singleton.h"
	"Containers/StaticString.h"
	"Containers/Union.h"
	"Algorithms/Cast.h"
	"Algorithms/EnumUtils.h"
	"Algorithms/Hash.h"
	"Algorithms/MemUtils.h"
	"Algorithms/StringUtils.h"
	"Log/Log.cpp"
	"Log/Log.h"
	"Log/TimeProfiler.h"
	"CMakeLists.txt"
	"Common.h"
	"Defines.h"
	"Stream/BufferedStream.h"
	"Stream/FileStream.cpp"
	"Stream/FileStream.h"
	"Stream/MemStream.h"
	"Stream/Stream.cpp"
	"Stream/Stream.h" )
add_library( "STL" STATIC ${SOURCES} )
source_group( "CompileTime" FILES "CompileTime/DefaultType.h" "CompileTime/TypeList.h" "CompileTime/TypeTraits.h" )
source_group( "Math" FILES "Math/Bytes.h" "Math/Color.h" "Math/Math.h" "Math/Rectangle.h" "Math/Vec.h" )
source_group( "Containers" FILES "Containers/AlignedBuffer.h" "Containers/ArrayView.h" "Containers/FixedArray.h" "Containers/FixedMap.h" "Containers/PoolAllocator.h" "Containers/Singleton.h" "Containers/StaticString.h" "Containers/Union.h" )
source_group( "Algorithms" FILES "Algorithms/Cast.h" "Algorithms/EnumUtils.h" "Algorithms/Hash.h" "Algorithms/MemUtils.h" "Algorithms/StringUtils.h" )
source_group( "Log" FILES "Log/Log.cpp" "Log/Log.h" "Log/TimeProfiler.h" )
source_group( "" FILES "CMakeLists.txt" "Common.h" "Defines.h" )
source_group( "Stream" FILES "Stream/BufferedStream.h" "Stream/FileStream.cpp" "Stream/FileStream.h" "Stream/MemStream.h" "Stream/Stream.cpp" "Stream/Stream.h" )
target_include_directories( "STL" PUBLIC ".." )
target_include_directories( "STL" PUBLIC "${FG_EXTERNALS_PATH}" )
target_include_directories( "STL" PRIVATE "" )
if (UNIX)
	target_link_libraries( "STL" "dl" )
endif()
target_compile_definitions( "STL" PUBLIC "${FG_GLOBAL_DEFINITIONS}" )
if (UNIX)
	target_compile_definitions( "STL" PUBLIC "_LARGEFILE_SOURCE" )
endif()
if (UNIX)
	target_compile_definitions( "STL" PUBLIC "_LARGE_FILES" )
endif()
if (UNIX)
	target_compile_definitions( "STL" PUBLIC "_FILE_OFFSET_BITS=64" )
endif()
# compiler
target_compile_definitions( "STL" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
set_target_properties( "STL" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
target_compile_definitions( "STL" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
target_compile_options( "STL" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
set_target_properties( "STL" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
target_compile_options( "STL" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
set_target_properties( "STL" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
target_compile_options( "STL" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
target_compile_definitions( "STL" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )

install( TARGETS "STL" ARCHIVE DESTINATION "lib" )


#==================================================================================================
# project: Tests.STL
#==================================================================================================
if (${FG_ENABLE_TESTS})
	set( SOURCES 
		"../tests/stl/main.cpp"
		"../tests/stl/UnitTestCommon.h"
		"../tests/stl/UnitTest_FixedArray.cpp"
		"../tests/stl/UnitTest_FixedMap.cpp"
		"../tests/stl/UnitTest_Math.cpp"
		"../tests/stl/UnitTest_StaticString.cpp"
		"../tests/stl/UnitTest_ToString.cpp" )
	if (DEFINED ANDROID)
		add_library( "Tests.STL" SHARED ${SOURCES} )
	else()
		add_executable( "Tests.STL" ${SOURCES} )
	endif()
	source_group( "" FILES "../tests/stl/main.cpp" "../tests/stl/UnitTestCommon.h" "../tests/stl/UnitTest_FixedArray.cpp" "../tests/stl/UnitTest_FixedMap.cpp" "../tests/stl/UnitTest_Math.cpp" "../tests/stl/UnitTest_StaticString.cpp" "../tests/stl/UnitTest_ToString.cpp" )
	set_property( TARGET "Tests.STL" PROPERTY FOLDER "Tests" )
	target_include_directories( "Tests.STL" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "Tests.STL" PRIVATE "../tests/.." )
	target_link_libraries( "Tests.STL" "STL" )
	# compiler
	target_compile_definitions( "Tests.STL" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Tests.STL" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Tests.STL" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Tests.STL" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Tests.STL" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Tests.STL" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Tests.STL" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Tests.STL" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Tests.STL" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()


add_test( NAME "Tests.STL" COMMAND "Tests.STL" )


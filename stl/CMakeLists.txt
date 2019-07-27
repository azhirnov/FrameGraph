# auto generated file
cmake_minimum_required (VERSION 3.6.0)

#==================================================================================================
# project: STL
#==================================================================================================
set( SOURCES 
	"CompileTime/DefaultType.h"
	"CompileTime/Hash.h"
	"CompileTime/Math.h"
	"CompileTime/TypeList.h"
	"CompileTime/TypeTraits.h"
	"CompileTime/UMax.h"
	"Platforms/ThreadName.cpp"
	"Platforms/ThreadName.h"
	"Platforms/WindowsHeader.h"
	"Math/BitMath.h"
	"Math/Bytes.h"
	"Math/Color.h"
	"Math/Math.h"
	"Math/Matrix.h"
	"Math/Rectangle.h"
	"Math/Vec.h"
	"Containers/AnyTypeRef.h"
	"Containers/Appendable.h"
	"Containers/ArrayView.h"
	"Containers/BitTree.h"
	"Containers/CachedIndexedPool.h"
	"Containers/ChunkedIndexedPool.h"
	"Containers/FixedArray.h"
	"Containers/FixedMap.h"
	"Containers/FixedTupleArray.h"
	"Containers/InPlace.h"
	"Containers/Iterators.h"
	"Containers/Optional.h"
	"Containers/Ptr.h"
	"Containers/Singleton.h"
	"Containers/StaticString.h"
	"Containers/StringView.h"
	"Containers/StringViewFwd.h"
	"Containers/StructView.h"
	"Containers/Union.h"
	"Containers/UntypedStorage.h"
	"Algorithms/ArrayUtils.h"
	"Algorithms/Cast.h"
	"Algorithms/EnumUtils.h"
	"Algorithms/Hash.h"
	"Algorithms/StringParser.cpp"
	"Algorithms/StringParser.h"
	"Algorithms/StringUtils.h"
	"Log/Log.cpp"
	"Log/Log.h"
	"Log/TimeProfiler.h"
	"Memory/LinearAllocator.h"
	"Memory/MemUtils.h"
	"Memory/MemWriter.h"
	"Memory/UntypedAllocator.h"
	"CMakeLists.txt"
	"Common.h"
	"Config.h"
	"Defines.h"
	"ThreadSafe/AtomicCounter.h"
	"ThreadSafe/AtomicPtr.h"
	"ThreadSafe/Barrier.cpp"
	"ThreadSafe/Barrier.h"
	"ThreadSafe/DataRaceCheck.h"
	"ThreadSafe/DummyLock.h"
	"ThreadSafe/LfDoubleBuffer.h"
	"ThreadSafe/LfFixedList.h"
	"ThreadSafe/LfFixedStack.h"
	"ThreadSafe/LfIndexedPool.h"
	"ThreadSafe/SpinLock.h"
	"Stream/BufferedStream.h"
	"Stream/FileStream.cpp"
	"Stream/FileStream.h"
	"Stream/MemStream.h"
	"Stream/Stream.cpp"
	"Stream/Stream.h" )
add_library( "STL" STATIC ${SOURCES} )
source_group( "CompileTime" FILES "CompileTime/DefaultType.h" "CompileTime/Hash.h" "CompileTime/Math.h" "CompileTime/TypeList.h" "CompileTime/TypeTraits.h" "CompileTime/UMax.h" )
source_group( "Platforms" FILES "Platforms/ThreadName.cpp" "Platforms/ThreadName.h" "Platforms/WindowsHeader.h" )
source_group( "Math" FILES "Math/BitMath.h" "Math/Bytes.h" "Math/Color.h" "Math/Math.h" "Math/Matrix.h" "Math/Rectangle.h" "Math/Vec.h" )
source_group( "Containers" FILES "Containers/AnyTypeRef.h" "Containers/Appendable.h" "Containers/ArrayView.h" "Containers/BitTree.h" "Containers/CachedIndexedPool.h" "Containers/ChunkedIndexedPool.h" "Containers/FixedArray.h" "Containers/FixedMap.h" "Containers/FixedTupleArray.h" "Containers/InPlace.h" "Containers/Iterators.h" "Containers/Optional.h" "Containers/Ptr.h" "Containers/Singleton.h" "Containers/StaticString.h" "Containers/StringView.h" "Containers/StringViewFwd.h" "Containers/StructView.h" "Containers/Union.h" "Containers/UntypedStorage.h" )
source_group( "Algorithms" FILES "Algorithms/ArrayUtils.h" "Algorithms/Cast.h" "Algorithms/EnumUtils.h" "Algorithms/Hash.h" "Algorithms/StringParser.cpp" "Algorithms/StringParser.h" "Algorithms/StringUtils.h" )
source_group( "Log" FILES "Log/Log.cpp" "Log/Log.h" "Log/TimeProfiler.h" )
source_group( "Memory" FILES "Memory/LinearAllocator.h" "Memory/MemUtils.h" "Memory/MemWriter.h" "Memory/UntypedAllocator.h" )
source_group( "" FILES "CMakeLists.txt" "Common.h" "Config.h" "Defines.h" )
source_group( "ThreadSafe" FILES "ThreadSafe/AtomicCounter.h" "ThreadSafe/AtomicPtr.h" "ThreadSafe/Barrier.cpp" "ThreadSafe/Barrier.h" "ThreadSafe/DataRaceCheck.h" "ThreadSafe/DummyLock.h" "ThreadSafe/LfDoubleBuffer.h" "ThreadSafe/LfFixedList.h" "ThreadSafe/LfFixedStack.h" "ThreadSafe/LfIndexedPool.h" "ThreadSafe/SpinLock.h" )
source_group( "Stream" FILES "Stream/BufferedStream.h" "Stream/FileStream.cpp" "Stream/FileStream.h" "Stream/MemStream.h" "Stream/Stream.cpp" "Stream/Stream.h" )
target_include_directories( "STL" PUBLIC ".." )
target_include_directories( "STL" PUBLIC "${FG_EXTERNALS_PATH}" )
target_include_directories( "STL" PRIVATE "" )
if (UNIX)
	target_link_libraries( "STL" "dl;pthread" )
endif()
target_link_libraries( "STL" "${FG_GLOBAL_LIBRARIES}" )
if (UNIX)
	target_compile_definitions( "STL" PUBLIC _LARGEFILE_SOURCE )
endif()
if (UNIX)
	target_compile_definitions( "STL" PUBLIC _LARGE_FILES )
endif()
target_compile_definitions( "STL" PUBLIC "${FG_GLOBAL_DEFINITIONS}" )
if (UNIX)
	target_compile_definitions( "STL" PUBLIC _FILE_OFFSET_BITS=64 )
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
		"../tests/stl/UnitTest_Array.cpp"
		"../tests/stl/UnitTest_BitTree.cpp"
		"../tests/stl/UnitTest_Color.cpp"
		"../tests/stl/UnitTest_Common.h"
		"../tests/stl/UnitTest_FixedArray.cpp"
		"../tests/stl/UnitTest_FixedMap.cpp"
		"../tests/stl/UnitTest_FixedTupleArray.cpp"
		"../tests/stl/UnitTest_IndexedPool.cpp"
		"../tests/stl/UnitTest_LfDoubleBuffer.cpp"
		"../tests/stl/UnitTest_LfFixedStack.cpp"
		"../tests/stl/UnitTest_LfIndexedPool.cpp"
		"../tests/stl/UnitTest_Math.cpp"
		"../tests/stl/UnitTest_Matrix.cpp"
		"../tests/stl/UnitTest_PoolAllocator.cpp"
		"../tests/stl/UnitTest_Rectangle.cpp"
		"../tests/stl/UnitTest_StaticString.cpp"
		"../tests/stl/UnitTest_StringParser.cpp"
		"../tests/stl/UnitTest_StructView.cpp"
		"../tests/stl/UnitTest_ToString.cpp" )
	if (DEFINED ANDROID)
		add_library( "Tests.STL" SHARED ${SOURCES} )
	else()
		add_executable( "Tests.STL" ${SOURCES} )
	endif()
	source_group( "" FILES "../tests/stl/main.cpp" "../tests/stl/UnitTest_Array.cpp" "../tests/stl/UnitTest_BitTree.cpp" "../tests/stl/UnitTest_Color.cpp" "../tests/stl/UnitTest_Common.h" "../tests/stl/UnitTest_FixedArray.cpp" "../tests/stl/UnitTest_FixedMap.cpp" "../tests/stl/UnitTest_FixedTupleArray.cpp" "../tests/stl/UnitTest_IndexedPool.cpp" "../tests/stl/UnitTest_LfDoubleBuffer.cpp" "../tests/stl/UnitTest_LfFixedStack.cpp" "../tests/stl/UnitTest_LfIndexedPool.cpp" "../tests/stl/UnitTest_Math.cpp" "../tests/stl/UnitTest_Matrix.cpp" "../tests/stl/UnitTest_PoolAllocator.cpp" "../tests/stl/UnitTest_Rectangle.cpp" "../tests/stl/UnitTest_StaticString.cpp" "../tests/stl/UnitTest_StringParser.cpp" "../tests/stl/UnitTest_StructView.cpp" "../tests/stl/UnitTest_ToString.cpp" )
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


#==================================================================================================
# project: Tests.Algorithms
#==================================================================================================
if (${FG_ENABLE_TESTS})
	set( SOURCES 
		"../tests/algorithms/CmdBatchSort.cpp"
		"../tests/algorithms/main.cpp"
		"../tests/algorithms/UnitTest_Common.h" )
	if (DEFINED ANDROID)
		add_library( "Tests.Algorithms" SHARED ${SOURCES} )
	else()
		add_executable( "Tests.Algorithms" ${SOURCES} )
	endif()
	source_group( "" FILES "../tests/algorithms/CmdBatchSort.cpp" "../tests/algorithms/main.cpp" "../tests/algorithms/UnitTest_Common.h" )
	set_property( TARGET "Tests.Algorithms" PROPERTY FOLDER "Tests" )
	target_include_directories( "Tests.Algorithms" PUBLIC "${FG_EXTERNALS_PATH}" )
	target_include_directories( "Tests.Algorithms" PRIVATE "../tests/.." )
	target_link_libraries( "Tests.Algorithms" "STL" )
	# compiler
	target_compile_definitions( "Tests.Algorithms" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_DEFINES_DEBUG}> )
	set_target_properties( "Tests.Algorithms" PROPERTIES LINK_FLAGS_RELEASE ${PROJECTS_SHARED_LINKER_FLAGS_RELEASE} )
	target_compile_definitions( "Tests.Algorithms" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_DEFINES_PROFILE}> )
	target_compile_options( "Tests.Algorithms" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_CXX_FLAGS_RELEASE}> )
	set_target_properties( "Tests.Algorithms" PROPERTIES LINK_FLAGS_DEBUG ${PROJECTS_SHARED_LINKER_FLAGS_DEBUG} )
	target_compile_options( "Tests.Algorithms" PRIVATE $<$<CONFIG:Profile>: ${PROJECTS_SHARED_CXX_FLAGS_PROFILE}> )
	set_target_properties( "Tests.Algorithms" PROPERTIES LINK_FLAGS_PROFILE ${PROJECTS_SHARED_LINKER_FLAGS_PROFILE} )
	target_compile_options( "Tests.Algorithms" PRIVATE $<$<CONFIG:Debug>: ${PROJECTS_SHARED_CXX_FLAGS_DEBUG}> )
	target_compile_definitions( "Tests.Algorithms" PRIVATE $<$<CONFIG:Release>: ${PROJECTS_SHARED_DEFINES_RELEASE}> )
	
endif()

add_test( NAME "Tests.STL" COMMAND "Tests.STL" )

add_test( NAME "Tests.Algorithms" COMMAND "Tests.Algorithms" )


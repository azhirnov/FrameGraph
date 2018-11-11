include( CheckCXXSourceCompiles )

if ( COMPILER_MSVC )
	set( CMAKE_REQUIRED_FLAGS "/std:c++latest" )

elseif ( COMPILER_GCC OR COMPILER_CLANG OR COMPILER_CLANG_APPLE OR COMPILER_CLANG_ANDROID )
	set( CMAKE_REQUIRED_FLAGS "-std=c++1z" )
endif ()

set( FG_COMPILER_DEFINITIONS "" )

#------------------------------------------------------------------------------
check_cxx_source_compiles(
	"#include <filesystem>
	namespace fs = std::filesystem;
	int main () {
		return 0;
	}"
	STD_FILESYSTEM_SUPPORTED
)

if (STD_FILESYSTEM_SUPPORTED)
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_STD_FILESYSTEM" )
endif ()

#------------------------------------------------------------------------------
check_cxx_source_compiles(
	"#include <new>
	static constexpr size_t Align = std::hardware_destructive_interference_size;
	int main () {
		return 0;
	}"
	STD_CACHELINESIZE_SUPPORTED
)

if (STD_CACHELINESIZE_SUPPORTED)
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_CACHE_LINE=std::hardware_destructive_interference_size" )
else ()
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_CACHE_LINE=64" ) # TODO
endif ()
#------------------------------------------------------------------------------

set( CMAKE_REQUIRED_FLAGS "" )

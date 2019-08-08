include( CheckCXXSourceCompiles )

if ( ${COMPILER_MSVC} )
	set( CMAKE_REQUIRED_FLAGS "/std:c++latest" )
else()
	set( CMAKE_REQUIRED_FLAGS "-std=c++17" )
endif ()

message( STATUS "Run compiler tests with flags: ${CMAKE_REQUIRED_FLAGS}" )

set( FG_COMPILER_DEFINITIONS "" )
set( FG_LINK_LIBRARIES "" )

#------------------------------------------------------------------------------
check_cxx_source_compiles(
	"#include <string_view>
	int main () {
		std::string_view str{\"1234\"};
		return 0;
	}"
	STD_STRINGVIEW_SUPPORTED )

if (STD_STRINGVIEW_SUPPORTED)
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_STD_STRINGVIEW" )
	set( STD_STRINGVIEW_SUPPORTED ON CACHE INTERNAL "" FORCE )
else()
	set( STD_STRINGVIEW_SUPPORTED OFF CACHE INTERNAL "" FORCE )
endif ()

#------------------------------------------------------------------------------
check_cxx_source_compiles(
	"#include <optional>
	int main () {
		std::optional<int> opt;
		return opt.has_value() ? 0 : 1;
	}"
	STD_OPTIONAL_SUPPORTED )

if (STD_OPTIONAL_SUPPORTED)
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_STD_OPTIONAL" )
	set( STD_OPTIONAL_SUPPORTED ON CACHE INTERNAL "" FORCE )
else()
	set( STD_OPTIONAL_SUPPORTED OFF CACHE INTERNAL "" FORCE )
endif ()

#------------------------------------------------------------------------------
check_cxx_source_compiles(
	"#include <variant>
	int main () {
		std::variant<int, float> var;
		var = 1.0f;
		return 0;
	}"
	STD_VARIANT_SUPPORTED )

if (STD_VARIANT_SUPPORTED)
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_STD_VARIANT" )
	set( STD_VARIANT_SUPPORTED ON CACHE INTERNAL "" FORCE )
else()
	set( STD_VARIANT_SUPPORTED OFF CACHE INTERNAL "" FORCE )
endif ()

#------------------------------------------------------------------------------
check_cxx_source_compiles(
	"#include <filesystem>
	namespace fs = std::filesystem;
	int main () {
		return 0;
	}"
	STD_FILESYSTEM_SUPPORTED )

if (STD_FILESYSTEM_SUPPORTED)
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_STD_FILESYSTEM" )

	if (${COMPILER_CLANG} OR ${COMPILER_CLANG_APPLE} OR ${COMPILER_CLANG_ANDROID} OR ${COMPILER_GCC})
		set( FG_LINK_LIBRARIES "${FG_LINK_LIBRARIES}" "stdc++fs" )
	endif()
	set( STD_FILESYSTEM_SUPPORTED ON CACHE INTERNAL "" FORCE )
else()
	set( STD_FILESYSTEM_SUPPORTED OFF CACHE INTERNAL "" FORCE )
endif ()

#------------------------------------------------------------------------------
check_cxx_source_compiles(
	"#include <new>
	static constexpr size_t Align = std::hardware_destructive_interference_size;
	int main () {
		return 0;
	}"
	STD_CACHELINESIZE_SUPPORTED )

if (STD_CACHELINESIZE_SUPPORTED)
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_CACHE_LINE=std::hardware_destructive_interference_size" )
else ()
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_CACHE_LINE=64" ) # TODO
endif ()

#------------------------------------------------------------------------------
check_cxx_source_compiles(
	"#include <barrier>
	int main () {
		std::barrier  temp;
		return 0;
	}"
	STD_BARRIER_SUPPORTED )

if (STD_BARRIER_SUPPORTED)
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_STD_BARRIER" )
endif ()

#------------------------------------------------------------------------------
check_cxx_source_compiles(
	"#include <functional>
	int main () {
		char buffer[128] = {};
		(void)(std::_Hash_array_representation( reinterpret_cast<const unsigned char*>(buffer), std::size(buffer) ));
		return 0;
	}"
	HAS_HASHFN_HashArrayRepresentation )

if (HAS_HASHFN_HashArrayRepresentation)
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_HAS_HASHFN_HashArrayRepresentation" )
endif ()

#------------------------------------------------------------------------------
check_cxx_source_compiles(
	"#include <functional>
	int main () {
		char buffer[128] = {};
		(void)(std::__murmur2_or_cityhash<size_t>()( buffer, std::size(buffer) ));
		return 0;
	}"
	HAS_HASHFN_Murmur2OrCityhash )

if (HAS_HASHFN_Murmur2OrCityhash)
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_HAS_HASHFN_Murmur2OrCityhash" )
endif ()

#------------------------------------------------------------------------------
check_cxx_source_compiles(
	"#include <functional>
	int main () {
		char buffer[128] = {};
		(void)(std::_Hash_bytes( buffer, std::size(buffer), 0 ));
		return 0;
	}"
	HAS_HASHFN_HashBytes )

if (HAS_HASHFN_HashBytes)
	set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" "FG_HAS_HASHFN_HashBytes" )
endif ()

#------------------------------------------------------------------------------

set( CMAKE_REQUIRED_FLAGS "" )
set( FG_COMPILER_DEFINITIONS "${FG_COMPILER_DEFINITIONS}" CACHE INTERNAL "" FORCE )
set( FG_LINK_LIBRARIES "${FG_LINK_LIBRARIES}" CACHE INTERNAL "" FORCE )

#message( STATUS "Supported features = ${CMAKE_CXX_COMPILE_FEATURES}" )


# auto generated file
cmake_minimum_required (VERSION 3.6.0)


# detect target platform
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set( TARGET_PLATFORM "PLATFORM_LINUX" CACHE INTERNAL "" FORCE )

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	set( TARGET_PLATFORM "PLATFORM_ANDROID" CACHE INTERNAL "" FORCE )

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set( TARGET_PLATFORM "PLATFORM_MACOS" CACHE INTERNAL "" FORCE )

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
	set( TARGET_PLATFORM "PLATFORM_IPHONE" CACHE INTERNAL "" FORCE )

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set( TARGET_PLATFORM "PLATFORM_WINDOWS" CACHE INTERNAL "" FORCE )

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsCE")
	set( TARGET_PLATFORM "PLATFORM_WINDOWSCE" CACHE INTERNAL "" FORCE )

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
	set( TARGET_PLATFORM "PLATFORM_WINDOWS_UWP" CACHE INTERNAL "" FORCE )

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsPhone")
	set( TARGET_PLATFORM "PLATFORM_WINDOWS_PHONE" CACHE INTERNAL "" FORCE )

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "QNX")
	set( TARGET_PLATFORM "PLATFORM_QNX" CACHE INTERNAL "" FORCE )
else ()
	message( FATAL_ERROR "unsupported platform ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}" )
endif ()
message( STATUS "TARGET_PLATFORM: ${TARGET_PLATFORM}" )

# detect target platform bits
if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
	set( PLATFORM_BITS 64 CACHE INTERNAL "" FORCE )
elseif (${CMAKE_SIZEOF_VOID_P} EQUAL 4)
	set( PLATFORM_BITS 32 CACHE INTERNAL "" FORCE )
else ()
	message( FATAL_ERROR "unsupported platform bits!" )
endif ()

if ( WIN32 )
	set( CMAKE_SYSTEM_VERSION "8.1" CACHE TYPE INTERNAL FORCE )
endif()
#==================================================================================================
# Visual Studio Compilation settings
#==================================================================================================
set( COMPILER_MSVC OFF )
string( FIND "${CMAKE_CXX_COMPILER_ID}" "MSVC" outPos )
if ( (outPos GREATER -1) )
	set( COMPILER_MSVC ON )
endif()
if ( COMPILER_MSVC )
	set( DETECTED_COMPILER "COMPILER_MSVC" )
	set( CURRENT_C_FLAGS ${CMAKE_C_FLAGS} )
	set( CURRENT_CXX_FLAGS ${CMAKE_CXX_FLAGS} )
	set( CURRENT_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} )
	set( CURRENT_STATIC_LINKER_FLAGS ${CMAKE_STATIC_LINKER_FLAGS} )
	set( CURRENT_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} )
	string( REPLACE "/EHa" " " CURRENT_CXX_FLAGS "${CURRENT_CXX_FLAGS}" )
	string( REPLACE "/EHsc" " " CURRENT_CXX_FLAGS "${CURRENT_CXX_FLAGS}" )
	string( REPLACE "/EHs" " " CURRENT_CXX_FLAGS "${CURRENT_CXX_FLAGS}" )
	string( REPLACE "//EHs-c-" " " CURRENT_CXX_FLAGS "${CURRENT_CXX_FLAGS}" )
	message( STATUS "CMAKE_C_FLAGS: ${CURRENT_C_FLAGS}" )
	message( STATUS "CMAKE_CXX_FLAGS: ${CURRENT_CXX_FLAGS}" )
	message( STATUS "CMAKE_EXE_LINKER_FLAGS: ${CURRENT_EXE_LINKER_FLAGS}" )
	message( STATUS "CMAKE_STATIC_LINKER_FLAGS: ${CURRENT_STATIC_LINKER_FLAGS}" )
	message( STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CURRENT_SHARED_LINKER_FLAGS}" )
	set( CMAKE_CONFIGURATION_TYPES Release Profile Debug )
	set( CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Configurations" FORCE )
	
	# Release
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Release>: > )
	set( CMAKE_C_FLAGS_RELEASE "${CURRENT_C_FLAGS} /D_NDEBUG /DNDEBUG  /MD /Ox /MP " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_RELEASE "${CURRENT_CXX_FLAGS} /D_NDEBUG /DNDEBUG /D_HAS_EXCEPTIONS=0  /MD /Ox /MP " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_RELEASE "${CURRENT_EXE_LINKER_FLAGS} /LTCG /RELEASE " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CURRENT_STATIC_LINKER_FLAGS} /LTCG /RELEASE " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CURRENT_SHARED_LINKER_FLAGS} /LTCG /RELEASE " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_RELEASE  /std:c++latest /MP /Gm- /Zc:inline /Gy- /fp:strict /fp:except- /we4002 /we4099 /we4129 /we4172 /we4201 /we4238 /we4239 /we4240 /we4251 /we4263 /we4264 /we4266 /we4273 /we4293 /we4305 /we4390 /we4456 /we4457 /we4458 /we4459 /we4473 /we4474 /we4522 /we4552 /we4553 /we4554 /we4700 /we4706 /we4715 /we4716 /we4717 /we4800 /we4927  /w14018  /w14127 /w14189 /w14244 /w14245 /w14287 /w14389 /w14505 /w14668 /w14701 /w14702 /w14703 /w14838 /w14946 /w14996 /w15038  /wd4061 /wd4062 /wd4063 /wd4310 /wd4365 /wd4455 /wd4503 /wd4514 /wd4623 /wd4625 /wd4626 /wd4710 /wd4714 /wd5026 /wd5027  /DCOMPILER_MSVC "/D${TARGET_PLATFORM}" "/DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "/DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "/DPLATFORM_BITS=${PLATFORM_BITS}" /Ob2 /Oi /Ot /Oy /GT /GL /GF /GS- /MD /W3 /Ox /analyze- /EHs-c- CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_RELEASE  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_RELEASE " /OPT:REF /OPT:ICF /INCREMENTAL:NO /LTCG /RELEASE /DYNAMICBASE" CACHE INTERNAL "" FORCE )
	# Profile
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Profile>: > )
	set( CMAKE_C_FLAGS_PROFILE "${CURRENT_C_FLAGS} /D_NDEBUG /DNDEBUG  /MD /Od /MP " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_PROFILE "${CURRENT_CXX_FLAGS} /D_NDEBUG /DNDEBUG  /MD /Od /MP " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_PROFILE "${CURRENT_EXE_LINKER_FLAGS} /DEBUG /PROFILE " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_PROFILE "${CURRENT_STATIC_LINKER_FLAGS} /DEBUG /PROFILE " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_PROFILE "${CURRENT_SHARED_LINKER_FLAGS} /DEBUG /PROFILE " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_PROFILE  /std:c++latest /MP /Gm- /Zc:inline /Gy- /fp:strict /fp:except- /we4002 /we4099 /we4129 /we4172 /we4201 /we4238 /we4239 /we4240 /we4251 /we4263 /we4264 /we4266 /we4273 /we4293 /we4305 /we4390 /we4456 /we4457 /we4458 /we4459 /we4473 /we4474 /we4522 /we4552 /we4553 /we4554 /we4700 /we4706 /we4715 /we4716 /we4717 /we4800 /we4927  /w14018 /w14100 /w14127 /w14189 /w14244 /w14245 /w14287 /w14389 /w14505 /w14668 /w14701 /w14702 /w14703 /w14838 /w14946 /w14996 /w15038  /wd4061 /wd4062 /wd4063 /wd4310 /wd4365 /wd4455 /wd4503 /wd4514 /wd4623 /wd4625 /wd4626 /wd4710 /wd4714 /wd5026 /wd5027  /DCOMPILER_MSVC "/D${TARGET_PLATFORM}" "/DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "/DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "/DPLATFORM_BITS=${PLATFORM_BITS}" /Ob2 /Oi /Ot /Oy /GT /GL /GF /GS- /MD /W3 /Od /analyze- /EHsc /GR CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_PROFILE  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_PROFILE " /OPT:REF /OPT:ICF /INCREMENTAL:NO /DEBUG /PROFILE" CACHE INTERNAL "" FORCE )
	# Debug
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>: > )
	set( CMAKE_C_FLAGS_DEBUG "${CURRENT_C_FLAGS} /D_DEBUG /MDd /Od /MP " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_DEBUG "${CURRENT_CXX_FLAGS} /D_DEBUG /MDd /Od /Zi /MP " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_DEBUG "${CURRENT_EXE_LINKER_FLAGS} /DEBUG:FULL " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CURRENT_STATIC_LINKER_FLAGS} /DEBUG:FULL " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CURRENT_SHARED_LINKER_FLAGS} /DEBUG:FULL " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_DEBUG  /std:c++latest /MP /Gm- /Zc:inline /Gy- /fp:strict /fp:except- /we4002 /we4099 /we4129 /we4172 /we4201 /we4238 /we4239 /we4240 /we4251 /we4263 /we4264 /we4266 /we4273 /we4293 /we4305 /we4390 /we4456 /we4457 /we4458 /we4459 /we4473 /we4474 /we4522 /we4552 /we4553 /we4554 /we4700 /we4706 /we4715 /we4716 /we4717 /we4800 /we4927  /w14018 /w14100 /w14127 /w14189 /w14244 /w14245 /w14287 /w14389 /w14505 /w14668 /w14701 /w14702 /w14703 /w14838 /w14946 /w14996 /w15038  /wd4061 /wd4062 /wd4063 /wd4310 /wd4365 /wd4455 /wd4503 /wd4514 /wd4623 /wd4625 /wd4626 /wd4710 /wd4714 /wd5026 /wd5027  /DCOMPILER_MSVC "/D${TARGET_PLATFORM}" "/DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "/DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "/DPLATFORM_BITS=${PLATFORM_BITS}" /W4 /WX- /sdl /Od /Ob0 /EHsc /Oy- /GF- /GS /GR /MDd /analyze- /Zi /RTCsu CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_DEBUG  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_DEBUG " /OPT:REF /OPT:ICF /INCREMENTAL:NO /DEBUG:FULL" CACHE INTERNAL "" FORCE )
	set( CMAKE_BUILD_TYPE "Debug")
	#--------------------------------------------
	set( CONFIGURATION_DEPENDENT_PATH ON CACHE INTERNAL "" FORCE )
	#--------------------------------------------
endif()


#==================================================================================================
# GCC Compilation settings
#==================================================================================================
set( COMPILER_GCC OFF )
string( FIND "${CMAKE_CXX_COMPILER_ID}" "GNU" outPos )
if ( (outPos GREATER -1) )
	set( COMPILER_GCC ON )
endif()
if ( COMPILER_GCC )
	set( DETECTED_COMPILER "COMPILER_GCC" )
	message( STATUS "CMAKE_C_FLAGS: ${CURRENT_C_FLAGS}" )
	message( STATUS "CMAKE_CXX_FLAGS: ${CURRENT_CXX_FLAGS}" )
	message( STATUS "CMAKE_EXE_LINKER_FLAGS: ${CURRENT_EXE_LINKER_FLAGS}" )
	message( STATUS "CMAKE_STATIC_LINKER_FLAGS: ${CURRENT_STATIC_LINKER_FLAGS}" )
	message( STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CURRENT_SHARED_LINKER_FLAGS}" )
	set( CMAKE_CONFIGURATION_TYPES Release Profile Debug )
	set( CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Configurations" FORCE )
	
	# Release
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Release>: > )
	set( CMAKE_C_FLAGS_RELEASE "${CURRENT_C_FLAGS} -D_NDEBUG -DNDEBUG  -O3 -finline-functions -Wno-undef -Wno-switch  " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_RELEASE "${CURRENT_CXX_FLAGS} -D_NDEBUG -DNDEBUG  -O3 -finline-functions -Wno-undef -Wno-switch  " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_RELEASE "${CURRENT_EXE_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CURRENT_STATIC_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CURRENT_SHARED_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_RELEASE  -std=c++1z -Wchar-subscripts -Wdouble-promotion -Wformat -Wmain -Wmissing-braces -Wmissing-include-dirs -Wuninitialized -Wmaybe-uninitialized -Wunknown-pragmas -Wpragmas -Wstrict-aliasing -Wstrict-overflow -Wendif-labels -Wfree-nonheap-object -Wpointer-arith -Wcast-align -Wwrite-strings -Wconversion-null -Wenum-compare -Wsign-compare -Wsizeof-pointer-memaccess -Wlogical-op -Waddress -Wno-unused -Wno-non-template-friend -Wno-zero-as-null-pointer-constant -Wno-shadow -Wno-enum-compare -Wno-narrowing -Wno-attributes -Wno-invalid-offsetof  -Werror=init-self -Werror=parentheses -Werror=return-local-addr -Werror=return-type -Werror=array-bounds -Werror=div-by-zero -Werror=missing-field-initializers -Werror=placement-new -Werror=sign-compare -Werror=cast-qual -Werror=cast-align -Werror=literal-suffix -Werror=shadow=local -Werror=delete-incomplete -Werror=subobject-linkage -Werror=odr -Werror=old-style-declaration -Werror=old-style-definition  -DCOMPILER_GCC "-D${TARGET_PLATFORM}" "-DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "-DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "-DPLATFORM_BITS=${PLATFORM_BITS}" -O3 -Ofast -fomit-frame-pointer -finline-functions CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_RELEASE  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_RELEASE " -static-libgcc -static-libstdc++" CACHE INTERNAL "" FORCE )
	# Profile
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Profile>: > )
	set( CMAKE_C_FLAGS_PROFILE "${CURRENT_C_FLAGS} -D_NDEBUG -DNDEBUG  -O2 -Wno-undef -Wno-switch  " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_PROFILE "${CURRENT_CXX_FLAGS} -D_NDEBUG -DNDEBUG  -O2 -Wno-undef -Wno-switch  " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_PROFILE "${CURRENT_EXE_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_PROFILE "${CURRENT_STATIC_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_PROFILE "${CURRENT_SHARED_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_PROFILE  -std=c++1z -Wchar-subscripts -Wdouble-promotion -Wformat -Wmain -Wmissing-braces -Wmissing-include-dirs -Wuninitialized -Wmaybe-uninitialized -Wunknown-pragmas -Wpragmas -Wstrict-aliasing -Wstrict-overflow -Wendif-labels -Wfree-nonheap-object -Wpointer-arith -Wcast-align -Wwrite-strings -Wconversion-null -Wenum-compare -Wsign-compare -Wsizeof-pointer-memaccess -Wlogical-op -Waddress -Wno-unused -Wno-non-template-friend -Wno-zero-as-null-pointer-constant -Wno-shadow -Wno-enum-compare -Wno-narrowing -Wno-attributes -Wno-invalid-offsetof  -Werror=init-self -Werror=parentheses -Werror=return-local-addr -Werror=return-type -Werror=array-bounds -Werror=div-by-zero -Werror=missing-field-initializers -Werror=placement-new -Werror=sign-compare -Werror=cast-qual -Werror=cast-align -Werror=literal-suffix -Werror=shadow=local -Werror=delete-incomplete -Werror=subobject-linkage -Werror=odr -Werror=old-style-declaration -Werror=old-style-definition  -DCOMPILER_GCC "-D${TARGET_PLATFORM}" "-DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "-DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "-DPLATFORM_BITS=${PLATFORM_BITS}" -O2 CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_PROFILE  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_PROFILE " -static-libgcc -static-libstdc++" CACHE INTERNAL "" FORCE )
	# Debug
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>: > )
	set( CMAKE_C_FLAGS_DEBUG "${CURRENT_C_FLAGS} -D_DEBUG -Og -Wno-undef -Wno-switch  " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_DEBUG "${CURRENT_CXX_FLAGS} -D_DEBUG -Og -Wno-undef -Wno-switch  " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_DEBUG "${CURRENT_EXE_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CURRENT_STATIC_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CURRENT_SHARED_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_DEBUG  -std=c++1z -Wchar-subscripts -Wdouble-promotion -Wformat -Wmain -Wmissing-braces -Wmissing-include-dirs -Wuninitialized -Wmaybe-uninitialized -Wunknown-pragmas -Wpragmas -Wstrict-aliasing -Wstrict-overflow -Wendif-labels -Wfree-nonheap-object -Wpointer-arith -Wcast-align -Wwrite-strings -Wconversion-null -Wenum-compare -Wsign-compare -Wsizeof-pointer-memaccess -Wlogical-op -Waddress -Wno-unused -Wno-non-template-friend -Wno-zero-as-null-pointer-constant -Wno-shadow -Wno-enum-compare -Wno-narrowing -Wno-attributes -Wno-invalid-offsetof  -Werror=init-self -Werror=parentheses -Werror=return-local-addr -Werror=return-type -Werror=array-bounds -Werror=div-by-zero -Werror=missing-field-initializers -Werror=placement-new -Werror=sign-compare -Werror=cast-qual -Werror=cast-align -Werror=literal-suffix -Werror=shadow=local -Werror=delete-incomplete -Werror=subobject-linkage -Werror=odr -Werror=old-style-declaration -Werror=old-style-definition  -DCOMPILER_GCC "-D${TARGET_PLATFORM}" "-DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "-DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "-DPLATFORM_BITS=${PLATFORM_BITS}" -ggdb -fchkp-check-incomplete-type -Og -Wno-terminate  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_DEBUG  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_DEBUG " -static-libgcc -static-libstdc++" CACHE INTERNAL "" FORCE )
	set( CMAKE_BUILD_TYPE "Debug")
	#--------------------------------------------
	set( CONFIGURATION_DEPENDENT_PATH OFF CACHE INTERNAL "" FORCE )
	#--------------------------------------------
endif()


#==================================================================================================
# Clang Compilation settings
#==================================================================================================
set( COMPILER_CLANG OFF )
string( FIND "${CMAKE_CXX_COMPILER_ID}" "Clang" outPos )
if ( (outPos GREATER -1) AND (WIN32 OR UNIX) )
	set( COMPILER_CLANG ON )
endif()
if ( COMPILER_CLANG )
	set( DETECTED_COMPILER "COMPILER_CLANG" )
	message( STATUS "CMAKE_C_FLAGS: ${CURRENT_C_FLAGS}" )
	message( STATUS "CMAKE_CXX_FLAGS: ${CURRENT_CXX_FLAGS}" )
	message( STATUS "CMAKE_EXE_LINKER_FLAGS: ${CURRENT_EXE_LINKER_FLAGS}" )
	message( STATUS "CMAKE_STATIC_LINKER_FLAGS: ${CURRENT_STATIC_LINKER_FLAGS}" )
	message( STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CURRENT_SHARED_LINKER_FLAGS}" )
	set( CMAKE_CONFIGURATION_TYPES Release Profile Debug )
	set( CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Configurations" FORCE )
	
	# Release
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Release>: > )
	set( CMAKE_C_FLAGS_RELEASE "${CURRENT_C_FLAGS} -D_NDEBUG -DNDEBUG  -O3 -Wno-undef -Wno-switch  " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_RELEASE "${CURRENT_CXX_FLAGS} -D_NDEBUG -DNDEBUG  -O3 -Wno-undef -Wno-switch  " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_RELEASE "${CURRENT_EXE_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CURRENT_STATIC_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CURRENT_SHARED_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_RELEASE  -std=c++1z -Wchar-subscripts -Wdouble-promotion -Wformat -Wmain -Wmissing-braces -Wmissing-include-dirs -Wunused -Wuninitialized -Wmaybe-uninitialized -Wunknown-pragmas -Wpragmas -Wstrict-aliasing -Wstrict-overflow -Wundef -Wendif-labels -Wfree-nonheap-object -Wpointer-arith -Wwrite-strings -Wconversion-null -Wzero-as-null-pointer-constant -Wenum-compare -Wsign-compare -Wsizeof-pointer-memaccess -Wlogical-op -frtti -fexceptions -Wloop-analysis -Wincrement-bool -Werror=init-self -Werror=parentheses -Werror=return-stack-address -Werror=return-type -Werror=non-template-friend -Werror=array-bounds -Werror=div-by-zero -Werror=address -Werror=missing-field-initializers -Werror=placement-new -Werror=cast-qual -Werror=cast-align -Werror=unknown-warning-option -Werror=user-defined-literals -Werror=keyword-macro -Werror=large-by-value-copy -Werror=instantiation-after-specialization -Werror=method-signatures -Werror=self-assign -Werror=self-move -Werror=infinite-recursion -Werror=pessimizing-move -Werror=dangling-else  -Wno-non-template-friend -Wno-comment -Wno-undefined-inline -Wno-c++11-narrowing -Wno-c++14-extensions -Wno-c++1z-extensions  -DCOMPILER_CLANG "-D${TARGET_PLATFORM}" "-DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "-DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "-DPLATFORM_BITS=${PLATFORM_BITS}" -O3 -Ofast -fomit-frame-pointer -finline-functions CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_RELEASE  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_RELEASE "" CACHE INTERNAL "" FORCE )
	# Profile
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Profile>: > )
	set( CMAKE_C_FLAGS_PROFILE "${CURRENT_C_FLAGS} -D_NDEBUG -DNDEBUG  -O2 -Wno-undef -Wno-switch  " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_PROFILE "${CURRENT_CXX_FLAGS} -D_NDEBUG -DNDEBUG  -O2 -Wno-undef -Wno-switch  " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_PROFILE "${CURRENT_EXE_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_PROFILE "${CURRENT_STATIC_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_PROFILE "${CURRENT_SHARED_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_PROFILE  -std=c++1z -Wchar-subscripts -Wdouble-promotion -Wformat -Wmain -Wmissing-braces -Wmissing-include-dirs -Wunused -Wuninitialized -Wmaybe-uninitialized -Wunknown-pragmas -Wpragmas -Wstrict-aliasing -Wstrict-overflow -Wundef -Wendif-labels -Wfree-nonheap-object -Wpointer-arith -Wwrite-strings -Wconversion-null -Wzero-as-null-pointer-constant -Wenum-compare -Wsign-compare -Wsizeof-pointer-memaccess -Wlogical-op -frtti -fexceptions -Wloop-analysis -Wincrement-bool -Werror=init-self -Werror=parentheses -Werror=return-stack-address -Werror=return-type -Werror=non-template-friend -Werror=array-bounds -Werror=div-by-zero -Werror=address -Werror=missing-field-initializers -Werror=placement-new -Werror=cast-qual -Werror=cast-align -Werror=unknown-warning-option -Werror=user-defined-literals -Werror=keyword-macro -Werror=large-by-value-copy -Werror=instantiation-after-specialization -Werror=method-signatures -Werror=self-assign -Werror=self-move -Werror=infinite-recursion -Werror=pessimizing-move -Werror=dangling-else  -Wno-non-template-friend -Wno-comment -Wno-undefined-inline -Wno-c++11-narrowing -Wno-c++14-extensions -Wno-c++1z-extensions  -DCOMPILER_CLANG "-D${TARGET_PLATFORM}" "-DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "-DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "-DPLATFORM_BITS=${PLATFORM_BITS}" -O2 -finline-functions CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_PROFILE  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_PROFILE "" CACHE INTERNAL "" FORCE )
	# Debug
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>: > )
	set( CMAKE_C_FLAGS_DEBUG "${CURRENT_C_FLAGS} -D_DEBUG -Og -Wno-undef -Wno-switch  " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_DEBUG "${CURRENT_CXX_FLAGS} -D_DEBUG -Og -Wno-undef -Wno-switch  " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_DEBUG "${CURRENT_EXE_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CURRENT_STATIC_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CURRENT_SHARED_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_DEBUG  -std=c++1z -Wchar-subscripts -Wdouble-promotion -Wformat -Wmain -Wmissing-braces -Wmissing-include-dirs -Wunused -Wuninitialized -Wmaybe-uninitialized -Wunknown-pragmas -Wpragmas -Wstrict-aliasing -Wstrict-overflow -Wundef -Wendif-labels -Wfree-nonheap-object -Wpointer-arith -Wwrite-strings -Wconversion-null -Wzero-as-null-pointer-constant -Wenum-compare -Wsign-compare -Wsizeof-pointer-memaccess -Wlogical-op -frtti -fexceptions -Wloop-analysis -Wincrement-bool -Werror=init-self -Werror=parentheses -Werror=return-stack-address -Werror=return-type -Werror=non-template-friend -Werror=array-bounds -Werror=div-by-zero -Werror=address -Werror=missing-field-initializers -Werror=placement-new -Werror=cast-qual -Werror=cast-align -Werror=unknown-warning-option -Werror=user-defined-literals -Werror=keyword-macro -Werror=large-by-value-copy -Werror=instantiation-after-specialization -Werror=method-signatures -Werror=self-assign -Werror=self-move -Werror=infinite-recursion -Werror=pessimizing-move -Werror=dangling-else  -Wno-non-template-friend -Wno-comment -Wno-undefined-inline -Wno-c++11-narrowing -Wno-c++14-extensions -Wno-c++1z-extensions  -DCOMPILER_CLANG "-D${TARGET_PLATFORM}" "-DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "-DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "-DPLATFORM_BITS=${PLATFORM_BITS}" -ggdb -fchkp-check-incomplete-type -Og CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_DEBUG  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_DEBUG "" CACHE INTERNAL "" FORCE )
	set( CMAKE_BUILD_TYPE "Debug")
	#--------------------------------------------
	set( CONFIGURATION_DEPENDENT_PATH OFF CACHE INTERNAL "" FORCE )
	#--------------------------------------------
endif()


#==================================================================================================
# Clang Compilation settings
#==================================================================================================
set( COMPILER_CLANG_APPLE OFF )
string( FIND "${CMAKE_CXX_COMPILER_ID}" "Clang" outPos )
if ( (outPos GREATER -1) AND (APPLE) )
	set( COMPILER_CLANG_APPLE ON )
endif()
if ( COMPILER_CLANG_APPLE )
	set( DETECTED_COMPILER "COMPILER_CLANG_APPLE" )
	message( STATUS "CMAKE_C_FLAGS: ${CURRENT_C_FLAGS}" )
	message( STATUS "CMAKE_CXX_FLAGS: ${CURRENT_CXX_FLAGS}" )
	message( STATUS "CMAKE_EXE_LINKER_FLAGS: ${CURRENT_EXE_LINKER_FLAGS}" )
	message( STATUS "CMAKE_STATIC_LINKER_FLAGS: ${CURRENT_STATIC_LINKER_FLAGS}" )
	message( STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CURRENT_SHARED_LINKER_FLAGS}" )
	set( CMAKE_CONFIGURATION_TYPES Release Profile Debug )
	set( CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Configurations" FORCE )
	
	# Release
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Release>: > )
	set( CMAKE_C_FLAGS_RELEASE "${CURRENT_C_FLAGS} -D_NDEBUG -DNDEBUG  -O3 -finline-functions -Wno-undef -Wno-switch -Wno-c++11-narrowing -Wno-unused  " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_RELEASE "${CURRENT_CXX_FLAGS} -D_NDEBUG -DNDEBUG  -O3 -finline-functions -Wno-undef -Wno-switch -Wno-c++11-narrowing -Wno-unused  " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_RELEASE "${CURRENT_EXE_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CURRENT_STATIC_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CURRENT_SHARED_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_RELEASE  -std=c++1z -Wchar-subscripts -Wdouble-promotion -Wformat -Wmain -Wmissing-braces -Wmissing-include-dirs -Wuninitialized -Wunknown-pragmas -Wpragmas -Wstrict-aliasing -Wstrict-overflow -Wundef -Wendif-labels -Wpointer-arith -Wwrite-strings -Wconversion-null -Wenum-compare -Wsign-compare -Wsizeof-pointer-memaccess -frtti -fexceptions -Wloop-analysis -Wincrement-bool -Werror=init-self -Werror=parentheses -Werror=return-stack-address -Werror=return-type -Werror=user-defined-literals -Werror=array-bounds -Werror=div-by-zero -Werror=address -Werror=missing-field-initializers -Werror=instantiation-after-specialization -Werror=cast-qual -Werror=unknown-warning-option -Werror=keyword-macro -Werror=large-by-value-copy -Werror=method-signatures -Werror=self-assign -Werror=self-move -Werror=infinite-recursion -Werror=pessimizing-move -Werror=dangling-else  -Wno-comment -Wno-undefined-inline -Wno-c++14-extensions -Wno-c++1z-extensions  -DCOMPILER_CLANG "-D${TARGET_PLATFORM}" "-DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "-DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "-DPLATFORM_BITS=${PLATFORM_BITS}" -O3 -Ofast -fomit-frame-pointer -finline-functions CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_RELEASE  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_RELEASE "" CACHE INTERNAL "" FORCE )
	# Profile
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Profile>: > )
	set( CMAKE_C_FLAGS_PROFILE "${CURRENT_C_FLAGS} -D_NDEBUG -DNDEBUG  -O2 -Wno-undef -Wno-switch -Wno-c++11-narrowing -Wno-unused  " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_PROFILE "${CURRENT_CXX_FLAGS} -D_NDEBUG -DNDEBUG  -O2 -Wno-undef -Wno-switch -Wno-c++11-narrowing -Wno-unused  " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_PROFILE "${CURRENT_EXE_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_PROFILE "${CURRENT_STATIC_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_PROFILE "${CURRENT_SHARED_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_PROFILE  -std=c++1z -Wchar-subscripts -Wdouble-promotion -Wformat -Wmain -Wmissing-braces -Wmissing-include-dirs -Wuninitialized -Wunknown-pragmas -Wpragmas -Wstrict-aliasing -Wstrict-overflow -Wundef -Wendif-labels -Wpointer-arith -Wwrite-strings -Wconversion-null -Wenum-compare -Wsign-compare -Wsizeof-pointer-memaccess -frtti -fexceptions -Wloop-analysis -Wincrement-bool -Werror=init-self -Werror=parentheses -Werror=return-stack-address -Werror=return-type -Werror=user-defined-literals -Werror=array-bounds -Werror=div-by-zero -Werror=address -Werror=missing-field-initializers -Werror=instantiation-after-specialization -Werror=cast-qual -Werror=unknown-warning-option -Werror=keyword-macro -Werror=large-by-value-copy -Werror=method-signatures -Werror=self-assign -Werror=self-move -Werror=infinite-recursion -Werror=pessimizing-move -Werror=dangling-else  -Wno-comment -Wno-undefined-inline -Wno-c++14-extensions -Wno-c++1z-extensions  -DCOMPILER_CLANG "-D${TARGET_PLATFORM}" "-DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "-DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "-DPLATFORM_BITS=${PLATFORM_BITS}" -O2 -finline-functions CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_PROFILE  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_PROFILE "" CACHE INTERNAL "" FORCE )
	# Debug
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>: > )
	set( CMAKE_C_FLAGS_DEBUG "${CURRENT_C_FLAGS} -D_DEBUG -Og -Wno-undef -Wno-switch -Wno-c++11-narrowing -Wno-unused  " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_DEBUG "${CURRENT_CXX_FLAGS} -D_DEBUG -Og -Wno-undef -Wno-switch -Wno-c++11-narrowing -Wno-unused  " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_DEBUG "${CURRENT_EXE_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CURRENT_STATIC_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CURRENT_SHARED_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_DEBUG  -std=c++1z -Wchar-subscripts -Wdouble-promotion -Wformat -Wmain -Wmissing-braces -Wmissing-include-dirs -Wuninitialized -Wunknown-pragmas -Wpragmas -Wstrict-aliasing -Wstrict-overflow -Wundef -Wendif-labels -Wpointer-arith -Wwrite-strings -Wconversion-null -Wenum-compare -Wsign-compare -Wsizeof-pointer-memaccess -frtti -fexceptions -Wloop-analysis -Wincrement-bool -Werror=init-self -Werror=parentheses -Werror=return-stack-address -Werror=return-type -Werror=user-defined-literals -Werror=array-bounds -Werror=div-by-zero -Werror=address -Werror=missing-field-initializers -Werror=instantiation-after-specialization -Werror=cast-qual -Werror=unknown-warning-option -Werror=keyword-macro -Werror=large-by-value-copy -Werror=method-signatures -Werror=self-assign -Werror=self-move -Werror=infinite-recursion -Werror=pessimizing-move -Werror=dangling-else  -Wno-comment -Wno-undefined-inline -Wno-c++14-extensions -Wno-c++1z-extensions  -DCOMPILER_CLANG "-D${TARGET_PLATFORM}" "-DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "-DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "-DPLATFORM_BITS=${PLATFORM_BITS}" -ggdb -Og CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_DEBUG  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_DEBUG "" CACHE INTERNAL "" FORCE )
	set( CMAKE_BUILD_TYPE "Debug")
	#--------------------------------------------
	set( CONFIGURATION_DEPENDENT_PATH ON CACHE INTERNAL "" FORCE )
	#--------------------------------------------
endif()


#==================================================================================================
# Clang Compilation settings
#==================================================================================================
set( COMPILER_CLANG_ANDROID OFF )
string( FIND "${CMAKE_CXX_COMPILER_ID}" "Clang" outPos )
if ( (outPos GREATER -1) AND (DEFINED ANDROID) )
	set( COMPILER_CLANG_ANDROID ON )
endif()
if ( COMPILER_CLANG_ANDROID )
	set( DETECTED_COMPILER "COMPILER_CLANG_ANDROID" )
	message( STATUS "CMAKE_C_FLAGS: ${CURRENT_C_FLAGS}" )
	message( STATUS "CMAKE_CXX_FLAGS: ${CURRENT_CXX_FLAGS}" )
	message( STATUS "CMAKE_EXE_LINKER_FLAGS: ${CURRENT_EXE_LINKER_FLAGS}" )
	message( STATUS "CMAKE_STATIC_LINKER_FLAGS: ${CURRENT_STATIC_LINKER_FLAGS}" )
	message( STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CURRENT_SHARED_LINKER_FLAGS}" )
	set( CMAKE_CONFIGURATION_TYPES Release Profile Debug )
	set( CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Configurations" FORCE )
	
	# Release
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Release>: > )
	set( CMAKE_C_FLAGS_RELEASE "${CURRENT_C_FLAGS} -D_NDEBUG -DNDEBUG  -O3 -finline-functions -Wno-undef -Wno-switch -Wno-c++11-narrowing -Wno-unused  " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_RELEASE "${CURRENT_CXX_FLAGS} -D_NDEBUG -DNDEBUG  -O3 -finline-functions -Wno-undef -Wno-switch -Wno-c++11-narrowing -Wno-unused  " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_RELEASE "${CURRENT_EXE_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CURRENT_STATIC_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CURRENT_SHARED_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_RELEASE  -std=c++1z -Wchar-subscripts -Wdouble-promotion -Wformat -Wmain -Wmissing-braces -Wmissing-include-dirs -Wuninitialized -Wunknown-pragmas -Wpragmas -Wstrict-aliasing -Wstrict-overflow -Wundef -Wendif-labels -Wpointer-arith -Wwrite-strings -Wconversion-null -Wenum-compare -Wsign-compare -Wsizeof-pointer-memaccess -frtti -fexceptions -Wloop-analysis -Wincrement-bool -Werror=init-self -Werror=parentheses -Werror=return-stack-address -Werror=return-type -Werror=user-defined-literals -Werror=array-bounds -Werror=div-by-zero -Werror=address -Werror=missing-field-initializers -Werror=instantiation-after-specialization -Werror=cast-qual -Werror=unknown-warning-option -Werror=keyword-macro -Werror=large-by-value-copy -Werror=dangling-else -Werror=method-signatures -Werror=self-assign -Werror=self-move -Werror=infinite-recursion -Werror=pessimizing-move  -Wno-comment -Wno-undefined-inline -Wno-c++14-extensions -Wno-c++1z-extensions  -DCOMPILER_CLANG "-D${TARGET_PLATFORM}" "-DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "-DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "-DPLATFORM_BITS=${PLATFORM_BITS}" -O3 -Ofast -fomit-frame-pointer -finline-functions CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_RELEASE  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_RELEASE " -static" CACHE INTERNAL "" FORCE )
	# Profile
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Profile>: > )
	set( CMAKE_C_FLAGS_PROFILE "${CURRENT_C_FLAGS} -D_NDEBUG -DNDEBUG  -O2 -Wno-undef -Wno-switch -Wno-c++11-narrowing -Wno-unused  " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_PROFILE "${CURRENT_CXX_FLAGS} -D_NDEBUG -DNDEBUG  -O2 -Wno-undef -Wno-switch -Wno-c++11-narrowing -Wno-unused  " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_PROFILE "${CURRENT_EXE_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_PROFILE "${CURRENT_STATIC_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_PROFILE "${CURRENT_SHARED_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_PROFILE  -std=c++1z -Wchar-subscripts -Wdouble-promotion -Wformat -Wmain -Wmissing-braces -Wmissing-include-dirs -Wuninitialized -Wunknown-pragmas -Wpragmas -Wstrict-aliasing -Wstrict-overflow -Wundef -Wendif-labels -Wpointer-arith -Wwrite-strings -Wconversion-null -Wenum-compare -Wsign-compare -Wsizeof-pointer-memaccess -frtti -fexceptions -Wloop-analysis -Wincrement-bool -Werror=init-self -Werror=parentheses -Werror=return-stack-address -Werror=return-type -Werror=user-defined-literals -Werror=array-bounds -Werror=div-by-zero -Werror=address -Werror=missing-field-initializers -Werror=instantiation-after-specialization -Werror=cast-qual -Werror=unknown-warning-option -Werror=keyword-macro -Werror=large-by-value-copy -Werror=dangling-else -Werror=method-signatures -Werror=self-assign -Werror=self-move -Werror=infinite-recursion -Werror=pessimizing-move  -Wno-comment -Wno-undefined-inline -Wno-c++14-extensions -Wno-c++1z-extensions  -DCOMPILER_CLANG "-D${TARGET_PLATFORM}" "-DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "-DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "-DPLATFORM_BITS=${PLATFORM_BITS}" -O2 -finline-functions CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_PROFILE  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_PROFILE " -static" CACHE INTERNAL "" FORCE )
	# Debug
	set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>: > )
	set( CMAKE_C_FLAGS_DEBUG "${CURRENT_C_FLAGS} -D_DEBUG -Og -Wno-undef -Wno-switch -Wno-c++11-narrowing -Wno-unused  " CACHE STRING "" FORCE )
	set( CMAKE_CXX_FLAGS_DEBUG "${CURRENT_CXX_FLAGS} -D_DEBUG -Og -Wno-undef -Wno-switch -Wno-c++11-narrowing -Wno-unused  " CACHE STRING "" FORCE )
	set( CMAKE_EXE_LINKER_FLAGS_DEBUG "${CURRENT_EXE_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CURRENT_STATIC_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CURRENT_SHARED_LINKER_FLAGS} " CACHE STRING "" FORCE )
	set( PROJECTS_SHARED_CXX_FLAGS_DEBUG  -std=c++1z -Wchar-subscripts -Wdouble-promotion -Wformat -Wmain -Wmissing-braces -Wmissing-include-dirs -Wuninitialized -Wunknown-pragmas -Wpragmas -Wstrict-aliasing -Wstrict-overflow -Wundef -Wendif-labels -Wpointer-arith -Wwrite-strings -Wconversion-null -Wenum-compare -Wsign-compare -Wsizeof-pointer-memaccess -frtti -fexceptions -Wloop-analysis -Wincrement-bool -Werror=init-self -Werror=parentheses -Werror=return-stack-address -Werror=return-type -Werror=user-defined-literals -Werror=array-bounds -Werror=div-by-zero -Werror=address -Werror=missing-field-initializers -Werror=instantiation-after-specialization -Werror=cast-qual -Werror=unknown-warning-option -Werror=keyword-macro -Werror=large-by-value-copy -Werror=dangling-else -Werror=method-signatures -Werror=self-assign -Werror=self-move -Werror=infinite-recursion -Werror=pessimizing-move  -Wno-comment -Wno-undefined-inline -Wno-c++14-extensions -Wno-c++1z-extensions  -DCOMPILER_CLANG "-D${TARGET_PLATFORM}" "-DPLATFORM_NAME=\"${CMAKE_SYSTEM_NAME}\"" "-DPLATFORM_CPU_NAME=\"${CMAKE_SYSTEM_PROCESSOR}\"" "-DPLATFORM_BITS=${PLATFORM_BITS}" -ggdb -Og CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_DEFINES_DEBUG  CACHE INTERNAL "" FORCE )
	set( PROJECTS_SHARED_LINKER_FLAGS_DEBUG " -static" CACHE INTERNAL "" FORCE )
	set( CMAKE_BUILD_TYPE "Debug")
	#--------------------------------------------
	set( CONFIGURATION_DEPENDENT_PATH OFF CACHE INTERNAL "" FORCE )
	#--------------------------------------------
endif()


if ( NOT DEFINED DETECTED_COMPILER )
	message( FATAL_ERROR "current compiler: '${CMAKE_CXX_COMPILER_ID}' is not configured for this project!" )
endif()


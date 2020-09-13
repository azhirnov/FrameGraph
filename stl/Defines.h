// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Config.h"


#ifdef COMPILER_MSVC
#	define and		&&
#	define or		||
#	define not		!
#endif


// mark output and input-output function arguments
#ifndef OUT
#	define OUT
#endif

#ifndef INOUT
#	define INOUT
#endif


// no discard
#ifndef ND_
# ifdef COMPILER_MSVC
#  if _MSC_VER >= 1917
#	define ND_				[[nodiscard]]
#  else
#	define ND_
#  endif
# endif	// COMPILER_MSVC

# ifdef COMPILER_CLANG
#  if __has_feature( cxx_attributes )
#	define ND_				[[nodiscard]]
#  else
#	define ND_
#  endif
# endif	// COMPILER_CLANG

# ifdef COMPILER_GCC
#  if __has_cpp_attribute( nodiscard )
#	define ND_				[[nodiscard]]
#  else
#	define ND_
#  endif
# endif	// COMPILER_GCC

#endif	// ND_


// null pointer
#ifndef null
#	define null				nullptr
#endif


// force inline
#ifndef forceinline
# ifdef FG_DEBUG
#	define forceinline		inline

# elif defined(COMPILER_MSVC)
#	define forceinline		__forceinline

# elif defined(COMPILER_CLANG) or defined(COMPILER_GCC)
#	define forceinline		__inline__ __attribute__((always_inline))

# else
#	pragma warning ("forceinline is not supported")
#	define forceinline		inline
# endif
#endif


// debug break
#ifndef FG_PRIVATE_BREAK_POINT
# if defined(COMPILER_MSVC) and defined(FG_DEBUG)
#	define FG_PRIVATE_BREAK_POINT()		__debugbreak()

# elif defined(PLATFORM_ANDROID) and defined(FG_DEBUG)
#	include <csignal>
#	define FG_PRIVATE_BREAK_POINT()		std::raise( SIGINT )

# elif (defined(COMPILER_CLANG) or defined(COMPILER_GCC)) and defined(FG_DEBUG)
#  if 1
#	include <exception>
#	define FG_PRIVATE_BREAK_POINT() 	throw std::runtime_error("breakpoint")
#  else
#	include <csignal>
#	define FG_PRIVATE_BREAK_POINT()		std::raise(SIGINT)
#  endif

# else
#	define FG_PRIVATE_BREAK_POINT()		{}
# endif
#endif


// exit
#ifndef FG_PRIVATE_EXIT
# if defined(PLATFORM_ANDROID)
#	define FG_PRIVATE_EXIT()	std::terminate()
# else
#	define FG_PRIVATE_EXIT()	::exit( EXIT_FAILURE )
# endif
#endif


// helper macro
#define FG_PRIVATE_GETARG_0( _0_, ... )				_0_
#define FG_PRIVATE_GETARG_1( _0_, _1_, ... )		_1_
#define FG_PRIVATE_GETARG_2( _0_, _1_, _2_, ... )	_2_
#define FG_PRIVATE_GETRAW( _value_ )				_value_
#define FG_PRIVATE_TOSTRING( ... )					#__VA_ARGS__
#define FG_PRIVATE_UNITE_RAW( _arg0_, _arg1_ )		FG_PRIVATE_UNITE( _arg0_, _arg1_ )
#define FG_PRIVATE_UNITE( _arg0_, _arg1_ )			_arg0_ ## _arg1_


// debug only check
#ifndef ASSERT
# ifdef FG_DEBUG
#	define ASSERT				CHECK
# else
#	define ASSERT( ... )		{}
# endif
#endif


// function name
#ifdef COMPILER_MSVC
#	define FG_FUNCTION_NAME			__FUNCTION__

#elif defined(COMPILER_CLANG) or defined(COMPILER_GCC)
#	define FG_FUNCTION_NAME			__func__

#else
#	define FG_FUNCTION_NAME			"unknown function"
#endif


// branch prediction optimization
#ifdef COMPILER_MSVC
#  if 0 //_MSC_VER >= 1926	// TODO: enable C++20
#	define if_likely( ... )		[[likely]] if ( __VA_ARGS__ )
#	define if_unlikely( ... )	[[unlikely]] if ( __VA_ARGS__ )
#  endif
#endif

#if defined(COMPILER_CLANG) or defined(COMPILER_GCC)
#	define if_likely( ... )		if ( __builtin_expect( !!(__VA_ARGS__), 1 ))
#	define if_unlikely( ... )	if ( __builtin_expect( !!(__VA_ARGS__), 0 ))
#endif

#ifndef if_likely
	// not supported
#	define if_likely( ... )		if ( __VA_ARGS__ )
#	define if_unlikely( ... )	if ( __VA_ARGS__ )
#endif


// no unique address
#if defined(COMPILER_GCC)
#  if __has_cpp_attribute( no_unique_address )
#	define NO_UNIQUE_ADDRESS	[[no_unique_address]]
#  endif
#endif

#ifndef NO_UNIQUE_ADDRESS
	// not supported
#	define NO_UNIQUE_ADDRESS
#endif


// debug only scope
#ifndef DEBUG_ONLY
# ifdef FG_DEBUG
#	define DEBUG_ONLY( ... )		__VA_ARGS__
# else
#	define DEBUG_ONLY( ... )
# endif
#endif


// log
// (text, file, line)
#ifndef FG_LOGD
# ifdef FG_DEBUG
#	define FG_LOGD	FG_LOGI
# else
#	define FG_LOGD( ... )
# endif
#endif

#ifndef FG_LOGI
#	define FG_LOGI( ... ) \
			FG_PRIVATE_LOGI(FG_PRIVATE_GETARG_0( __VA_ARGS__, "" ), \
							FG_PRIVATE_GETARG_1( __VA_ARGS__, __FILE__ ), \
							FG_PRIVATE_GETARG_2( __VA_ARGS__, __FILE__, __LINE__ ))
#endif

#ifndef FG_LOGE
#	define FG_LOGE( ... ) \
			FG_PRIVATE_LOGE(FG_PRIVATE_GETARG_0( __VA_ARGS__, "" ), \
							FG_PRIVATE_GETARG_1( __VA_ARGS__, __FILE__ ), \
							FG_PRIVATE_GETARG_2( __VA_ARGS__, __FILE__, __LINE__ ))
#endif


// check function return value
#ifndef CHECK
#	define FG_PRIVATE_CHECK( _expr_, _text_ ) \
		{if_likely (( _expr_ )) {} \
		 else { \
			FG_LOGE( _text_ ); \
		}}

#   define CHECK( _func_ ) \
		FG_PRIVATE_CHECK( (_func_), FG_PRIVATE_TOSTRING( _func_ ) )
#endif


// check function return value and return error code
#ifndef CHECK_ERR
#	define FG_PRIVATE_CHECK_ERR( _expr_, _ret_ ) \
		{if_likely (( _expr_ )) {}\
		  else { \
			FG_LOGE( FG_PRIVATE_TOSTRING( _expr_ ) ); \
			return (_ret_); \
		}}

#	define CHECK_ERR( ... ) \
		FG_PRIVATE_CHECK_ERR( FG_PRIVATE_GETARG_0( __VA_ARGS__ ), FG_PRIVATE_GETARG_1( __VA_ARGS__, ::FGC::Default ))
#endif

#ifndef CHECK_ERRV
#	define CHECK_ERRV( _expr_ ) \
		FG_PRIVATE_CHECK_ERR( (_expr_), void() )
#endif


// check function return value and exit
#ifndef CHECK_FATAL
#	define CHECK_FATAL( _expr_ ) \
		{if_likely (( _expr_ )) {}\
		  else { \
			FG_LOGE( FG_PRIVATE_TOSTRING( _expr_ ) ); \
			FG_PRIVATE_EXIT(); \
		}}
#endif


// return error code
#ifndef RETURN_ERR
#	define FG_PRIVATE_RETURN_ERR( _text_, _ret_ ) \
		{ FG_LOGE( _text_ );  return (_ret_); }

#	define RETURN_ERR( ... ) \
		FG_PRIVATE_RETURN_ERR( FG_PRIVATE_GETARG_0( __VA_ARGS__ ), FG_PRIVATE_GETARG_1( __VA_ARGS__, ::FGC::Default ))
#endif


// compile time assert
#ifndef STATIC_ASSERT
#	define STATIC_ASSERT( ... ) \
		static_assert(	FG_PRIVATE_GETRAW( FG_PRIVATE_GETARG_0( __VA_ARGS__ ) ), \
						FG_PRIVATE_GETRAW( FG_PRIVATE_GETARG_1( __VA_ARGS__, FG_PRIVATE_TOSTRING(__VA_ARGS__) )))
#endif


// bit operators
#define FG_BIT_OPERATORS( _type_ ) \
	ND_ constexpr _type_  operator |  (_type_ lhs, _type_ rhs)	{ return _type_( FGC::ToNearUInt(lhs) | FGC::ToNearUInt(rhs) ); } \
	ND_ constexpr _type_  operator &  (_type_ lhs, _type_ rhs)	{ return _type_( FGC::ToNearUInt(lhs) & FGC::ToNearUInt(rhs) ); } \
	\
	constexpr _type_&  operator |= (_type_ &lhs, _type_ rhs)	{ return lhs = _type_( FGC::ToNearUInt(lhs) | FGC::ToNearUInt(rhs) ); } \
	constexpr _type_&  operator &= (_type_ &lhs, _type_ rhs)	{ return lhs = _type_( FGC::ToNearUInt(lhs) & FGC::ToNearUInt(rhs) ); } \
	\
	ND_ constexpr _type_  operator ~ (_type_ lhs)				{ return _type_(~FGC::ToNearUInt(lhs)); } \
	ND_ constexpr bool   operator ! (_type_ lhs)				{ return not FGC::ToNearUInt(lhs); } \
	

// enable/disable checks for enums
#ifdef COMPILER_MSVC
#	define BEGIN_ENUM_CHECKS() \
		__pragma (warning (push)) \
		__pragma (warning (error: 4061)) /*enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label*/ \
		__pragma (warning (error: 4062)) /*enumerator 'identifier' in switch of enum 'enumeration' is not handled*/ \
		__pragma (warning (error: 4063)) /*case 'number' is not a valid value for switch of enum 'type'*/ \

#	define END_ENUM_CHECKS() \
		__pragma (warning (pop)) \

#elif defined(COMPILER_CLANG)
#	define BEGIN_ENUM_CHECKS() \
		 _Pragma( "clang diagnostic error \"-Wswitch\"" )

#	define END_ENUM_CHECKS() \
		 _Pragma( "clang diagnostic ignored \"-Wswitch\"" )

#else
#	define BEGIN_ENUM_CHECKS()
#	define END_ENUM_CHECKS()

#endif


// allocator
#ifdef COMPILER_MSVC
#	define FG_ALLOCATOR		__declspec( allocator )
#else
#	define FG_ALLOCATOR
#endif


// exclusive lock
#ifndef EXLOCK
#	define EXLOCK( _syncObj_ ) \
		std::unique_lock	FG_PRIVATE_UNITE_RAW( __scopeLock, __COUNTER__ ) { _syncObj_ }
#endif

// shared lock
#ifndef SHAREDLOCK
#	define SHAREDLOCK( _syncObj_ ) \
		std::shared_lock	FG_PRIVATE_UNITE_RAW( __sharedLock, __COUNTER__ ) { _syncObj_ }
#endif


// thiscall, cdecl
#ifdef COMPILER_MSVC
#	define FG_CDECL		__cdecl
#	define FG_THISCALL	__thiscall

#elif defined(COMPILER_CLANG) or defined(COMPILER_GCC)
#	define FG_CDECL		__attribute__((cdecl))
#	define FG_THISCALL	__attribute__((thiscall))
#endif


// to fix compiler error C2338
#ifdef COMPILER_MSVC
#	define _ENABLE_EXTENDED_ALIGNED_STORAGE
#endif


// compile time messages
#ifndef FG_COMPILATION_MESSAGE
#	if defined(COMPILER_CLANG)
#		define FG_PRIVATE_MESSAGE_TOSTR(x)	#x
#		define FG_COMPILATION_MESSAGE( _message_ )	_Pragma(FG_PRIVATE_MESSAGE_TOSTR( GCC warning ("" _message_) ))

#	elif defined(COMPILER_MSVC)
#		define FG_COMPILATION_MESSAGE( _message_ )	__pragma(message( _message_ ))

#	else
#		define FG_COMPILATION_MESSAGE( _message_ )	// not supported
#	endif
#endif


// setup for build on CI
#ifdef FG_CI_BUILD

#	undef  FG_PRIVATE_BREAK_POINT
#	define FG_PRIVATE_BREAK_POINT()	{}

#	undef  FG_PRIVATE_CHECK
#	define FG_PRIVATE_CHECK( _expr_, _text_ ) \
		{if ( !(_expr_) ) { \
			FG_LOGI( _text_ ); \
			FG_PRIVATE_EXIT(); \
		}}

#	undef  FG_PRIVATE_CHECK_ERR
#	define FG_PRIVATE_CHECK_ERR( _expr_, _ret_ ) \
		{if ( !(_expr_) ) { \
			FG_LOGI( FG_PRIVATE_TOSTRING( _expr_ )); \
			FG_PRIVATE_EXIT(); \
		}}

#	undef  CHECK_FATAL
#	define CHECK_FATAL( _expr_ ) \
		{if ( !(_expr_) ) { \
			FG_LOGI( FG_PRIVATE_TOSTRING( _expr_ ) ); \
			FG_PRIVATE_EXIT(); \
		}}

#	undef  FG_PRIVATE_RETURN_ERR
#	define FG_PRIVATE_RETURN_ERR( _text_, _ret_ ) \
		{FG_LOGI( _text_ ); \
		 FG_PRIVATE_EXIT(); \
		}

#endif


// replace assertions by exceptions
#ifndef FG_NO_EXCEPTIONS

#	include <stdexcept>

#	undef  FG_PRIVATE_BREAK_POINT
#	define FG_PRIVATE_BREAK_POINT()	{}

#	undef  FG_LOGE
#	define FG_LOGE	FG_LOGI

	// keep ASSERT and CHECK behaviour because they may be used in destructor
	// but override CHECK_ERR, CHECK_FATAL and RETURN_ERR to allow user to handle this errors

#	undef  FG_PRIVATE_CHECK_ERR
#	define FG_PRIVATE_CHECK_ERR( _expr_, _ret_ ) \
		{if ( !(_expr_) ) { \
			throw FGC::FGException{ FG_PRIVATE_TOSTRING( _expr_ )}; \
		}}

#	undef  CHECK_FATAL
#	define CHECK_FATAL( _expr_ ) \
		{if ( !(_expr_) ) { \
			throw FGC::FGException{ FG_PRIVATE_TOSTRING( _expr_ )}; \
		}}

#	undef  FG_PRIVATE_RETURN_ERR
#	define FG_PRIVATE_RETURN_ERR( _text_, _ret_ ) \
		{throw FGC::FGException{ _text_ };}

#endif


// check definitions
#ifdef FG_CPP_DETECT_MISMATCH

#  ifdef FG_OPTIMAL_MEMORY_ORDER
#	pragma detect_mismatch( "FG_OPTIMAL_MEMORY_ORDER", "1" )
#  else
#	pragma detect_mismatch( "FG_OPTIMAL_MEMORY_ORDER", "0" )
#  endif

#  ifdef FG_DEBUG
#	pragma detect_mismatch( "FG_DEBUG", "1" )
#  else
#	pragma detect_mismatch( "FG_DEBUG", "0" )
#  endif

#  if defined(FG_FAST_HASH) && FG_FAST_HASH
#	pragma detect_mismatch( "FG_FAST_HASH", "1" )
#  else
#	pragma detect_mismatch( "FG_FAST_HASH", "0" )
#  endif

#  ifdef FG_ENABLE_DATA_RACE_CHECK
#	pragma detect_mismatch( "FG_ENABLE_DATA_RACE_CHECK", "1" )
#  else
#	pragma detect_mismatch( "FG_ENABLE_DATA_RACE_CHECK", "0" )
#  endif

#  ifdef FG_STD_STRINGVIEW
#	pragma detect_mismatch( "FG_STD_STRINGVIEW", "1" )
#  else
#	pragma detect_mismatch( "FG_STD_STRINGVIEW", "0" )
#  endif

#  ifdef FG_STD_OPTIONAL
#	pragma detect_mismatch( "FG_STD_OPTIONAL", "1" )
#  else
#	pragma detect_mismatch( "FG_STD_OPTIONAL", "0" )
#  endif

#  ifdef FG_STD_VARIANT
#	pragma detect_mismatch( "FG_STD_VARIANT", "1" )
#  else
#	pragma detect_mismatch( "FG_STD_VARIANT", "0" )
#  endif

#  ifdef FG_STD_BARRIER
#	pragma detect_mismatch( "FG_STD_BARRIER", "1" )
#  else
#	pragma detect_mismatch( "FG_STD_BARRIER", "0" )
#  endif

#  ifdef FG_CI_BUILD
#	pragma detect_mismatch( "FG_CI_BUILD", "1" )
#  else
#	pragma detect_mismatch( "FG_CI_BUILD", "0" )
#  endif

#  ifdef FG_NO_EXCEPTIONS
#	pragma detect_mismatch( "FG_NO_EXCEPTIONS", "1" )
#  else
#	pragma detect_mismatch( "FG_NO_EXCEPTIONS", "0" )
#  endif

#endif	// FG_CPP_DETECT_MISMATCH

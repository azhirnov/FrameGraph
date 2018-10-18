// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once


#if defined(DEBUG) || defined(_DEBUG)
#	define FG_DEBUG
#else
#	define FG_RELEASE
#endif

#define FG_FAST_HASH	0


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
#	define ND_				[[nodiscard]]
#endif


// null pointer
#ifndef null
#	define null				nullptr
#endif


// force inline
#ifndef forceinline
# ifdef FG_DEBUG
#	define forceinline		inline

# elif defined COMPILER_MSVC
#	define forceinline		__forceinline

# elif defined(COMPILER_CLANG) or defined(COMPILER_GCC)
#	define forceinline		__inline__ __attribute__((always_inline))

# endif
#endif


// macro for unused variables
#ifndef FG_UNUSED
#	define FG_UNUSED( ... )		(void)( __VA_ARGS__ )
#endif


// debug break
#ifndef FG_BREAK_POINT
# if defined(COMPILER_MSVC)
#	define FG_PRIVATE_BREAK_POINT()		__debugbreak()

# elif defined(COMPILER_CLANG) or defined(COMPILER_GCC)
#  if 1
#	include <exception>
#	define FG_PRIVATE_BREAK_POINT() 	throw std::runtime_error("breakpoint")
#  elif 0
#	include <csignal>
#	define FG_PRIVATE_BREAK_POINT()		std::raise(SIGINT)
#  endif
# endif
#endif


// DLL import/export
#if !defined(FG_DLL_EXPORT) || !defined(FG_DLL_IMPORT)
# if defined(COMPILER_MSVC)
#	define FG_DLL_EXPORT			__declspec( dllexport )
#	define FG_DLL_IMPORT			__declspec( dllimport )

# elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#  ifdef PLATFORM_WINDOWS
#	define FG_DLL_EXPORT			__attribute__ (dllexport)
#	define FG_DLL_IMPORT			__attribute__ (dllimport)
#  else
#	define FG_DLL_EXPORT			__attribute__ (visibility("default"))
#	define FG_DLL_IMPORT			__attribute__ (visibility("default"))
#  endif

# else
#	error define FG_DLL_EXPORT and FG_DLL_IMPORT for you compiler
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
        {if (( _expr_ )) {} \
		 else { \
			FG_LOGE( _text_ ); \
		}}

#   define CHECK( _func_ ) \
        FG_PRIVATE_CHECK( (_func_), FG_PRIVATE_TOSTRING( _func_ ) )
#endif


// check function return value and return error code
#ifndef CHECK_ERR
#	define FG_PRIVATE_CHECK_ERR( _expr_, _ret_ ) \
        {if (( _expr_ )) {}\
		  else { \
            FG_LOGE( FG_PRIVATE_TOSTRING( _expr_ ) ); \
			return (_ret_); \
		}}

#	define CHECK_ERR( ... ) \
        FG_PRIVATE_CHECK_ERR( FG_PRIVATE_GETARG_0( __VA_ARGS__ ), FG_PRIVATE_GETARG_1( __VA_ARGS__, ::FG::Default ) )
#endif


// check function return value and exit
#ifndef CHECK_FATAL
#	define CHECK_FATAL( _expr_ ) \
        {if (( _expr_ )) {}\
		  else { \
            FG_LOGE( FG_PRIVATE_TOSTRING( _expr_ ) ); \
			::exit( EXIT_FAILURE ); \
		}}
#endif


// return error code
#ifndef RETURN_ERR
#	define FG_PRIVATE_RETURN_ERR( _text_, _ret_ ) \
		{ FG_LOGE( _text_ );  return (_ret_); }

#	define RETURN_ERR( ... ) \
        FG_PRIVATE_RETURN_ERR( FG_PRIVATE_GETARG_0( __VA_ARGS__ ), FG_PRIVATE_GETARG_1( __VA_ARGS__, ::FG::Default ) )
#endif


// compile time assert
#ifndef STATIC_ASSERT
#	define STATIC_ASSERT( ... ) \
        static_assert(	FG_PRIVATE_GETRAW( FG_PRIVATE_GETARG_0( __VA_ARGS__ ) ), \
                        FG_PRIVATE_GETRAW( FG_PRIVATE_GETARG_1( __VA_ARGS__, FG_PRIVATE_TOSTRING(__VA_ARGS__) ) ) )
#endif


// bit operators
#define FG_BIT_OPERATORS( _type_ ) \
	ND_ constexpr _type_  operator |  (_type_ lhs, _type_ rhs)	noexcept	{ return _type_( FG::EnumToUInt(lhs) | FG::EnumToUInt(rhs) ); } \
	ND_ constexpr _type_  operator &  (_type_ lhs, _type_ rhs)	noexcept	{ return _type_( FG::EnumToUInt(lhs) & FG::EnumToUInt(rhs) ); } \
	\
	constexpr _type_&  operator |= (_type_ &lhs, _type_ rhs)	noexcept	{ return lhs = _type_( FG::EnumToUInt(lhs) | FG::EnumToUInt(rhs) ); } \
	constexpr _type_&  operator &= (_type_ &lhs, _type_ rhs)	noexcept	{ return lhs = _type_( FG::EnumToUInt(lhs) & FG::EnumToUInt(rhs) ); } \
	\
	ND_ constexpr _type_  operator ~ (_type_ lhs) noexcept					{ return _type_(~FG::EnumToUInt(lhs)); } \
	ND_ constexpr bool    operator ! (_type_ lhs) noexcept					{ return not FG::EnumToUInt(lhs); } \
	

// enable/disable checks for enums
#ifdef COMPILER_MSVC
#	define ENABLE_ENUM_CHECKS() \
		__pragma (warning (push)) \
        __pragma (warning (error: 4061)) /*enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label*/ \
        __pragma (warning (error: 4062)) /*enumerator 'identifier' in switch of enum 'enumeration' is not handled*/ \
        __pragma (warning (error: 4063)) /*case 'number' is not a valid value for switch of enum 'type'*/ \

#	define DISABLE_ENUM_CHECKS() \
		__pragma (warning (pop)) \

#elif defined(COMPILER_CLANG) or defined(COMPILER_GCC)
#	define ENABLE_ENUM_CHECKS()		// TODO
#	define DISABLE_ENUM_CHECKS()	// TODO

#else
#	define ENABLE_ENUM_CHECKS()
#	define DISABLE_ENUM_CHECKS()

#endif


// allocator
#ifdef COMPILER_MSVC
#	define FG_ALLOCATOR		__declspec( allocator )
#else
#	define FG_ALLOCATOR
#endif



#ifndef SCOPELOCK
#	define SCOPELOCK( _syncObj_ ) \
		std::lock_guard		FG_PRIVATE_UNITE_RAW( __scopeLock, __COUNTER__ ) ( _syncObj_ )
#endif

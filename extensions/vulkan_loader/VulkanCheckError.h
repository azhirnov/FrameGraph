// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once


#ifndef FG_DEBUG
#	define VK_CALL( ... )		{ (void)(__VA_ARGS__); }
#	define VK_CHECK( ... )		{ (void)(__VA_ARGS__); }

#else
#	define VK_CALL( ... ) \
	{ \
		const ::VkResult __vk_err__ = (__VA_ARGS__); \
		::FG::__vk_CheckErrors( __vk_err__, FG_PRIVATE_TOSTRING( __VA_ARGS__ ), FG_FUNCTION_NAME, __FILE__, __LINE__ ); \
	}

#	define FG_PRIVATE_VK_CALL_R( _func_, _ret_, ... ) \
	{ \
		const ::VkResult __vk_err__ = (_func_); \
		if ( not ::FG::__vk_CheckErrors( __vk_err__, FG_PRIVATE_TOSTRING( _func_ ), FG_FUNCTION_NAME, __FILE__, __LINE__ ) ) \
			return _ret_; \
	}

#	define VK_CHECK( ... ) \
		FG_PRIVATE_VK_CALL_R( FG_PRIVATE_GETARG_0( __VA_ARGS__ ), FG_PRIVATE_GETARG_1( __VA_ARGS__, ::FG::Default ) )
#endif


namespace FG
{

	bool __vk_CheckErrors (VkResult errCode, const char *vkcall, const char *func, const char *file, int line);
	
}	// FG

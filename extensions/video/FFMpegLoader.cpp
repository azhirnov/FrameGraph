// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef FG_ENABLE_FFMPEG

#include "video/FFMpegLoader.h"
#include "stl/Containers/Singleton.h"

# if defined(PLATFORM_WINDOWS) or defined(VK_USE_PLATFORM_WIN32_KHR)

#	include "stl/Platforms/WindowsHeader.h"

	using SharedLib_t	= HMODULE;

# else
//#elif defined(PLATFORM_LINUX) or defined(PLATFORM_ANDROID)
#	include <dlfcn.h>
#   include <linux/limits.h>

	using SharedLib_t	= void*;
# endif

namespace FG
{
	using namespace FGC;
	
/*
=================================================
	FFMpegLib
=================================================
*/
namespace {
	struct FFMpegLib
	{
		SharedLib_t		avcodec		= null;
		SharedLib_t		avformat	= null;
		SharedLib_t		avutil		= null;
		SharedLib_t		swscale		= null;
		bool			loaded		= false;
		int				refCounter	= 0;
	};
}
/*
=================================================
	Load
=================================================
*/
	bool FFMpegLoader::Load ()
	{
		FFMpegLib&	lib = *Singleton< FFMpegLib >();
		
		if ( lib.loaded and lib.refCounter > 0 )
		{
			++lib.refCounter;
			return true;
		}

		// avcodec
		{
		#ifdef PLATFORM_WINDOWS
			lib.avcodec = ::LoadLibraryA( "avcodec-58.dll" );
			#define GET_AVCODEC_FN( _name_ ) \
				FFMpegLoader::_name_ = BitCast< decltype(&::_name_) >( ::GetProcAddress( lib.avcodec, FG_PRIVATE_TOSTRING(_name_) )); \
				loaded &= (FFMpegLoader::_name_ != null);
		#else
			lib.avcodec = ::dlopen( "libavcodec-58.so", RTLD_NOW | RTLD_LOCAL );
			#define GET_AVCODEC_FN( _name_ ) \
				FFMpegLoader::_name_ = BitCast< decltype(&::_name_) >( ::dlsym( lib.avcodec, FG_PRIVATE_TOSTRING(_name_) )); \
				loaded &= (FFMpegLoader::_name_ != null);
		#endif
			
			bool	loaded = true;

			CHECK_ERR( lib.avcodec );
			FG_FFMPEG_AVCODEC_FUNCS( GET_AVCODEC_FN );
			CHECK_ERR( loaded );
		}

		// avformat
		{
		#ifdef PLATFORM_WINDOWS
			lib.avformat = ::LoadLibraryA( "avformat-58.dll" );
			#define GET_AVFORMAT_FN( _name_ ) \
				FFMpegLoader::_name_ = BitCast< decltype(&::_name_) >( ::GetProcAddress( lib.avformat, FG_PRIVATE_TOSTRING(_name_) )); \
				loaded &= (FFMpegLoader::_name_ != null);
		#else
			lib.avformat = ::dlopen( "libavformat-58.so", RTLD_NOW | RTLD_LOCAL );
			#define GET_AVFORMAT_FN( _name_ ) \
				FFMpegLoader::_name_ = BitCast< decltype(&::_name_) >( ::dlsym( lib.avformat, FG_PRIVATE_TOSTRING(_name_) )); \
				loaded &= (FFMpegLoader::_name_ != null);
		#endif
			
			bool	loaded = true;

			CHECK_ERR( lib.avformat );
			FG_FFMPEG_AVFORMAT_FUNCS( GET_AVFORMAT_FN );
			CHECK_ERR( loaded );
		}

		// avutil
		{
		#ifdef PLATFORM_WINDOWS
			lib.avutil = ::LoadLibraryA( "avutil-56.dll" );
			#define GET_AVUTIL_FN( _name_ ) \
				FFMpegLoader::_name_ = BitCast< decltype(&::_name_) >( ::GetProcAddress( lib.avutil, FG_PRIVATE_TOSTRING(_name_) )); \
				loaded &= (FFMpegLoader::_name_ != null);
		#else
			lib.avutil = ::dlopen( "libavutil-56.so", RTLD_NOW | RTLD_LOCAL );
			#define GET_AVUTIL_FN( _name_ ) \
				FFMpegLoader::_name_ = BitCast< decltype(&::_name_) >( ::dlsym( lib.avutil, FG_PRIVATE_TOSTRING(_name_) )); \
				loaded &= (FFMpegLoader::_name_ != null);
		#endif
			
			bool	loaded = true;

			CHECK_ERR( lib.avutil );
			FG_FFMPEG_AVUTIL_FUNCS( GET_AVUTIL_FN );
			CHECK_ERR( loaded );
		}

		// swscale
		{
		#ifdef PLATFORM_WINDOWS
			lib.swscale = ::LoadLibraryA( "swscale-5.dll" );
			#define GET_SWSCALE_FN( _name_ ) \
				FFMpegLoader::_name_ = BitCast< decltype(&::_name_) >( ::GetProcAddress( lib.swscale, FG_PRIVATE_TOSTRING(_name_) )); \
				loaded &= (FFMpegLoader::_name_ != null);
		#else
			lib.swscale = ::dlopen( "libswscale-5.so", RTLD_NOW | RTLD_LOCAL );
			#define GET_SWSCALE_FN( _name_ ) \
				FFMpegLoader::_name_ = BitCast< decltype(&::_name_) >( ::dlsym( lib.swscale, FG_PRIVATE_TOSTRING(_name_) )); \
				loaded &= (FFMpegLoader::_name_ != null);
		#endif
			
			bool	loaded = true;
			
			CHECK_ERR( lib.swscale );
			FG_FFMPEG_SWSCALE_FUNCS( GET_SWSCALE_FN );
			CHECK_ERR( loaded );
		}
		
		lib.refCounter	= 1;
		lib.loaded		= true;
		return true;
	}
	
/*
=================================================
	Unload
=================================================
*/
	void FFMpegLoader::Unload ()
	{
		FFMpegLib&	lib = *Singleton< FFMpegLib >();
		
		ASSERT( lib.refCounter > 0 );

		if ( (--lib.refCounter) != 0 )
			return;
		
		#ifdef PLATFORM_WINDOWS
			if ( lib.avcodec != null )
				::FreeLibrary( lib.avcodec );
			
			if ( lib.avformat != null )
				::FreeLibrary( lib.avformat );

			if ( lib.avutil != null )
				::FreeLibrary( lib.avutil );
			
			if ( lib.swscale != null )
				::FreeLibrary( lib.swscale );
		#else

			if ( lib.avcodec != null )
				::dlclose( lib.avcodec );
			
			if ( lib.avformat != null )
				::dlclose( lib.avformat );

			if ( lib.avutil != null )
				::dlclose( lib.avutil );

			if ( lib.swscale != null )
				::dlclose( lib.swscale );
		#endif

		lib.avcodec		= null;
		lib.avformat	= null;
		lib.avutil		= null;
		lib.swscale		= null;

		#define ZERO_FN( _name_ ) \
			FFMpegLoader::_name_ = null;

		FG_FFMPEG_AVCODEC_FUNCS( ZERO_FN );
		FG_FFMPEG_AVFORMAT_FUNCS( ZERO_FN );
		FG_FFMPEG_AVUTIL_FUNCS( ZERO_FN );
		FG_FFMPEG_SWSCALE_FUNCS( ZERO_FN );
	}

}	// FG

#endif	// FG_ENABLE_FFMPEG

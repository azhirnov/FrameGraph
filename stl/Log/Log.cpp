// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Algorithms/StringUtils.h"
#include <iostream>
#include <shared_mutex>

using namespace FGC;

namespace {
/*
=================================================
	ToShortPath
=================================================
*/
	ND_ inline StringView  ToShortPath (StringView file)
	{
		const uint	max_parts = 2;

		size_t	i = file.length()-1;
		uint	j = 0;

		for (; i < file.length() and j < max_parts; --i)
		{
			const char	c = file[i];

			if ( (c == '\\') | (c == '/') )
				++j;
		}

		return file.substr( i + (j == max_parts ? 2 : 0) );
	}

/*
=================================================
	ConsoleOutput
=================================================
*/
	inline void ConsoleOutput (StringView message, StringView file, int line, bool isError)
	{
		const String str = String{ ToShortPath( file )} << '(' << ToString( line ) << "): " << message;

		if ( isError )
			std::cerr << "\x1B[31m" << str << "\033[0m" << std::endl;
		else
			std::cout << str << std::endl;
	}

}	// namespace

/*
=================================================
	Info / Error
=================================================
*/
	Logger::EResult  FGC::Logger::Info (const char *msg, const char *func, const char *file, int line)
	{
		return Info( StringView{msg}, StringView{func}, StringView{file}, line );
	}

	Logger::EResult  FGC::Logger::Error (const char *msg, const char *func, const char *file, int line)
	{
		return Error( StringView{msg}, StringView{func}, StringView{file}, line );
	}
	
/*
=================================================
	ExternalLogger
=================================================
*/
namespace {
	static FGC::SharedMutex			s_LogCallbackGuard;
	static FGC::Logger::Callback_t	s_LogCallback			= null;
	static void *					s_LogCallbackUserData	= null;

	static void  ExternalLogger (const StringView &msg, const StringView &file, int line, bool isError)
	{
		SHAREDLOCK( s_LogCallbackGuard );

		if ( s_LogCallback )
			s_LogCallback( s_LogCallbackUserData, msg, file, line, isError );
	}
}
/*
=================================================
	SetCallback
=================================================
*/

	void  FGC::Logger::SetCallback (Callback_t cb, void* userData)
	{
		EXLOCK( s_LogCallbackGuard );
		s_LogCallback			= cb;
		s_LogCallbackUserData	= userData;
	}
//-----------------------------------------------------------------------------



// Android
#if	defined(PLATFORM_ANDROID)

#	include <android/log.h>

namespace {
	static constexpr char	FG_ANDROID_TAG[] = "FrameGraph";
}

/*
=================================================
	Info
=================================================
*/
	Logger::EResult  FGC::Logger::Info (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		ExternalLogger( msg, file, line, false );
		(void)__android_log_print( ANDROID_LOG_INFO, FG_ANDROID_TAG, "%s (%i): %s", ToShortPath( file ).data(), line, msg.data() );
		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Logger::EResult  FGC::Logger::Error (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		ExternalLogger( msg, file, line, true );
		(void)__android_log_print( ANDROID_LOG_ERROR, FG_ANDROID_TAG, "%s (%i): %s", ToShortPath( file ).data(), line, msg.data() );
		return EResult::Continue;
	}
//-----------------------------------------------------------------------------



// Windows
#elif defined(PLATFORM_WINDOWS)
	
#	include "stl/Platforms/WindowsHeader.h"

/*
=================================================
	IDEConsoleMessage
=================================================
*/
namespace {
	inline void IDEConsoleMessage (StringView message, StringView file, int line, bool isError)
	{
	#ifdef COMPILER_MSVC
		const String	str = String{file} << '(' << ToString( line ) << "): " << (isError ? "Error: " : "") << message << '\n';

		::OutputDebugStringA( str.c_str() );
	#endif
	}
}
/*
=================================================
	Info
=================================================
*/
	Logger::EResult  FGC::Logger::Info (const StringView &msg, const StringView &, const StringView &file, int line)
	{
		IDEConsoleMessage( msg, file, line, false );
		ConsoleOutput( msg, file, line, false );
		ExternalLogger( msg, file, line, false );

		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Logger::EResult  FGC::Logger::Error (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		IDEConsoleMessage( msg, file, line, true );
		ConsoleOutput( msg, file, line, true );
		ExternalLogger( msg, file, line, true );

		const String	caption	= "Error message";

		const String	str		= "File: "s << ToShortPath( file ) <<
								  "\nLine: " << ToString( line ) <<
								  "\nFunction: " << func <<
								  "\n\nMessage:\n" << msg;

		int	result = ::MessageBoxExA( null, str.c_str(), caption.c_str(),
									  MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST | MB_DEFBUTTON3,
									  MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ));
		switch ( result )
		{
			case IDABORT :	return Logger::EResult::Abort;
			case IDRETRY :	return Logger::EResult::Break;
			case IDIGNORE :	return Logger::EResult::Continue;
			default :		return Logger::EResult(~0u);
		};
	}
//-----------------------------------------------------------------------------



// OSX
#elif defined(PLATFORM_MACOS) or defined(PLATFORM_IOS)

/*
=================================================
	Info
=================================================
*/
	Logger::EResult  FGC::Logger::Info (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		ConsoleOutput( msg, file, line, false );
		ExternalLogger( msg, file, line, false );
		return EResult::Continue;
	}

/*
=================================================
	Error
=================================================
*/
	Logger::EResult  FGC::Logger::Error (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		ConsoleOutput( msg, file, line, true );
		ExternalLogger( msg, file, line, true );
		return EResult::Abort;
	}
//-----------------------------------------------------------------------------



// Linux
#elif defined(PLATFORM_LINUX)

/*
=================================================
	Info
=================================================
*/
	Logger::EResult  FGC::Logger::Info (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		ConsoleOutput( msg, file, line, false );
		ExternalLogger( msg, file, line, false );
		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Logger::EResult  FGC::Logger::Error (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		ConsoleOutput( msg, file, line, true );
		ExternalLogger( msg, file, line, true );
		return EResult::Continue;
	}
//-----------------------------------------------------------------------------


#else

#	error unsupported platform

#endif

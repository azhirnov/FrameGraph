// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Algorithms/StringUtils.h"
#include <iostream>

using namespace FG;


/*
=================================================
	ConsoleOutput
=================================================
*/
	inline void ConsoleOutput (StringView message, StringView file, int line, bool isError)
	{
		const String str = String{file} << '(' << ToString( line ) << "): " << message;

		if ( isError )
			std::cerr << str << std::endl;
		else
			std::cout << str << std::endl;
	}
	
/*
=================================================
	
=================================================
*/
	Log::EResult  FG::Log::Info (const char *msg, const char *func, const char *file, int line)
	{
		return Info( StringView{msg}, StringView{func}, StringView{file}, line );
	}

	Log::EResult  FG::Log::Error (const char *msg, const char *func, const char *file, int line)
	{
		return Error( StringView{msg}, StringView{func}, StringView{file}, line );
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
	Log::EResult  FG::Log::Info (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		(void)__android_log_print( ANDROID_LOG_WARN, FG_ANDROID_TAG, "%s (%i): %s", file.data(), line, msg.data() );
		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Log::EResult  FG::Log::Error (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		(void)__android_log_print( ANDROID_LOG_ERROR, FG_ANDROID_TAG, "%s (%i): %s", file.data(), line, msg.data() );
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
	static void IDEConsoleMessage (StringView message, StringView file, int line, bool isError)
	{
	#ifdef COMPILER_MSVC
		const String	str = String(file) << '(' << ToString( line ) << "): " << (isError ? "Error: " : "") << message << '\n';

		::OutputDebugStringA( str.data() );
	#endif
	}
	
/*
=================================================
	Info
=================================================
*/
	Log::EResult  FG::Log::Info (const StringView &msg, const StringView &, const StringView &file, int line)
	{
		IDEConsoleMessage( msg, file, line, false );
		ConsoleOutput( msg, file, line, false );

		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Log::EResult  FG::Log::Error (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		IDEConsoleMessage( msg, file, line, true );
		ConsoleOutput( msg, file, line, true );

		const String	caption	= "Error message";

		const String	str		= "File: "s << file <<
								  "\nLine: " << ToString( line ) <<
								  "\nFunction: " << func <<
								  "\n\nMessage:\n" << msg;

		int	result = ::MessageBoxExA( null, str.data(), caption.data(),
									  MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST | MB_DEFBUTTON3,
									  MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ) );
		switch ( result )
		{
			case IDABORT :		return Log::EResult::Abort;
			case IDRETRY :		return Log::EResult::Break;
			case IDIGNORE :		return Log::EResult::Continue;
			default :			return Log::EResult(~0u);
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
	Log::EResult  FG::Log::Info (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		ConsoleOutput( msg, file, line, false );
		return EResult::Continue;
	}

/*
=================================================
	Error
=================================================
*/
	Log::EResult  FG::Log::Error (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		ConsoleOutput( msg, file, line, true );
		return EResult::Abort;
	}
//-----------------------------------------------------------------------------



// Linux
#elif defined(PLATFORM_LINUX)

// sudo apt-get install lesstif2-dev
//#include <Xm/Xm.h>
//#include <Xm/PushB.h>

/*
=================================================
	Info
=================================================
*/
	Log::EResult  FG::Log::Info (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		ConsoleOutput( msg, file, line, false );
		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Log::EResult  FG::Log::Error (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		/*Widget top_wid, button;
		XtAppContext  app;

		top_wid = XtVaAppInitialize(&app, "Push", NULL, 0,
			&argc, argv, NULL, NULL);

		button = XmCreatePushButton(top_wid, "Push_me", NULL, 0);

		/* tell Xt to manage button * /
					XtManageChild(button);

					/* attach fn to widget * /
		XtAddCallback(button, XmNactivateCallback, pushed_fn, NULL);

		XtRealizeWidget(top_wid); /* display widget hierarchy * /
		XtAppMainLoop(app); /* enter processing loop * /
	*/
		ConsoleOutput( msg, file, line, true );
		return EResult::Break;
	}
//-----------------------------------------------------------------------------



// FreeBSD
#elif 0	// TODO

/*
=================================================
	Info
=================================================
*/
	Log::EResult  FG::Log::Info (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		ConsoleOutput( msg, file, line, false );
		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Log::EResult  FG::Log::Error (const StringView &msg, const StringView &func, const StringView &file, int line)
	{
		ConsoleOutput( msg, file, line, true );
		return EResult::Abort;
	}
//-----------------------------------------------------------------------------


#else

#	error unsupported platform

#endif

// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "stl/include/StringUtils.h"
#include "stl/include/ToString.h"
#include <iostream>

using namespace FG;


/*
=================================================
    ConsoleOutput
=================================================
*/
    static void ConsoleOutput (StringView message, StringView file, int line, bool isError)
    {
		const String str = String(file) << '(' << ToString( line ) << "): " << message;

        if ( isError )
            std::cerr << str << std::endl;
        else
            std::cout << str << std::endl;
    }

//-----------------------------------------------------------------------------



// Android
#if	(defined( ANDROID ) or defined( __ANDROID__ ))

#	include <android/log.h>

/*
=================================================
	Info
=================================================
*/
	Log::EResult  FG::Log::Info (StringView msg, StringView func, StringView file, int line)
	{
        ConsoleOutput( msg, file, line, false );
		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Log::EResult  FG::Log::Error (StringView msg, StringView func, StringView file, int line)
	{
        ConsoleOutput( msg, file, line, true );
		return EResult::Abort;
	}
//-----------------------------------------------------------------------------



// Windows
#elif (defined( _WIN32 ) || defined( _WIN64 ) || defined( WIN32 ) || defined( WIN64 ) || \
	   defined( __CYGWIN__ ) || defined( __MINGW32__ ) || defined( __MINGW32__ ))

# ifdef _MSC_VER
#	pragma warning (push, 0)
#	include <Windows.h>
#	pragma warning (pop)
# else
#	include <Windows.h>
# endif

/*
=================================================
	IDEConsoleMessage
=================================================
*/
    static void IDEConsoleMessage (StringView message, StringView file, int line, bool isError)
	{
	#ifdef _MSC_VER
        const String	str = String(file) << '(' << ToString( line ) << "): " << (isError ? "Error: " : "") << message << '\n';

		::OutputDebugStringA( str.data() );
	#endif
	}
	
/*
=================================================
	Info
=================================================
*/
	Log::EResult  FG::Log::Info (StringView msg, StringView, StringView file, int line)
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
	Log::EResult  FG::Log::Error (StringView msg, StringView func, StringView file, int line)
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
#elif (defined( __APPLE__ ) || defined( MACOSX )) 

/*
=================================================
	Info
=================================================
*/
	Log::EResult  FG::Log::Info (StringView msg, StringView func, StringView file, int line)
	{
        ConsoleOutput( msg, file, line, false );
		return EResult::Continue;
	}

/*
=================================================
	Error
=================================================
*/
	Log::EResult  FG::Log::Error (StringView msg, StringView func, StringView file, int line)
	{
        ConsoleOutput( msg, file, line, true );
		return EResult::Abort;
	}
//-----------------------------------------------------------------------------



// Linux
#elif (defined( __linux__ ) || defined( __gnu_linux__ ))

// sudo apt-get install lesstif2-dev
//#include <Xm/Xm.h>
//#include <Xm/PushB.h>

/*
=================================================
	Info
=================================================
*/
	Log::EResult  FG::Log::Info (StringView msg, StringView func, StringView file, int line)
    {
        ConsoleOutput( msg, file, line, false );
		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Log::EResult  FG::Log::Error (StringView msg, StringView func, StringView file, int line)
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
#elif (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__))

/*
=================================================
	Info
=================================================
*/
	Log::EResult  FG::Log::Info (StringView msg, StringView func, StringView file, int line)
	{
        ConsoleOutput( msg, file, line, false );
		return EResult::Continue;
	}
	
/*
=================================================
	Error
=================================================
*/
	Log::EResult  FG::Log::Error (StringView msg, StringView func, StringView file, int line)
	{
        ConsoleOutput( msg, file, line, true );
		return EResult::Abort;
	}
//-----------------------------------------------------------------------------


#else

#	error unsupported platform

#endif

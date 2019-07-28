// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Platforms/ThreadName.h"
#include "stl/Platforms/WindowsHeader.h"


namespace FGC
{
namespace
{
#ifdef PLATFORM_WINDOWS
	#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.  
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	 } THREADNAME_INFO;
	#pragma pack(pop)

/*
=================================================
	SetCurrentThreadNameImpl
=================================================
*/
	static void SetCurrentThreadNameImpl (const char* name)
	{
		constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;

		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = name;
		info.dwThreadID = ::GetCurrentThreadId();
		info.dwFlags = 0;

	#pragma warning(push)
	#pragma warning(disable: 6320 6322)
		__try{
			::RaiseException( MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		}
	#pragma warning(pop)
	}
//-----------------------------------------------------------------------------
#else

	static void SetCurrentThreadNameImpl (const char*)
	{
		FG_COMPILATION_MESSAGE( "SetCurrentThreadName() - not supported for current platform" )
	}

#endif
}	// namespace


/*
=================================================
	SetCurrentThreadName
=================================================
*/
	void SetCurrentThreadName (NtStringView name)
	{
		SetCurrentThreadNameImpl( name.c_str() );
	}

}	// FGC

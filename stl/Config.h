// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#	define FG_DEBUG
#else
#	define FG_RELEASE
#endif


#ifdef FG_DEBUG
#	define FG_ENABLE_DATA_RACE_CHECK
#else
#	define FG_OPTIMAL_MEMORY_ORDER
#endif


// mem leak check
#if defined(COMPILER_MSVC) && defined(FG_ENABLE_MEMLEAK_CHECKS) && defined(FG_DEBUG)
#	define _CRTDBG_MAP_ALLOC
#	include <stdlib.h>
#	include <crtdbg.h>

	// call at exit
	// returns 'true' if no mem leaks
#	define FG_DUMP_MEMLEAKS()	(::_CrtDumpMemoryLeaks() != 1)
#else

#	define FG_DUMP_MEMLEAKS()	(true)
#endif


#define FG_FAST_HASH	0

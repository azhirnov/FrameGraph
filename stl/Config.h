// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once


#ifdef FG_DEBUG
#	define FG_ENABLE_DATA_RACE_CHECK
#else
//#	define FG_OPTIMAL_MEMORY_ORDER
#endif


#define FG_FAST_HASH	0

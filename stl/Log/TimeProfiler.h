// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/StringUtils.h"
#include <chrono>

namespace FG
{

	//
	// Time Profiler
	//

	struct TimeProfiler
	{
	private:
		using Clock_t		= std::chrono::high_resolution_clock;
		using TimePoint_t	= std::chrono::time_point< Clock_t >;

		String				_message;
		const TimePoint_t	_startTime	= Clock_t::now();
		const char *		_file		= null;
		const int			_line		= 0;


	public:
		TimeProfiler (StringView name)
		{
			_message = name;
		}


		TimeProfiler (StringView name, const char *func, const char *file, int line) :
			_file{ file }, _line{ line }
		{
			_message << "time profiler: " << name << (name.empty() ? "" : ", ") << "function: " << func;
		}
		

		~TimeProfiler ()
		{
			_message << "; TIME: " << ToString( Clock_t::now() - _startTime );

			if ( _file ) {
				FG_PRIVATE_LOGI( _message, _file, _line );
			}else{
				FG_LOGI( _message );
			}
		}
	};


#	define FG_TIMEPROFILER( ... ) \
		::FG::TimeProfiler	FG_PRIVATE_UNITE_RAW( __timeProf, __COUNTER__ ) ( \
								FG_PRIVATE_GETRAW( FG_PRIVATE_GETARG_0( "" __VA_ARGS__, "no name" ) ), \
								FG_FUNCTION_NAME, \
								__FILE__, \
								__LINE__ ) \


}	// FG

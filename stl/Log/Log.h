// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Defines.h"
#include "stl/Containers/StringViewFwd.h"

namespace FG
{
	

	//
	// Logger
	//

	struct Log
	{
		enum class EResult {
			Continue,
			Break,
			Abort,
		};
		
		static EResult  Info (const char *msg, const char *func, const char *file, int line);
		static EResult  Info (const StringView &msg, const StringView &func, const StringView &file, int line);

		static EResult  Error (const char *msg, const char *func, const char *file, int line);
		static EResult  Error (const StringView &msg, const StringView &func, const StringView &file, int line);
	};

}	// FG


#define FG_PRIVATE_LOGX( _level_, _msg_, _file_, _line_ ) \
	{switch ( ::FG::Log::_level_( (_msg_), (FG_FUNCTION_NAME), (_file_), (_line_) ) ) \
	{ \
		case ::FG::Log::EResult::Continue :	break; \
		case ::FG::Log::EResult::Break :	FG_PRIVATE_BREAK_POINT();	break; \
		case ::FG::Log::EResult::Abort :	FG_PRIVATE_EXIT();			break; \
	}}

#define FG_PRIVATE_LOGI( _msg_, _file_, _line_ )	FG_PRIVATE_LOGX( Info, (_msg_), (_file_), (_line_) )
#define FG_PRIVATE_LOGE( _msg_, _file_, _line_ )	FG_PRIVATE_LOGX( Error, (_msg_), (_file_), (_line_) )

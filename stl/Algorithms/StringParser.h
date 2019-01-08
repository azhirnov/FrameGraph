// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/StringUtils.h"

namespace FG
{

	//
	// String Parser
	//

	struct StringParser final
	{
	public:
			static void  ToEndOfLine	(StringView str, INOUT size_t &pos);
			static void  ToBeginOfLine	(StringView str, INOUT size_t &pos);
			static void  ToNextLine		(StringView str, INOUT size_t &pos);
			static void  ToPrevLine		(StringView str, INOUT size_t &pos);

		ND_ static bool  IsBeginOfLine	(StringView str, size_t pos);
		ND_ static bool  IsEndOfLine	(StringView str, size_t pos);

		ND_ static size_t CalculateNumberOfLines (StringView str);

			static bool  MoveToLine (StringView str, INOUT size_t &pos, size_t lineNumber);

			static void  ReadCurrLine (StringView str, INOUT size_t &pos, OUT StringView &result);
			static void  ReadLineToEnd (StringView str, INOUT size_t &pos, OUT StringView &result);

			static bool  ReadTo (StringView str, StringView endSymbol, INOUT size_t &pos, OUT StringView &result);

			static bool  ReadString (StringView str, INOUT size_t &pos, OUT StringView &result);
	};


}	// FG

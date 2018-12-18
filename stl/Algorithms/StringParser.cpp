// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Algorithms/StringParser.h"
#include "stl/Math/Math.h"
#include "stl/CompileTime/DefaultType.h"

namespace FG
{

/*
=================================================
	ToEndOfLine
=================================================
*/
	void StringParser::ToEndOfLine (StringView str, INOUT size_t &pos)
	{
		if ( pos < str.length() and (str[pos] == '\n' or str[pos] == '\r') )
			return;

		while ( pos < str.length() )
		{
			const char	n = (pos+1) >= str.length() ? 0 : str[pos+1];
				
			++pos;

			if ( n == '\n' or n == '\r' )
				return;
		}
	}
	
/*
=================================================
	ToBeginOfLine
=================================================
*/
	void StringParser::ToBeginOfLine (StringView str, INOUT size_t &pos)
	{
		pos = Min( pos, str.length() );

		while ( pos <= str.length() )
		{
			//const char	c = str[pos];
			const char	p = (pos-1) >= str.length() ? '\0' : str[pos-1];
				
			if ( p == '\n' or p == '\r' or p == '\0' )
				return;

			--pos;
		}
		pos = 0;
	}
	
/*
=================================================
	IsBeginOfLine
=================================================
*/
	bool StringParser::IsBeginOfLine (StringView str, const size_t pos)
	{
		size_t	p = pos;
		ToBeginOfLine( str, INOUT p );
		return p == pos;
	}
	
/*
=================================================
	IsEndOfLine
=================================================
*/
	bool StringParser::IsEndOfLine (StringView str, const size_t pos)
	{
		size_t	p = pos;
		ToEndOfLine( str, INOUT p );
		return p == pos;
	}

/*
=================================================
	ToNextLine
=================================================
*/
	void StringParser::ToNextLine (StringView str, INOUT size_t &pos)
	{
		while ( pos < str.length() )
		{
			const char	c = str[pos];
			const char	n = (pos+1) >= str.length() ? 0 : str[pos+1];
			
			++pos;

			// windows style "\r\n"
			if ( c == '\r' and n == '\n' )
			{
				++pos;
				return;
			}

			// linux style "\n" (or mac style "\r")
			if ( c == '\n' or c == '\r' )
				return;
		}
	}
	
/*
=================================================
	ToPrevLine
=================================================
*/
	void StringParser::ToPrevLine (StringView str, INOUT size_t &pos)
	{
		pos = Min( pos, str.length() );

		while ( pos <= str.length() )
		{
			const char	c = str[pos];
			const char	p = (pos-1) >= str.length() ? 0 : str[pos-1];
			
			--pos;

			// windows style "\r\n"
			if ( p == '\r' and c == '\n' )
			{
				--pos;
				return;
			}

			// linux style "\n" (or mac style "\r")
			if ( p == '\n' or p == '\r' )
				return;
		}
	}
	
/*
=================================================
	CalculateNumberOfLines
=================================================
*/
	size_t  StringParser::CalculateNumberOfLines (StringView str)
	{
		size_t	lines = 0;

		for (size_t pos = 0; pos < str.length(); ++lines)
		{
			ToNextLine( str, INOUT pos );
		}
		return lines;
	}
	
/*
=================================================
	MoveToLine
=================================================
*/
	bool StringParser::MoveToLine (StringView str, INOUT size_t &pos, size_t lineNumber)
	{
		size_t	lines = 0;

		for (; pos < str.length() and lines < lineNumber; ++lines)
		{
			ToNextLine( str, INOUT pos );
		}
		return lines == lineNumber;
	}

/*
=================================================
	ReadCurrLine
---
	Read line from begin of line to end of line
	and move to next line.
=================================================
*/
	void StringParser::ReadCurrLine (StringView str, INOUT size_t &pos, OUT StringView &result)
	{
		ToBeginOfLine( str, INOUT pos );

		ReadLineToEnd( str, INOUT pos, OUT result );
	}
	
/*
=================================================
	ReadLineToEnd
----
	Read line from current position to end of line
	and move to next line.
=================================================
*/
	void StringParser::ReadLineToEnd (StringView str, INOUT size_t &pos, OUT StringView &result)
	{
		const size_t	prev_pos = pos;

		ToEndOfLine( str, INOUT pos );

		result = str.substr( prev_pos, pos - prev_pos );

		ToNextLine( str, INOUT pos );
	}

/*
=================================================
	ReadString
----
	read string from " to "
=================================================
*/
	bool StringParser::ReadString (StringView str, INOUT size_t &pos, OUT StringView &result)
	{
		result = Default;

		for (; pos < str.length(); ++pos)
		{
			if ( str[pos] == '"' )
				break;
		}

		CHECK_ERR( str[pos] == '"' );

		const size_t	begin = ++pos;

		for (; pos < str.length(); ++pos)
		{
			const char	c = str[pos];

			if ( c == '"' )
			{
				result = StringView( str.data() + begin, pos - begin );
				++pos;
				return true;
			}
		}
	
		RETURN_ERR( "no pair for bracket \"" );
	}
	
/*
=================================================
	ReadTo
----
	read from 'pos' to symbol 'endSymbol'
=================================================
*/
	bool StringParser::ReadTo (StringView str, StringView endSymbol, INOUT size_t &pos, OUT StringView &result)
	{
		result = Default;

		size_t	start = pos;
		pos = str.find( endSymbol, start );

		if ( pos == StringView::npos )
			RETURN_ERR( "end symbol not found" );

		result = str.substr( start, pos );
		return true;
	}


}	// FG

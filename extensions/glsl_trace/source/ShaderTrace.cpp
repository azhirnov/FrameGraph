// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Common.h"

/*
=================================================
	_AppendSource
=================================================
*/
void ShaderTrace::_AppendSource (const char* source, size_t length)
{
	ASSERT( length > 0 );

	SourceInfo	info;
	size_t		pos = 0;

	info.code.assign( source, length );
	info.lines.reserve( 64 );

	for (size_t j = 0, len = info.code.length(); j < len; ++j)
	{
		const char	c = info.code[j];
		const char	n = (j+1) >= len ? 0 : info.code[j+1];

		// windows style "\r\n"
		if ( c == '\r' and n == '\n' )
		{
			info.lines.push_back({ pos, j });
			pos = (++j) + 1;
		}
		else
		// linux style "\n" (or mac style "\r")
		if ( c == '\n' or c == '\r' )
		{
			info.lines.push_back({ pos, j });
			pos = j + 1;
		}
	}

	if ( pos < info.code.length() )
		info.lines.push_back({ pos, info.code.length() });

	_sources.push_back( std::move(info) );
}

/*
=================================================
	SetSource
=================================================
*/
void ShaderTrace::SetSource (const char* const* sources, const size_t *lengths, size_t count)
{
	_sources.clear();
	_sources.reserve( count );

	for (size_t i = 0; i < count; ++i) {
		_AppendSource( sources[i], (lengths ? lengths[i] : strlen(sources[i])) );
	}
}

void ShaderTrace::SetSource (const char* source, size_t length)
{
	ASSERT( length > 0 );

	const char*	sources[] = { source };
	return SetSource( sources, &length, 1 );
}

void ShaderTrace::IncludeSource (const char* filename, const char* source, size_t length)
{
	ASSERT( length > 0 );

	_fileMap.insert_or_assign( std::string(filename), uint32_t(_sources.size()) );
	_AppendSource( source, length );
}

/*
=================================================
	GetSource
=================================================
*/
void ShaderTrace::GetSource (OUT std::string &result) const
{
	size_t	total_size = _sources.size()*2;

	for (auto& src : _sources) {
		total_size += src.code.length();
	}

	result.clear();
	result.reserve( total_size );

	for (auto& src : _sources) {
		result.append( src.code );
	}
}

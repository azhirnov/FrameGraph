// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Algorithms/StringUtils.h"
#include "stl/Containers/StaticString.h"
#include <chrono>

using namespace FG;

#define TEST	CHECK_FATAL


using TimePoint_t	= std::chrono::time_point< std::chrono::high_resolution_clock >;
using Duration_t	= TimePoint_t::duration;


template <size_t N>
inline void RandomString (OUT StaticString<N> &str)
{
	String	str2;
	size_t	len		= Min( size_t(rand() & 0xFF) + 12, str.capacity()-1 );
	uint	range	= ('Z' - 'A');

	for (uint i = 0; i < len; ++i) {
		uint v = rand() % (range*2);
		str2 += char(v > range ? 'a' + v - range : 'A' + v);
	}

	str = StringView(str2);
};

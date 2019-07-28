// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_STD_STRINGVIEW
#	include <string_view>

namespace FGC
{
							using StringView		= std::string_view;
	template <typename T>	using BasicStringView	= std::basic_string_view<T>;
}

#elif defined(FG_ENABLE_STRINGVIEW)

#	include "external/string_view/include/nonstd/string_view.hpp"

namespace FGC
{
							using StringView		= nonstd::string_view;
	template <typename T>	using BasicStringView	= nonstd::basic_string_view<T>;
}

#endif	// FG_STD_STRINGVIEW

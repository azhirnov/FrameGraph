// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_STD_STRINGVIEW
#	include <string_view>

namespace FG
{
							using StringView		= std::string_view;
	template <typename T>	using BasicStringView	= std::basic_string_view<T>;
}

#else

namespace FG
{
	template <typename T>	struct BasicStringView;

	using StringView = BasicStringView<char>;
}

#endif	// FG_STD_STRINGVIEW

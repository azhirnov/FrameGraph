// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_STD_OPTIONAL

# include <optional>

namespace FG
{
	template <typename T>	using Optional	= std::optional< T >;

}	// FG


#elif defined(FG_ENABLE_OPTIONAL)

# include "external/optional/optional.hpp"

namespace FG
{
	template <typename T>	using Optional	= std::experimental::optional< T >;

}	// FG

#endif	// FG_STD_OPTIONAL

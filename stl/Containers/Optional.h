// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_STD_OPTIONAL

# include <optional>

namespace FGC
{
	template <typename T>	using Optional	= std::optional< T >;

}	// FGC


#elif defined(FG_ENABLE_OPTIONAL)

# include "external/optional/optional.hpp"

namespace FGC
{
	template <typename T>	using Optional	= std::experimental::optional< T >;

}	// FGC

#else

# error Optional is not supported!

#endif	// FG_STD_OPTIONAL

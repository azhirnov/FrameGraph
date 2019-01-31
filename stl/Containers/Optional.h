// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef FG_STD_OPTIONAL

# include <optional>

namespace FG
{
	template <typename T>	using Optional	= std::optional< T >;

}	// FG


#else

namespace FG
{

	template <typename T>
	struct Optional
	{
		// TODO
	};


}	// FG

#endif	// FG_STD_OPTIONAL

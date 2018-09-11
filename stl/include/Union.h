// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include <variant>
#include "stl/include/Common.h"

namespace FG
{
	template <typename ...Types>	using Union		= std::variant< Types... >;
	
	template <typename... Types>	struct overloaded : Types... { using Types::operator()...; };

	template <typename... Types>	overloaded (Types...) -> overloaded<Types...>;

}	// FG

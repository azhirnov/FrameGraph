// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include <variant>
#include "stl/Common.h"

namespace FG
{
	namespace _fg_hidden_
	{
		template <typename... Types>	struct overloaded final : Types... { using Types::operator()...; };

		template <typename... Types>	overloaded (Types...) -> overloaded<Types...>;
	}
	

	template <typename ...Types>	using Union = std::variant< Types... >;
	
	
/*
=================================================
	Visit
=================================================
*/
	template <typename ...Types, typename ...Funcs>
	inline constexpr decltype(auto)  Visit (Union<Types...> &un, Funcs&&... fn)
	{
		using namespace _fg_hidden_;
		return std::visit( overloaded{ std::forward<Funcs &&>(fn)... }, un );
	}

	template <typename ...Types, typename ...Funcs>
	inline constexpr decltype(auto)  Visit (const Union<Types...> &un, Funcs&&... fn)
	{
		using namespace _fg_hidden_;
		return std::visit( overloaded{ std::forward<Funcs &&>(fn)... }, un );
	}

}	// FG

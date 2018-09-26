// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include <variant>
#include "stl/include/Common.h"

namespace FG
{
	namespace _fg_hidden_
	{
		template <typename... Types>	struct overloaded : Types... { using Types::operator()...; };

		template <typename... Types>	overloaded (Types...) -> overloaded<Types...>;
	}
	

	template <typename ...Types>	using Union = std::variant< Types... >;
	
	
/*
=================================================
	Visit
=================================================
*/
	template <typename UnionType, typename ...Funcs>
	inline void Visit (UnionType &un, Funcs... fn)
	{
		using namespace _fg_hidden_;
		std::visit( overloaded{ std::forward<Funcs>(fn)... }, un );
	}

	template <typename UnionType, typename ...Funcs>
	inline void Visit (const UnionType &un, Funcs... fn)
	{
		using namespace _fg_hidden_;
		std::visit( overloaded{ std::forward<Funcs>(fn)... }, un );
	}

}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

#ifdef FG_STD_VARIANT

# include <variant>

namespace FG
{
	namespace _fg_hidden_
	{
		template <typename... Types>	struct overloaded final : Types... { using Types::operator()...; };

		template <typename... Types>	overloaded (Types...) -> overloaded<Types...>;
	}
	

	template <typename ...Types>	using Union		= std::variant< Types... >;
									using NullUnion	= std::monostate;
	
/*
=================================================
	Visit
=================================================
*/
	template <typename ...Types, typename ...Funcs>
	forceinline constexpr decltype(auto)  Visit (Union<Types...> &un, Funcs&&... fn)
	{
		using namespace _fg_hidden_;
		return std::visit( overloaded{ std::forward<Funcs &&>(fn)... }, un );
	}

	template <typename ...Types, typename ...Funcs>
	forceinline constexpr decltype(auto)  Visit (const Union<Types...> &un, Funcs&&... fn)
	{
		using namespace _fg_hidden_;
		return std::visit( overloaded{ std::forward<Funcs &&>(fn)... }, un );
	}
	
/*
=================================================
	UnionGet
=================================================
*/
	template <typename T, typename ...Types>
	ND_ forceinline constexpr T&  UnionGet (Union<Types...> &un)
	{
		return std::get<T>( un );
	}
	
	template <typename T, typename ...Types>
	ND_ forceinline constexpr T*  UnionGetIf (Union<Types...> *un)
	{
		return std::get_if<T>( un );
	}
	
	template <typename T, typename ...Types>
	ND_ forceinline constexpr T const*  UnionGetIf (const Union<Types...> *un)
	{
		return std::get_if<T>( un );
	}

	template <typename T, typename ...Types>
	ND_ forceinline constexpr bool  HoldsAlternative (const Union<Types...> &un)
	{
		return std::holds_alternative<T>( un );
	}

}	// FG

#else

namespace FG
{

	struct NullUnion {};


	//
	// Union
	//

	template <typename ...Types>
	struct Union
	{
	};

	
/*
=================================================
	Visit
=================================================
*/
	template <typename ...Types, typename Func>
	forceinline constexpr decltype(auto)  Visit (Union<Types...> &un, Func&& fn)
	{

	}

/*
=================================================
	UnionGet
=================================================
*/
	template <typename T, typename ...Types>
	ND_ forceinline constexpr T&  UnionGet (Union<Types...> &un)
	{
		return un.template Get<T>();
	}
	
	template <typename T, typename ...Types>
	ND_ forceinline constexpr T*  UnionGetIf (Union<Types...> *un)
	{
		return un->template GetIf<T>();
	}
	
	template <typename T, typename ...Types>
	ND_ forceinline constexpr bool  HoldsAlternative (const Union<Types...> &un)
	{
		return un.template Is<T>();
	}

}	// FG

#endif	// FG_STD_VARIANT

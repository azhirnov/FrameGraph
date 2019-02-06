// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Common.h"

namespace FG
{
	namespace _fg_hidden_
	{
		template <typename... Types>	struct overloaded final : Types... { using Types::operator()...; };

		template <typename... Types>	overloaded (Types...) -> overloaded<Types...>;
	}
}	// FG


#ifdef FG_STD_VARIANT

# include <variant>

namespace FG
{
	template <typename ...Types>	using Union			= std::variant< Types... >;
									using NullUnion		= std::monostate;

	template <typename T>	constexpr std::in_place_type_t<T> InPlaceIndex {};
	
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
	ND_ forceinline constexpr T const&  UnionGet (const Union<Types...> &un)
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


#elif defined(FG_ENABLE_VARIANT)

# include "external/variant/include/mpark/variant.hpp"

namespace FG
{
	template <typename ...Types>	using Union		= mpark::variant< Types... >;
									using NullUnion	= mpark::monostate;

	template <typename T>	constexpr mpark::in_place_type_t<T>  InPlaceIndex {};
									
/*
=================================================
	Visit
=================================================
*/
	template <typename ...Types, typename ...Funcs>
	forceinline constexpr decltype(auto)  Visit (Union<Types...> &un, Funcs&&... fn)
	{
		using namespace _fg_hidden_;
		return mpark::visit( overloaded{ std::forward<Funcs &&>(fn)... }, un );
	}

	template <typename ...Types, typename ...Funcs>
	forceinline constexpr decltype(auto)  Visit (const Union<Types...> &un, Funcs&&... fn)
	{
		using namespace _fg_hidden_;
		return mpark::visit( overloaded{ std::forward<Funcs &&>(fn)... }, un );
	}
	
/*
=================================================
	UnionGet
=================================================
*/
	template <typename T, typename ...Types>
	ND_ forceinline constexpr T&  UnionGet (Union<Types...> &un)
	{
		return mpark::get<T>( un );
	}
	
	template <typename T, typename ...Types>
	ND_ forceinline constexpr T const&  UnionGet (const Union<Types...> &un)
	{
		return mpark::get<T>( un );
	}

	template <typename T, typename ...Types>
	ND_ forceinline constexpr T*  UnionGetIf (Union<Types...> *un)
	{
		return mpark::get_if<T>( un );
	}
	
	template <typename T, typename ...Types>
	ND_ forceinline constexpr T const*  UnionGetIf (const Union<Types...> *un)
	{
		return mpark::get_if<T>( un );
	}

	template <typename T, typename ...Types>
	ND_ forceinline constexpr bool  HoldsAlternative (const Union<Types...> &un)
	{
		return mpark::holds_alternative<T>( un );
	}

}	// FG

#endif	// FG_STD_VARIANT

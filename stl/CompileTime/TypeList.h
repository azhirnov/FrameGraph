// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"
#include <tuple>

namespace FG
{
namespace _fg_hidden_
{

	template <typename Type, size_t I, typename TL>
	struct TL_GetIndex;

	template <typename Type, size_t I>
	struct TL_GetIndex< Type, I, std::tuple<> >
	{
		inline static constexpr size_t	value = ~size_t(0);
	};

	template <typename Type, size_t I, typename Head, typename... Tail>
	struct TL_GetIndex< Type, I, std::tuple<Head, Tail...> >
	{
		inline static constexpr size_t	value = std::conditional_t< std::is_same_v<Type, Head>,
													std::integral_constant<size_t, I>, TL_GetIndex< Type, I+1, std::tuple<Tail...> > >::value;
	};


}	// _fg_hidden_


	//
	// Type List
	//

	template <typename... Types>
	struct TypeList
	{
		template <size_t I>
		using							Get		= typename std::tuple_element<I, std::tuple<Types...>>::type;

		template <typename T>
		inline static constexpr size_t	Index	= _fg_hidden_::TL_GetIndex< T, 0, std::tuple<Types...> >::value;

		inline static constexpr size_t	Count	= std::tuple_size< std::tuple<Types...> >::value;

		template <typename T>
		inline static constexpr bool	HasType	= (Index<T> != ~size_t(0));
	};

}	// FG

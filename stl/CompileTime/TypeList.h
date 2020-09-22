// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace FGC
{
namespace _fgc_hidden_
{

	template <typename Type, size_t I, typename TL>
	struct TL_GetIndex;

	template <typename Type, size_t I>
	struct TL_GetIndex< Type, I, std::tuple<> >
	{
		inline static constexpr size_t	value = UMax;
	};

	template <typename Type, size_t I, typename Head, typename... Tail>
	struct TL_GetIndex< Type, I, std::tuple<Head, Tail...> >
	{
		inline static constexpr size_t	value = Conditional< IsSameTypes<Type, Head>,
													std::integral_constant<size_t, I>, TL_GetIndex< Type, I+1, std::tuple<Tail...> > >::value;
	};

}	// _fgc_hidden_


	//
	// Type List
	//

	template <typename... Types>
	struct TypeList
	{
	public:
		template <typename T>
		inline static constexpr size_t	Index	= _fgc_hidden_::TL_GetIndex< T, 0, std::tuple<Types...> >::value;

		inline static constexpr size_t	Count	= std::tuple_size< std::tuple<Types...> >::value;

		template <typename T>
		inline static constexpr bool	HasType	= (Index<T> != UMax);
		
		template <size_t I>
		using							Get		= typename std::tuple_element<I, std::tuple<Types...>>::type;
		
		template <size_t I>
		using							GetT	= std::tuple_element<I, std::tuple<Types...>>;

		struct Front { using			type	= Get<0>; };
		struct Back  { using			type	= Get<Count-1>; };

		template <typename FN>
		static constexpr void Visit (FN&& fn)	{ return _Visit<0>( std::forward<FN>(fn) ); }

	private:
		template <size_t I, typename FN>
		static constexpr void _Visit (FN&& fn)
		{
			if constexpr( I < Count )
			{
				using T = Get<I>;
				fn.template operator()<T,I>();
				_Visit< I+1 >( std::forward<FN>(fn) );
			}
			Unused( fn );
		}
	};

	
	template <typename... Types>
	struct TypeList< std::tuple<Types...> > final : TypeList< Types... >
	{};


}	// FGC

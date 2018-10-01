// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Common.h"

namespace FG
{
	namespace _fg_hidden_
	{
		template <typename T, bool IsEnum>
		struct _IsEnumWithUnknown2 {
			static const bool	value = false;
		};

		template <typename T>
		struct _IsEnumWithUnknown2< T, true > {
			static const bool	value = true; //Detect_Unknown<T>::value;
		};

		template <typename T>
		static constexpr bool	_IsEnumWithUnknown = _IsEnumWithUnknown2< T, std::is_enum_v<T> >::value;


		template <typename T, int Index>
		struct _GetDefaultValueForUninitialized2 {};

		template <typename T>
		struct _GetDefaultValueForUninitialized2< T, 0 > {
			static T Get ()		{ return T(); }
		};

		template <typename T>
		struct _GetDefaultValueForUninitialized2< T, /*int, float, pointer*/2 > {
			static T Get ()		{ return T(0); }
		};
		
		template <typename T>
		struct _GetDefaultValueForUninitialized2< T, /*enum*/1 > {
			static T Get ()		{ return T::Unknown; }
		};


		template <typename T>
		struct _GetDefaultValueForUninitialized
		{
			static constexpr int GetIndex ()
			{
				return	_IsEnumWithUnknown<T>  ? 1 :
							std::is_floating_point<T>::value or
							std::is_integral<T>::value		 or
							std::is_pointer<T>::value		 or
							std::is_enum<T>::value  ? 2 :
								0;
			}

			static constexpr T GetDefault ()
			{
				return _GetDefaultValueForUninitialized2< T, GetIndex() >::Get();
			}
		};
		

		struct DefaultType final
		{
			constexpr DefaultType ()
			{}

			template <typename T>
			ND_ constexpr operator T () const
			{
				return _GetDefaultValueForUninitialized<T>::GetDefault();
			}
		};

	}	// _fg_hidden_

		
	static constexpr _fg_hidden_::DefaultType		Default = {};

}	// FG

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include <type_traits>

namespace FG
{

	template <typename T>
	static constexpr bool	IsFloatPoint		= std::is_floating_point_v<T>;

	template <typename T>
	static constexpr bool	IsInteger			= std::is_integral_v<T>;
	
	template <typename T>
	static constexpr bool	IsSignedInteger		= std::is_integral_v<T> && std::is_signed_v<T>;
	
	template <typename T>
	static constexpr bool	IsUnsignedInteger	= std::is_integral_v<T> && std::is_unsigned_v<T>;

	template <typename T>
	static constexpr bool	IsStaticArray		= std::is_array_v<T>;

	template <typename T>
	static constexpr bool	IsScalar			= std::is_scalar_v<T>;
	
	template <typename T>
	static constexpr bool	IsEnum				= std::is_enum_v<T>;
	
	template <typename T>
	static constexpr bool	IsScalarOrEnum		= std::is_scalar_v<T> or std::is_enum_v<T>;

	template <typename T>
	static constexpr bool	IsPOD				= std::is_pod_v<T>;

	template <typename T>
	static constexpr bool	IsPointer			= std::is_pointer_v<T>;

	template <typename T>
	static constexpr bool	IsClass				= std::is_class_v<T>;

	template <typename T>
	static constexpr bool	IsUnion				= std::is_union_v<T>;

	template <typename T>
	static constexpr bool	IsConst				= std::is_const_v<T>;

	template <typename T1, typename T2>
	static constexpr bool	IsSameTypes			= std::is_same_v<T1, T2>;


	template <bool Test, typename Type = void>
	using EnableIf		= std::enable_if_t< Test, Type >;


	template <bool Test, typename IfTrue, typename IfFalse>
	using Conditional	= std::conditional_t< Test, IfTrue, IfFalse >;


}	// FG

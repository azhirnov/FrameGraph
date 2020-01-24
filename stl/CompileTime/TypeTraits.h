// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include <type_traits>
#include <cstdint>

namespace FGC
{

	template <typename T>
	static constexpr bool	IsFloatPoint		= std::is_floating_point<T>::value;

	template <typename T>
	static constexpr bool	IsInteger			= std::is_integral<T>::value;
	
	template <typename T>
	static constexpr bool	IsSignedInteger		= std::is_integral<T>::value && std::is_signed<T>::value;
	
	template <typename T>
	static constexpr bool	IsUnsignedInteger	= std::is_integral<T>::value && std::is_unsigned<T>::value;

	template <typename T>
	static constexpr bool	IsStaticArray		= std::is_array<T>::value;

	template <typename T>
	static constexpr bool	IsScalar			= std::is_scalar<T>::value;
	
	template <typename T>
	static constexpr bool	IsEnum				= std::is_enum<T>::value;
	
	template <typename T>
	static constexpr bool	IsScalarOrEnum		= std::is_scalar<T>::value or std::is_enum<T>::value;

	template <typename T>
	static constexpr bool	IsPOD				= std::is_pod<T>::value;

	template <typename T>
	static constexpr bool	IsPointer			= std::is_pointer<T>::value;

	template <typename T>
	static constexpr bool	IsClass				= std::is_class<T>::value;

	template <typename T>
	static constexpr bool	IsUnion				= std::is_union<T>::value;

	template <typename T>
	static constexpr bool	IsConst				= std::is_const<T>::value;

	template <typename T1, typename T2>
	static constexpr bool	IsSameTypes			= std::is_same<T1, T2>::value;


	template <bool Test, typename Type = void>
	using EnableIf		= std::enable_if_t< Test, Type >;

	template <bool Test, typename Type = void>
	using DisableIf		= std::enable_if_t< !Test, Type >;


	template <bool Test, typename IfTrue, typename IfFalse>
	using Conditional	= std::conditional_t< Test, IfTrue, IfFalse >;


	template <typename T>
	using ToUnsignedInteger	= Conditional< sizeof(T) == sizeof(uint64_t), uint64_t,
								Conditional< sizeof(T) == sizeof(uint32_t), uint32_t,
									Conditional< sizeof(T) == sizeof(uint16_t), uint16_t,
										Conditional< sizeof(T) == sizeof(uint8_t), uint8_t,
											void >>>>;
	
	template <typename T>
	using ToSignedInteger	= Conditional< sizeof(T) == sizeof(int64_t), int64_t,
								Conditional< sizeof(T) == sizeof(int32_t), int32_t,
									Conditional< sizeof(T) == sizeof(int16_t), int16_t,
										Conditional< sizeof(T) == sizeof(int8_t), int8_t,
											void >>>>;

}	// FGC

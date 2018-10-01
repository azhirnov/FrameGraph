// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include <vector>
#include <string>
#include <array>
#include <string_view>
#include <memory>		// shared_ptr, weak_ptr, unique_ptr
#include <optional>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <bitset>
#include <cstring>
#include <cmath>

#include "stl/Defines.h"
#include "stl/Log/Log.h"
#include "stl/Algorithms/Hash.h"


namespace FG
{
	using uint			= uint32_t;

	using StringView	= std::string_view;
	using String		= std::string;


	template <typename T>	using Optional	= std::optional< T >;

	template <typename T>	using Array		= std::vector< T >;

	template <typename T>	using UniquePtr	= std::unique_ptr< T >;

	template <typename T>	using SharedPtr	= std::shared_ptr< T >;
	template <typename T>	using WeakPtr	= std::weak_ptr< T >;

	template <typename T>	using Deque		= std::deque<T>;


	template <typename T,
			  size_t ArraySize>
	using StaticArray	= std::array< T, ArraySize >;


	template <typename FirstT,
			  typename SecondT>
	using Pair = std::pair< FirstT, SecondT >;
	

	template <typename T,
			  typename Hash = std::hash<T>>
	using HashSet = std::unordered_set< T, Hash >;


	template <typename Key,
			  typename Value,
			  typename Hash = std::hash<Key>>
	using HashMap = std::unordered_map< Key, Value, Hash >;


}	// FG

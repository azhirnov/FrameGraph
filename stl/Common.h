// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Frame Graph Core library
*/

#pragma once

#include "stl/Defines.h"

#include <vector>
#include <string>
#include <array>
#include <memory>		// shared_ptr, weak_ptr, unique_ptr
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <bitset>
#include <cstring>
#include <cmath>
#include <malloc.h>
#include <atomic>
#include <mutex>

#include "stl/Log/Log.h"
#include "stl/Algorithms/Hash.h"
#include "stl/CompileTime/TypeTraits.h"
#include "stl/CompileTime/UMax.h"
#include "stl/CompileTime/DefaultType.h"


namespace FGC
{
	using uint = uint32_t;

							using String		= std::string;
	template <typename T>	using BasicString	= std::basic_string< T >;

	template <typename T>	using Array			= std::vector< T >;

	template <typename T>	using UniquePtr		= std::unique_ptr< T >;

	template <typename T>	using SharedPtr		= std::shared_ptr< T >;
	template <typename T>	using WeakPtr		= std::weak_ptr< T >;

	template <typename T>	using Deque			= std::deque< T >;

	template <size_t N>		using BitSet		= std::bitset< N >;

	template <typename...T>	using Tuple			= std::tuple< T... >;

	template <typename T>	using Atomic		= std::atomic< T >;


	using Mutex	= std::mutex;


	template <typename T,
			  size_t ArraySize>
	using StaticArray	= std::array< T, ArraySize >;


	template <typename FirstT,
			  typename SecondT>
	using Pair = std::pair< FirstT, SecondT >;
	

	template <typename T,
			  typename Hasher = std::hash<T>>
	using HashSet = std::unordered_set< T, Hasher >;


	template <typename Key,
			  typename Value,
			  typename Hasher = std::hash<Key>>
	using HashMap = std::unordered_map< Key, Value, Hasher >;


#	ifdef FG_OPTIMAL_MEMORY_ORDER
	static constexpr std::memory_order	memory_order_acquire	= std::memory_order_acquire;
	static constexpr std::memory_order	memory_order_release	= std::memory_order_release;
	static constexpr std::memory_order	memory_order_acq_rel	= std::memory_order_acq_rel;
	static constexpr std::memory_order	memory_order_relaxed	= std::memory_order_relaxed;
#	else
	static constexpr std::memory_order	memory_order_acquire	= std::memory_order_seq_cst;
	static constexpr std::memory_order	memory_order_release	= std::memory_order_seq_cst;
	static constexpr std::memory_order	memory_order_acq_rel	= std::memory_order_seq_cst;
	static constexpr std::memory_order	memory_order_relaxed	= std::memory_order_seq_cst;
#	endif	// FG_OPTIMAL_MEMORY_ORDER


}	// FGC

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Math.h"
#include "stl/Math/Bytes.h"
#include "stl/Memory/UntypedAllocator.h"
#include "stl/Containers/BitTree.h"

namespace FG
{

	//
	// Indexed Pool
	//

	template <typename ValueType,
			  typename IndexType = uint,
			  size_t MaxSize = 1024
			 >
	struct IndexedPool2
	{
	// types
	public:
		using Self			= IndexedPool2< ValueType, IndexType, MaxSize >;
		using Index_t		= IndexType;
		using Value_t		= ValueType;

	private:
		using BitTree_t		= BitTree< IndexType, IndexType, MaxSize >;
		using Values_t		= StaticArray< Value_t, MaxSize >;


	// variables
	private:
		BitTree_t		_bits;
		Values_t		_values;


	// methods
	public:
		IndexedPool2 () {}
		~IndexedPool2 () {}
		
		IndexedPool2 (const Self &) = delete;
		IndexedPool2 (Self &&) = delete;

		Self& operator = (const Self &) = delete;
		Self& operator = (Self &&) = delete;

		ND_ bool Assign (OUT Index_t &index)					{ return _bits.Assign( OUT index ); }
		ND_ bool IsAssigned (Index_t index) const				{ return _bits.IsAssigned( index ); }
			void Unassign (Index_t index)						{ return _bits.Unassign( index ); }

		ND_ Value_t &		operator [] (Index_t index)			{ return _values[ index ]; }
		ND_ Value_t const&	operator [] (Index_t index) const	{ return _values[ index ]; }
	};


}	// FG

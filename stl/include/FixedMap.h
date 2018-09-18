// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Map emulation on static array with linear search.
	Use only for small number of elements.
*/

#pragma once

#include "stl/include/Common.h"

namespace FG
{

	//
	// Fixed Size Map
	//

	template <typename Key, typename Value, size_t ArraySize>
	struct FixedMap
	{
	// types
	public:
		using iterator			= Pair< const Key, Value > *;
		using const_iterator	= Pair< Key, Value > const *;
		using pair_t			= Pair< Key, Value >;

	private:
		using Container_t		= std::array< Pair< Key, Value >, ArraySize >;
		using Self				= FixedMap< Key, Value, ArraySize >;


	// variables
	private:
		Container_t		_array;
		size_t			_count	= 0;


	// methods
	public:
		FixedMap () {}

		ND_ size_t			size ()			const	{ return _count; }
		ND_ bool			empty ()		const	{ return _count == 0; }

		ND_ iterator		begin ()				{ return BitCast< iterator >(_array.data()); }
		ND_ const_iterator	begin ()		const	{ return BitCast< const_iterator >(_array.data()); }
		ND_ iterator		end ()					{ return begin() + _count; }
		ND_ const_iterator	end ()			const	{ return begin() + _count; }
		
		ND_ static constexpr size_t	capacity ()		{ return ArraySize; }


		ND_ bool	operator == (const Self &rhs) const
		{
			if ( _count != rhs._count )
				return false;

			// warning: left and right data must be sorted!
			for (size_t i = 0; i < _count; ++i)
			{
				if ( _array[i] != rhs._array[i] )
					return false;
			}
			return true;
		}


		Pair<const_iterator,bool>	insert (const Pair<Key, Value> &value)
		{
			ASSERT( _count < ArraySize );
			ASSERT( count( value.first ) == 0 );

			return { BitCast< const_iterator >( &(_array[_count++] = value) ), true };
		}


		Pair<const_iterator,bool>	insert (Pair<Key, Value> &&value)
		{
			ASSERT( _count < ArraySize );
			ASSERT( count( value.first ) == 0 );

			return { BitCast< const_iterator >( &(_array[_count++] = std::move(value)) ), true };
		}


		ND_ const_iterator	find (const Key &key) const
		{
			const const_iterator	tail = end();
			const_iterator			iter = begin();

			for (; iter != tail and iter->first != key; ++iter)
			{}
			return iter;
		}


		ND_ iterator	find (const Key &key)
		{
			const iterator	tail = end();
			iterator		iter = begin();

			for (; iter != tail and iter->first != key; ++iter)
			{}
			return iter;
		}


		ND_ size_t	count (const Key &key) const
		{
			const const_iterator	tail = end();
			const_iterator			iter = begin();
			size_t					cnt  = 0;
			
			for (; iter != tail; ++iter)
			{
				if ( iter->first == key )
					++cnt;
			}
			return cnt;
		}
		

		void clear ()
		{
			for (auto iter = _array.begin(); iter != _array.end(); ++iter)
			{
                typename Container_t::value_type	temp;
				std::swap( *iter, temp );
			}
			_count = 0;
		}


		void sort ()
		{
			std::sort( _array.data(), _array.data() + _count, [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; } );
		}
	};


}	// FG


namespace std
{
	template <typename Key, typename Value, size_t ArraySize>
	struct hash< FG::FixedMap<Key, Value, ArraySize> >
	{
		ND_ size_t  operator () (const FG::FixedMap<Key, Value, ArraySize> &value) const noexcept
		{
			FG::HashVal	result;

			for (auto& item : value) {
				result << FG::HashOf( item );
			}
			return size_t(result);
		}
	};

}	// std

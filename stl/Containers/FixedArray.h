// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/Memory/MemUtils.h"

namespace FGC
{

	//
	// Fixed Size Array
	//

	template <typename T, size_t ArraySize>
	struct alignas(Max(alignof(T), sizeof(void*))) FixedArray
	{
	// types
	public:
		using iterator			= T *;
		using const_iterator	= const T *;
		using Self				= FixedArray< T, ArraySize >;


	// variables
	private:
		union {
			T			_array[ ArraySize ];
			char		_data[ ArraySize * sizeof(T) ];		// don't use this field!
		};
		size_t			_count	= 0;


	// methods
	public:
		constexpr FixedArray ()
		{
			DEBUG_ONLY( memset( data(), 0, sizeof(T) * capacity() ));
			
			STATIC_ASSERT( alignof(Self) % alignof(T) == 0 );
		}

		constexpr FixedArray (std::initializer_list<T> list) : FixedArray()
		{
			ASSERT( list.size() < capacity() );
			assign( list.begin(), list.end() );
		}

		constexpr FixedArray (ArrayView<T> view) : FixedArray()
		{
			ASSERT( view.size() < capacity() );
			assign( view.begin(), view.end() );
		}

		constexpr FixedArray (const Self &other) : FixedArray()
		{
			assign( other.begin(), other.end() );
		}

		constexpr FixedArray (Self &&other) : _count{other._count}
		{
			ASSERT( not _IsMemoryAliased( other.begin(), other.end() ) );

			for (size_t i = 0; i < _count; ++i) {
				PlacementNew<T>( data() + i, std::move(other._array[i]) );
			}
			other.clear();
		}

		~FixedArray ()
		{
			clear();
		}


		ND_ constexpr operator ArrayView<T> ()					const	{ return ArrayView<T>{ data(), _count }; }

		ND_ constexpr size_t			size ()					const	{ return _count; }
		ND_ constexpr bool				empty ()				const	{ return _count == 0; }
		ND_ constexpr T *				data ()							{ return std::addressof( _array[0] ); }
		ND_ constexpr T const *			data ()					const	{ return std::addressof( _array[0] ); }

		ND_ constexpr T &				operator [] (size_t i)			{ ASSERT( i < _count );  return _array[i]; }
		ND_ constexpr T const &			operator [] (size_t i)	const	{ ASSERT( i < _count );  return _array[i]; }

		ND_ constexpr iterator			begin ()						{ return data(); }
		ND_ constexpr const_iterator	begin ()				const	{ return data(); }
		ND_ constexpr iterator			end ()							{ return data() + _count; }
		ND_ constexpr const_iterator	end ()					const	{ return data() + _count; }

		ND_ constexpr T &				front ()						{ ASSERT( _count > 0 );  return _array[0]; }
		ND_ constexpr T const&			front ()				const	{ ASSERT( _count > 0 );  return _array[0]; }
		ND_ constexpr T &				back ()							{ ASSERT( _count > 0 );  return _array[_count-1]; }
		ND_ constexpr T const&			back ()					const	{ ASSERT( _count > 0 );  return _array[_count-1]; }
		
		ND_ static constexpr size_t		capacity ()						{ return ArraySize; }
		
		ND_ constexpr bool  operator == (ArrayView<T> rhs)		const	{ return ArrayView<T>{*this} == rhs; }
		ND_ constexpr bool  operator != (ArrayView<T> rhs)		const	{ return ArrayView<T>{*this} != rhs; }
		ND_ constexpr bool  operator >  (ArrayView<T> rhs)		const	{ return ArrayView<T>{*this} >  rhs; }
		ND_ constexpr bool  operator <  (ArrayView<T> rhs)		const	{ return ArrayView<T>{*this} <  rhs; }
		ND_ constexpr bool  operator >= (ArrayView<T> rhs)		const	{ return ArrayView<T>{*this} >= rhs; }
		ND_ constexpr bool  operator <= (ArrayView<T> rhs)		const	{ return ArrayView<T>{*this} <= rhs; }


		constexpr FixedArray&  operator = (const Self &rhs)
		{
			assign( rhs.begin(), rhs.end() );
			return *this;
		}
		
		constexpr FixedArray&  operator = (ArrayView<T> rhs)
		{
			ASSERT( rhs.size() < capacity() );
			assign( rhs.begin(), rhs.end() );
			return *this;
		}

		constexpr FixedArray&  operator = (Self &&rhs)
		{
			ASSERT( not _IsMemoryAliased( rhs.begin(), rhs.end() ) );

			clear();
			_count = rhs._count;

			for (size_t i = 0; i < _count; ++i) {
				PlacementNew<T>( data() + i, std::move(rhs._array[i]) );
			}
			rhs.clear();
			return *this;
		}


		constexpr void  assign (const_iterator beginIter, const_iterator endIter)
		{
			ASSERT( beginIter <= endIter );
			ASSERT( not _IsMemoryAliased( beginIter, endIter ) );

			clear();

			for (auto iter = beginIter; _count < capacity() and iter != endIter; ++iter, ++_count)
			{
				PlacementNew<T>( data() + _count, *iter );
			}
		}


		constexpr void  append (ArrayView<T> items)
		{
			for (auto& item : items) {
				push_back( item );
			}
		}


		constexpr void  push_back (const T &value)
		{
			ASSERT( _count < capacity() );
			PlacementNew<T>( data() + (_count++), value );
		}

		constexpr void  push_back (T &&value)
		{
			ASSERT( _count < capacity() );
			PlacementNew<T>( data() + (_count++), std::move(value) );
		}


		template <typename ...Args>
		constexpr T&  emplace_back (Args&& ...args)
		{
			ASSERT( _count < capacity() );
			T* ptr = data() + (_count++);
			PlacementNew<T>( ptr, std::forward<Args &&>( args )... );
			return *ptr;
		}


		constexpr void  pop_back ()
		{
			ASSERT( _count > 0 );
			--_count;

			_array[_count].~T();
			DEBUG_ONLY( memset( data() + _count, 0, sizeof(T) ));
		}


		constexpr void  insert (size_t pos, T &&value)
		{
			ASSERT( _count < capacity() );

			if ( pos >= _count )
				return push_back( std::move(value) );

			for (size_t i = _count++; i > pos; --i) {
				PlacementNew<T>( data() + i, std::move(data()[i-1]) );
			}

			PlacementNew<T>( data() + pos, std::move(value) );
		}


		constexpr void  resize (size_t newSize)
		{
			newSize = Min( newSize, capacity() );

			if ( newSize < _count )
			{
				// delete elements
				for (size_t i = newSize; i < _count; ++i)
				{
					_array[i].~T();
				}
				DEBUG_ONLY( memset( data() + newSize, 0, sizeof(T) * (_count - newSize) ));
			}

			if ( newSize > _count )
			{
				// create elements
				for (size_t i = _count; i < newSize; ++i)
				{
					PlacementNew<T>( data() + i );
				}
			}

			_count = newSize;
		}


		constexpr void  clear ()
		{
			for (size_t i = 0; i < _count; ++i)
			{
				_array[i].~T();
			}

			_count = 0;

			DEBUG_ONLY( memset( data(), 0, sizeof(T) * capacity() ));
		}


		constexpr void  fast_erase (size_t index)
		{
			ASSERT( index < _count );

			--_count;

			if ( index == _count )
			{
				// erase from back
				_array[_count].~T();
			}
			else
			{
				// move element from back to 'index'
				_array[index].~T();
				PlacementNew<T>( data() + index, std::move(_array[_count]) );
			}

			DEBUG_ONLY( memset( data() + _count, 0, sizeof(T) ));
		}


	private:
		ND_ forceinline bool  _IsMemoryAliased (const_iterator beginIter, const_iterator endIter) const
		{
			return IsIntersects( begin(), end(), beginIter, endIter );
		}
	};


}	// FGC


namespace std
{
	template <typename T, size_t ArraySize>
	struct hash< FGC::FixedArray<T, ArraySize> >
	{
        ND_ size_t  operator () (const FGC::FixedArray<T, ArraySize> &value) const
		{
			return size_t(FGC::HashOf( FGC::ArrayView<T>{ value } ));
		}
	};

}	// std

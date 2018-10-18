// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"

namespace FG
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


	// variables
	private:
		union {
			T			_array[ ArraySize ];
			char		_data[ ArraySize * sizeof(T) ];		// don't use this field!
		};
		size_t			_count	= 0;


	// methods
	public:
		FixedArray ()
		{
			DEBUG_ONLY( ::memset( data(), 0, sizeof(T) * capacity() ));
			
			STATIC_ASSERT( alignof(FixedArray<T,ArraySize>) % alignof(T) == 0 );
		}

		FixedArray (std::initializer_list<T> list) : FixedArray()
		{
			ASSERT( list.size() < capacity() );
			assign( list.begin(), list.end() );
		}

		FixedArray (ArrayView<T> view) : FixedArray()
		{
			ASSERT( view.size() < capacity() );
			assign( view.begin(), view.end() );
		}

		FixedArray (const FixedArray &other) : FixedArray()
		{
			assign( other.begin(), other.end() );
		}

		FixedArray (FixedArray &&other) : _count{other._count}
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


		ND_ operator ArrayView<T> ()				const	{ return ArrayView<T>{ data(), _count }; }

		ND_ size_t			size ()					const	{ return _count; }
		ND_ bool			empty ()				const	{ return _count == 0; }
		ND_ T *				data ()							{ return std::addressof( _array[0] ); }
		ND_ T const *		data ()					const	{ return std::addressof( _array[0] ); }

		ND_ T &				operator [] (size_t i)			{ ASSERT( i < _count );  return _array[i]; }
		ND_ T const &		operator [] (size_t i)	const	{ ASSERT( i < _count );  return _array[i]; }

		ND_ iterator		begin ()						{ return data(); }
		ND_ const_iterator	begin ()				const	{ return data(); }
		ND_ iterator		end ()							{ return data() + _count; }
		ND_ const_iterator	end ()					const	{ return data() + _count; }

		ND_ T &				front ()						{ ASSERT( _count > 0 );  return _array[0]; }
		ND_ T const&		front ()				const	{ ASSERT( _count > 0 );  return _array[0]; }
		ND_ T &				back ()							{ ASSERT( _count > 0 );  return _array[_count-1]; }
		ND_ T const&		back ()					const	{ ASSERT( _count > 0 );  return _array[_count-1]; }
		
		ND_ static constexpr size_t	capacity ()				{ return ArraySize; }
		

		FixedArray& operator = (const FixedArray &rhs)
		{
			assign( rhs.begin(), rhs.end() );
			return *this;
		}
		
		FixedArray& operator = (ArrayView<T> rhs)
		{
			ASSERT( rhs.size() < capacity() );
			assign( rhs.begin(), rhs.end() );
			return *this;
		}

		FixedArray& operator = (FixedArray &&rhs)
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


		ND_ bool  operator == (ArrayView<T> rhs) const
		{
			if ( size() != rhs.size() )
				return false;

			for (size_t i = 0; i < size(); ++i)
			{
				if ( not ((*this)[i] == rhs[i]) )
					return false;
			}
			return true;
		}

		ND_ bool  operator != (ArrayView<T> rhs) const
		{
			return not (*this == rhs);
		}

		ND_ bool  operator >  (ArrayView<T> rhs) const
		{
			if ( size() != rhs.size() )
				return size() > rhs.size();

			for (size_t i = 0; i < size(); ++i)
			{
				if ( (*this)[i] != rhs[i] )
					return (*this)[i] > rhs[i];
			}
			return true;
		}


		void assign (const_iterator beginIter, const_iterator endIter)
		{
			ASSERT( beginIter <= endIter );
			ASSERT( not _IsMemoryAliased( beginIter, endIter ) );

			clear();

			for (auto iter = beginIter; _count < capacity() and iter != endIter; ++iter, ++_count)
			{
				PlacementNew<T>( data() + _count, *iter );
			}
		}


		void push_back (const T &value)
		{
			ASSERT( _count < capacity() );
			PlacementNew<T>( data() + (_count++), value );
		}

		void push_back (T &&value)
		{
			ASSERT( _count < capacity() );
			PlacementNew<T>( data() + (_count++), std::move(value) );
		}


		template <typename ...Args>
		void emplace_back (Args&& ...args)
		{
			ASSERT( _count < capacity() );
			PlacementNew<T>( data() + (_count++), std::forward<Args &&>( args )... );
		}


		void pop_back ()
		{
			ASSERT( _count > 0 );
			--_count;

			_array[_count].~T();
			DEBUG_ONLY( ::memset( data() + _count, 0, sizeof(T) ));
		}


		void resize (size_t newSize)
		{
			newSize = Min( newSize, capacity() );

			if ( newSize < _count )
			{
				// delete elements
				for (size_t i = newSize; i < _count; ++i)
				{
					_array[i].~T();
				}
				DEBUG_ONLY( ::memset( data() + newSize, 0, sizeof(T) * (_count - newSize) ));
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


		void clear ()
		{
			for (size_t i = 0; i < _count; ++i)
			{
				_array[i].~T();
			}

			_count = 0;

			DEBUG_ONLY( ::memset( data(), 0, sizeof(T) * capacity() ));
		}


	private:
		ND_ forceinline bool _IsMemoryAliased (const_iterator beginIter, const_iterator endIter) const
		{
			return IsIntersects( begin(), end(), beginIter, endIter );
		}
	};


}	// FG


namespace std
{
	template <typename T, size_t ArraySize>
	struct hash< FG::FixedArray<T, ArraySize> >
	{
		ND_ size_t  operator () (const FG::FixedArray<T, ArraySize> &value) const noexcept
		{
			return size_t(FG::HashOf( FG::ArrayView<T>{ value } ));
		}
	};

}	// std

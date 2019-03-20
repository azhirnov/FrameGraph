// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/Types.h"

namespace FG
{

	//
	// Buffer View
	//

	struct BufferView
	{
	// types
	public:
		using T				= uint8_t;
		using value_type	= T;


		struct Iterator
		{
			friend struct BufferView;

		// variables
		private:
			ArrayView< ArrayView<T> >	_parts;
			size_t						_partIndex	= 0;
			size_t						_index		= 0;
			
		// methods
		private:
			Iterator (ArrayView<ArrayView<T>> parts, size_t partIndex, size_t index) :
				_parts{parts}, _partIndex{partIndex}, _index{index} {}

		public:
			Iterator (const Iterator &) = default;
			Iterator (Iterator &&) = default;

			ND_ bool  operator == (const Iterator &rhs) const
			{
				return	_parts.data()	== rhs._parts.data()	and
						_partIndex		== rhs._partIndex		and
						_index			== rhs._index;
			}

			ND_ T const&  operator *  () const
			{
				return _parts[_partIndex][_index];
			}

			ND_ T const*  operator -> () const
			{
				return &_parts[_partIndex][_index];
			}

			Iterator&  operator ++ ()
			{
				if ( ++_index >= _parts[_partIndex].size() ) {
					++_partIndex;
					_index = 0;
				}
				return *this;
			}

			Iterator  operator ++ (int)
			{
				Iterator res{*this};
				++(*this);
				return res;
			}

			Iterator&  operator -- ()
			{
				if ( --_index >= _parts[_partIndex].size() ) {
					--_partIndex;
					_index = _parts[_partIndex].size();
				}
				return *this;
			}

			Iterator  operator -- (int)
			{
				Iterator res{*this};
				--(*this);
				return res;
			}
		};


	// variables
	private:
		ArrayView< ArrayView<T> >	_parts;


	// methods
	public:
		BufferView ()
		{}

		BufferView (ArrayView<ArrayView<T>> parts) : _parts{parts}
		{}
		

		ND_ T const&  operator [] (size_t i) const
		{
			for (auto& val : _parts)
			{
				if ( i < val.size() )
					return val[i];

				i -= val.size();
			}

			ASSERT(false);	// index out of range
			return _parts.back().back();
		}


		ND_ ArrayView<ArrayView<T>>  Parts () const
		{
			return _parts;
		}


		ND_ Iterator  begin () const
		{
			return Iterator{ _parts, 0, 0 };
		}

		ND_ Iterator  end () const
		{
			return not empty() ?	Iterator{ _parts, _parts.size()-1, _parts.back().size() } :
									Iterator{ _parts, 0, 1 };
		}


		ND_ bool  empty () const
		{
			return size() == 0;
		}

		ND_ size_t  size () const
		{
			size_t	result = 0;
			for (auto& val : _parts) {
				result += val.size();
			}
			return result;
		}


		ND_ T const *  data () const
		{
			ASSERT( _parts.size() == 1 );
			return _parts.front().data();
		}


		ND_ ArrayView<T>  section (size_t first, size_t count) const
		{
			for (auto& val : _parts)
			{
				if ( first < val.size() )
					return val.section( first, count );

				first -= val.size();
			}

			ASSERT(false);
			return {};
		}
	};


}	// FG

// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"
#include "stl/Math/Math.h"
#include "stl/Memory/MemUtils.h"

namespace FGC
{

	//
	// Memory Writer
	//

	struct MemWriter
	{
	// variables
	private:
		void *		_ptr	= null;
		size_t		_offset	= 0;
		BytesU		_size;


	// methods
	public:
		MemWriter () {}
		MemWriter (void *ptr, BytesU size) : _ptr{ptr}, _size{size} {}


		ND_ void*  Reserve (BytesU size, BytesU align)
		{
			ASSERT( _ptr );
			size_t	result = AlignToLarger( size_t(_ptr) + _offset, size_t(align) );

			_offset = (result - size_t(_ptr)) + size_t(size);
			ASSERT( _offset <= _size );

			return BitCast<void *>( result );
		}


		template <typename T>
		ND_ T&  Reserve ()
		{
			return *Cast<T>( Reserve( SizeOf<T>, AlignOf<T> ));
		}

		template <typename T, typename ...Args>
		ND_ T&  Emplace (Args&& ...args)
		{
			return *PlacementNew<T>( &Reserve<T>(), std::forward<Args&&>( args )... );
		}

		template <typename T, typename ...Args>
		ND_ T&  EmplaceSized (BytesU size, Args&& ...args)
		{
			ASSERT( size >= SizeOf<T> );
			return *PlacementNew<T>( Reserve( size, AlignOf<T> ), std::forward<Args&&>( args )... );
		}


		template <typename T>
		ND_ T*  ReserveArray (size_t count)
		{
			return count ? Cast<T>( Reserve( SizeOf<T> * count, AlignOf<T> )) : null;
		}

		template <typename T, typename ...Args>
		ND_ T*  EmplaceArray (size_t count, Args&& ...args)
		{
			T*	result = ReserveArray<T>( count );

			for (size_t i = 0; i < count; ++i) {
				PlacementNew<T>( result + i, std::forward<Args&&>( args )... );
			}
			return result;
		}


		void Clear ()
		{
			ASSERT( _ptr );
			memset( _ptr, 0, size_t(_size) );
		}


		ND_ BytesU  OffsetOf (void *ptr, BytesU defaultValue = ~0_b) const
		{
			if ( ptr ) {
				ASSERT( ptr >= _ptr and ptr < _ptr + _size );
				return BytesU{size_t(ptr) - size_t(_ptr)};
			}
			return defaultValue;
		}
	};


}	// FGC

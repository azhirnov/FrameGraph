// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Containers/ArrayView.h"
#include "stl/Math/Bytes.h"

namespace FG
{

	//
	// Aligned Buffer
	//

	template <size_t Align>
	struct AlignedBuffer
	{
	// variables
	private:
		Array<uint8_t>		_data;
		size_t				_size	= 0;


	// methods
	public:
		AlignedBuffer ()							{}

		void reserve (size_t size)					{ return _data.reserve( size + align() ); }
		void reserve (BytesU size)					{ return reserve( size_t(size) ); }

		void resize (size_t size);
		void resize (BytesU size)					{ return resize( size_t(size) ); }

		void shrink_to_fit ()						{ return _data.shrink_to_fit(); }
		void clear ()								{ _data.clear();  _size = 0; }

		ND_ uint8_t *		data ();
		ND_ uint8_t const*	data ()			const	{ return const_cast< AlignedBuffer<Align> *>(this)->data(); }

		ND_ bool			empty ()		const	{ return _data.empty(); }
		ND_ size_t			size ()			const	{ return _size; }
		ND_ size_t			capacity ()		const	{ return _data.capacity(); }

		ND_ BytesU			SizeInBytes ()	const	{ return BytesU(size()); }

		ND_ operator ArrayView<uint8_t> ()	const	{ return { data(), size() }; }

		ND_ static constexpr size_t		align ()	{ return Align; }
	};

	
/*
=================================================
	data
=================================================
*/
	template <size_t A>
	inline uint8_t *  AlignedBuffer<A>::data ()
	{
		STATIC_ASSERT( A > 0 and (A & (A - 1)) == 0 );
		
		size_t	aligned = (BitCast<size_t>(_data.data()) + A-1) & ~(A-1);

		return BitCast<uint8_t *>( aligned );
	}
	
/*
=================================================
	resize
=================================================
*/
	template <size_t A>
	inline void  AlignedBuffer<A>::resize (size_t size)
	{
		_size = size;
		return _data.resize( size + align() );
	}

}	// FG

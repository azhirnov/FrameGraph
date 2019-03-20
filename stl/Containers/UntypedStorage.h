// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Math/Bytes.h"

namespace FGC
{

	//
	// Untyped Storage
	//

	template <size_t Size, size_t Align>
	struct UntypedStorage final
	{
	// types
	public:
		using Self = UntypedStorage< Size, Align >;


	// variables
	private:
		alignas(Align) char	_buffer [Size];


	// methods
	public:
		UntypedStorage ()
		{
			DEBUG_ONLY( memset( _buffer, 0, sizeof(_buffer) ));
		}

		template <typename T>
		ND_ T*  Cast (BytesU offset = 0_b)
		{
			ASSERT( SizeOf<T> + offset <= sizeof(_buffer) );
			return BitCast<T *>( _buffer + offset );
		}

		template <typename T>
		ND_ T const*  Cast (BytesU offset = 0_b) const
		{
			ASSERT( SizeOf<T> + offset <= sizeof(_buffer) );
			return BitCast<T const*>( _buffer + offset );
		}
	};


}	// FGC

// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Stream/Stream.h"
#include <stdio.h>
#include <filesystem>

namespace FGC
{

	//
	// Memory Write-only Stream
	//

	class MemWStream final : public WStream
	{
	// variables
	private:
		Array<uint8_t>		_data;


	// methods
	public:
		MemWStream () {
			_data.reserve( 4u << 20 );
		}

		~MemWStream () {}
		
		bool	IsOpen ()	const override		{ return true; }
		BytesU	Position ()	const override		{ return BytesU(_data.size()); }
		BytesU	Size ()		const override		{ return BytesU(_data.size()); }
		

		BytesU	Write2 (const void *buffer, BytesU size) override
		{
			const size_t	prev_size = _data.size();

			_data.resize( prev_size + size_t(size) );
			memcpy( _data.data() + BytesU(prev_size), buffer, size_t(size) );

			return size;
		}


		void Flush () override
		{
			// do nothing
		}


		ND_ ArrayView<uint8_t>  GetData () const	{ return _data; }

		void Clear ()								{ _data.clear(); }
	};


}	// FGC

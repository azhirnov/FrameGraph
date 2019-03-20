// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Stream/Stream.h"

namespace FGC
{

	//
	// Buffered Read-only Stream
	//

	class BufRStream final : public RStream
	{
	// variables
	private:
		Array<uint8_t>		_buffer;


	// methods
	public:
		BufRStream ();
		~BufRStream ();
		
		bool	IsOpen ()	const override;
		BytesU	Position ()	const override;
		BytesU	Size ()		const override;
		
		BytesU	Write2 (const void *buffer, BytesU size) override;
	};


}	// FGC

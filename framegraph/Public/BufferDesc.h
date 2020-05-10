// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/ResourceEnums.h"

namespace FG
{

	//
	// Buffer Description
	//

	struct BufferDesc
	{
	// variables
		BytesU			size;
		EBufferUsage	usage		= Default;
		EQueueUsage		queues		= Default;
		//bool			isLogical	= false;
		bool			isExternal	= false;

	// methods
		BufferDesc () {}
		BufferDesc (BytesU size, EBufferUsage usage, EQueueUsage queues = Default) : size{size}, usage{usage}, queues{queues} {}

		BufferDesc&  Size (BytesU value)			{ size = value;  return *this; }
		BufferDesc&  Size (size_t value)			{ size = BytesU{value};  return *this; }
		BufferDesc&  Usage (EBufferUsage value)		{ usage = value;  return *this; }
	};
	

	//
	// Buffer View description
	//

	struct BufferViewDesc
	{
	// variables
		EPixelFormat		format	= Default;
		BytesU				offset;
		BytesU				size	{ ~0_b };

	// methods
		BufferViewDesc () {}

		BufferViewDesc (EPixelFormat	format,
						BytesU			offset,
						BytesU			size) :
			format{format}, offset{offset}, size{size} {}

		void Validate (const BufferDesc &desc);
		
		ND_ bool operator == (const BufferViewDesc &rhs) const;
	};

}	// FG


namespace std
{
	template <>
	struct hash< FG::BufferViewDesc >
	{
		ND_ size_t  operator () (const FG::BufferViewDesc &value) const
		{
		#if FG_FAST_HASH
			return size_t(FGC::HashOf( AddressOf(value), sizeof(value) ));
		#else
			FG::HashVal	result;
			result << FGC::HashOf( value.format );
			result << FGC::HashOf( value.offset );
			result << FGC::HashOf( value.size );
			return size_t(result);
		#endif
		}
	};

}	// std

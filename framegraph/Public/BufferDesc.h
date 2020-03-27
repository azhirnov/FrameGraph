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


}	// FG

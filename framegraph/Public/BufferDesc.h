// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

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
		//bool			isLogical	= false;
		bool			isExternal	= false;

	// methods
		BufferDesc () {}
		BufferDesc (BytesU size, EBufferUsage usage) : size(size), usage(usage) {}
	};


}	// FG

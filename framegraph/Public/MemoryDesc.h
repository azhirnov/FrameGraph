// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "framegraph/Public/ResourceEnums.h"
#include "framegraph/Public/IDs.h"

namespace FG
{

	//
	// Memory Description
	//

	struct MemoryDesc
	{
	// variables
		EMemoryType		type	= EMemoryType::Default;
		MemPoolID		poolId;


	// methods
		MemoryDesc () {}
		MemoryDesc (EMemoryType type) : type{type} {}
		MemoryDesc (EMemoryType type, MemPoolID poolId) : type{type}, poolId{poolId} {}
	};


}	// FG
